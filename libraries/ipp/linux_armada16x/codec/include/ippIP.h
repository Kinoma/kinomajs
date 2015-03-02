/***************************************************************************************** 
Copyright (c) 2009, Marvell International Ltd. 
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Marvell nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY MARVELL ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL MARVELL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************************/


#if !defined( __IPPIP_H__ ) 
#define __IPPIP_H__
#define __IPPI_H__

#ifndef __IPPDEFS_H__
  #include "ippdefs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CALLBACK
#define _CALLBACK __STDCALL
#endif

void* _CALLBACK ippiMalloc(int size);
void  _CALLBACK ippiFree(void *pSrcBuf);

typedef enum {
    ippMskSize1x3 = 13,
    ippMskSize1x5 = 15,
    ippMskSize3x1 = 31,
    ippMskSize3x3 = 33,
    ippMskSize5x1 = 51,
    ippMskSize5x5 = 55
} IppiMaskSize;

typedef enum {
    ippAlphaOver,
    ippAlphaIn,
    ippAlphaOut,
    ippAlphaATop,
    ippAlphaXor,
    ippAlphaPlus,
    ippAlphaOverPremul,
    ippAlphaInPremul,
    ippAlphaOutPremul,
    ippAlphaATopPremul,
    ippAlphaXorPremul,
    ippAlphaPlusPremul
} IppiAlphaType;

typedef enum {
    ippDitherNone,
    ippDitherFS,
    ippDitherJJN,
    ippDitherStucki,
    ippDitherBayer
} IppiDitherType;

/*============ Intel IPP Camera Interface structure defination =============*/
typedef enum 
{
	ippCameraInterpNearest  	 = 0,    /* for nearest interpolation */
	ippCameraInterpBilinear      = 1,    /* for bilinear interpolation */
	ippCameraInterpMedian		 = 2,    /* for median interpolation */
	ippCameraInterpNearLinear    = 3    /* for NearLinear interpolation */
}IppCameraInterpolation;

typedef enum
{
	ippCameraRotateDisable       = 0,
	ippCameraRotate90L           = 1,    
	ippCameraRotate90R           = 2,
	ippCameraRotate180           = 3,
	ippCameraFlipHorizontal		 = 4,
	ippCameraFlipVertical		 = 5
}IppCameraRotation;

typedef enum
{
	ippCameraCscYCbCr422ToRGB565 = 0,
	ippCameraCscYCbCr422ToRGB555 = 1,
	ippCameraCscYCbCr422ToRGB444 = 2,
	ippCameraCscYCbCr422ToRGB888 = 3,
	ippCameraCscYCbCr420ToRGB565 = 4,
	ippCameraCscYCbCr420ToRGB555 = 5,
	ippCameraCscYCbCr420ToRGB444 = 6,
	ippCameraCscYCbCr420ToRGB888 = 7,

	/* CFA format to YUV422/420 format */
	ippCameraCscRGGBToYCbCr422P  = 8,
	ippCameraCscRGGBToYCbCr420   = 9,
/*	ippCameraCscBGGRToYCbCr422P  = 10,
	ippCameraCscBGGRToYCbCr420   = 11,
	ippCameraCscGRBGToYCbCr422P  = 12,
	ippCameraCscGRBGToYCbCr420   = 13,
	ippCameraCscGBRGToYCbCr422P  = 14,
	ippCameraCscGBRGToYCbCr420   = 15,*/  /* for future extension */

	ippCameraCscYCbCr422ToRGB666 = 16,
	ippCameraCscYCbCr420ToRGB666 = 17,
	ippCameraCscYCbCr422ToRGBA8888 = 18,
	ippCameraCscYCbCr420ToRGBA8888 = 19,

	ippCameraCscYCbCr420ToBGR888 = 20
}IppCameraCsc;

typedef enum
{
	ippGamPreOneTable            = 0,
	ippGamCusOneTable            = 1,
	ippGamPreThreeTable          = 2,
	ippGamCusThreeTable          = 3
} IppCameraGam;

typedef enum
{
	ippCameraAEHist              = 0,
	ippCameraAEMean              = 1,
	ippCameraAECenter            = 2,
	ippCameraAESpot              = 3,
	ippCameraAEMatrix            = 4
} IppCameraAEMethod;

typedef enum
{
	ippCameraAWBWGA              = 0,
	ippCameraAWBWPD              = 1
} IppCameraAWBMethod;

typedef struct
{
	int						srcStep;
	IppiSize				srcSize;
	int						dstStep[3];
	IppiSize				dstSize;
	IppCameraInterpolation	interpolation;
	IppCameraRotation		rotation;
	IppCameraCsc			colorConversion;
	IppBool					bExtendBorder;
} IppiRawPixProcCfg_P3R;

typedef struct
{
	IppCameraGam			GammaFlag;
	int						GammaIndex[3];
	Ipp8u*					pGammaTable[3];
	Ipp32u*					pDeadPixMap;
    int						DPMLen;
	IppiPoint				DPMOffset;
	IppCameraInterpolation  DPInterp;
    Ipp16s*					pCCMatrix;
} IppiCAMCfg;

typedef void IppiRawPixProcSpec_P3R;

/* AWB definitions */

/*****************************/
/*           AE/AWB          */
/*****************************/

typedef struct
{
	IppCameraAEMethod		AEMethod;
	IppiRect*				pWindows;
	Ipp16u*					pWeights;
	int						WindowsNum; 
	Ipp16u					MagicPointThreshold;
	Ipp16u					LowThreshold;
	Ipp16u					DarkThreshold;
	Ipp16u					HighThreshold;
	Ipp16u                  MedianThreshold; 	/* target brightness*/    
    Ipp16u                  NarrowThreshold;   
	Ipp16u					TStep;				/* integrationtime step */
	Ipp16u					GStep;				/* gain step */
	int						T_min;				/* Low limit of the integration time */
	int						T_max;				/* High limit of the integration time */
	int						G_min;				/* Low limit of the gain value */
	int						G_max;				/* High limit of the gain value */
} IppiAEConfig;

typedef struct
{
	IppCameraAWBMethod AWBMethod;
	int				WindowsNum;
	IppiRect*		pWindows;
	Ipp8u*			pWeights;
	Ipp16u			RefRG; /* Q10, AWB target r/g and b/g values.*/
	Ipp16u			RefBG;
	Ipp16u			RGlow; /* Q10, when |r/g 每 RefRG| < RGlow and |b/g 每 RefBG| < BGlow, we are close white enough, keep the old settings */
	Ipp16u			BGlow;
	Ipp16u			RGhigh; /* Q10, when |(r/g)/(RGain_old/GGain_old) 每 RefRG| > RGhigh or
									|(b/g)/(BGain_old/GGain_old) 每 RefBG| > BGhigh,
									we assert that a predominate color exists and stop AWB */
	Ipp16u			BGhigh;
	Ipp16u			T1; /* used by WPD criteria |R-G|<T1 */
	Ipp16u			T2; /* used by WPD criteria |B-G|<T2 */
	Ipp16u			T3; /* used by WPD criteria |R+B-2G|<T3 */
	Ipp16u			T4; /* used by WPD criteria R+B+2G>T4 */
	Ipp16u			T5; /* used by WPD criteria R+B+2G<T5 */
	int				ValidNum; /* used by WPD.
								for a single window, when the number of discriminated white point
									is smaller than ValidNum, this window is discarded.
								If all windows are discarded, WPD failed */
	Ipp16u			Step;
	Ipp16u			GainMin;
	Ipp16u			GainMax;
	Ipp16u			RGainDefault;
	Ipp16u			GGainDefault;
	Ipp16u			BGainDefault;
} IppiAWBCfg;

typedef void IppiAESpec_C1R;
typedef void IppiAWBSpec_C1R;

/*****************************/
/*          AE/AWB end       */
/*****************************/

/*****************************/
/*          AF begin         */
/*****************************/
typedef enum 
{
    AF_RGGB10 = 0,
    AF_RGB888 = 1,
	AF_YUV422 = 2,
	AF_RAW = 3
}IppAFColorFormat;  

typedef enum 
 {
	ippCameraNoiseDefault=0,	/*No noise reduction*/
	ippCameraMedianFilter=1,	/*Median filter*/
	ippCameraAverageFilter=2,	/*Average filter*/
	ippCameraGaussianFilter=3,	/*Gaussian Filter*/
	ippCameraWMFilter=4,		/*Weighted median filter*/ 
}IppCameraNoiseReduction;

/* AF camera type*/
typedef enum 
{
	ippCameraStillCamera=0,   /*Still camera */
	ippCameraVideoCamera=1,   /*Video camera*/
}IppCameraType;

/* AF config structure */
typedef struct 
{
	int			iSchLevel;
	int			iSchNumPerLevel[4];
	Ipp32s		iMaxLens;
	Ipp32s		iMinLens;
	IppCameraType cameraType;
	IppCameraNoiseReduction NRScheme;
}IppAFConfig;

typedef struct
{
	int iNextLens;
	int iOptimalLens;
	int iOptimalSharp;
	int iTrueAF;
	int iStop;
}IppiAFResult;

typedef void IppiAFSpec_C1R;

/*****************************/
/*          AF end		     */
/*****************************/


/* /////////////////////////////////////////////////////////////////////////////
//                  Copy/Set Functions
///////////////////////////////////////////////////////////////////////////// */

/* /////////////////////////////////////////////////////////////////////////////
// Name:       ippiSet_8u_C1R()
//             ippiSet_8u_C3R()
//             ippiSet_8u_AC4R()
//
// Purpose:    Set Image by the constant
//
// Returns:
//    ippStsNoErr       no error
//    ippStsNullPtrErr  NULL==pDst
//    ippStsStepErr     dstSTep <=0
//    ippStsSizeErr     dstRoiSize.width<1 || dstRoiSize.height<1
//
// Parameters:
//    value          value of the constant (or pointer to the array of constants)
//    pDst           pointer to the target image ROI
//    dstStep        target scan line size (bytes)
//    dstRoiSize     size of target ROI
//
*/
IPPAPI(IppStatus, ippiSet_8u_C1R, (
               Ipp8u value, Ipp8u* pDst, int dstStep,
               IppiSize dstRoiSize))
IPPAPI(IppStatus, ippiSet_8u_C3R, (
               const Ipp8u value[3], Ipp8u* pDst, int dstStep,
               IppiSize dstRoiSize))

/* /////////////////////////////////////////////////////////////////////////////
// Name:       ippiCopy_8u_C1R()
//             ippiCopy_8u_C3R()
//             ippiCopy_8u_AC4R()
//
// Purpose:    Copy Image
//
// Returns:
//    ippStsNoErr       no error
//    ippStsNullPtrErr  NULL==pSrc || NULL==pDst
//    ippStsStepErr     srcSTep<=0 || dstSTep <=0
//    ippStsSizeErr     dstRoiSize.width<1 || dstRoiSize.height<1
//
// Parameters:
//    pSrc           pointer to the source image ROI
//    srcStep        source scan line size (bytes)
//    pDst           pointer to the target image ROI
//    dstStep        target scan line size (bytes)
//    dstRoiSize     size of target ROI
//
*/
IPPAPI(IppStatus, ippiCopy_8u_C1R, (
               const Ipp8u* pSrc, int srcStep,
                     Ipp8u* pDst, int dstStep,
               IppiSize dstRoiSize))
IPPAPI(IppStatus, ippiCopy_8u_C3R, (
               const Ipp8u* pSrc, int srcStep,
                     Ipp8u* pDst, int dstStep,
               IppiSize dstRoiSize))


/* /////////////////////////////////////////////////////////////////////////////
//                  Arithmetic & Logical Functions
///////////////////////////////////////////////////////////////////////////// */

/* ////////////////////////////////////////////////////////////////////////////
//  Names:     ippiAddC,       ippiAdd
//             ippiSubC,       ippiSub
//             ippiMulC,       ippiMul
//             ippiMulCScale,  ippiMulScale
//             ippiSqr
//
//  Purpose:   Arithmetic Operations upon every element
//             of the source image ROI
//
//  Arguments:
//    pSrc                 pointer to the source ROI
//    pSrc1                pointer to the first source ROI
//    pSrc2                pointer to the second source ROI
//    pDst                 pointer to the destination ROI
//    srcStep,dstStep,     scan line size (bytes)
//    src1Step,src2Step    of the source/destination image
//    value                constant operand
//    roiSize              size of the image ROI
//    scaleFactor          scale factor value
//
//  Return:                Reason:
//    ippStsNullPtrErr     pointer(s) to the source/destination ROI is NULL
//    ippStsSizeErr        ROI sizes are negative or zero
//    ippStsNoErr          no problems
//
//  Note:
//    AddC(v,X,Y)    :  Y[n] = X[n] + v
//    Add(X,Y,Z)     :  Z[n] = Y[n] + X[n]
//
//    SubC(v,X,Y)    :  Y[n] = X[n] - v
//    Sub(X,Y,Z)     :  Z[n] = Y[n] - X[n]
//
//    MulC(v,X,Y)    :  Y[n] = X[n] * v
//    Mul(X,Y,Z)     :  Z[n] = Y[n] * X[n]
//
//    MulCScale(v,X,Y)  :  Y[n] = ( X[n] * v ) / IPP_MAX_8U
//    MulScale(X,Y,Z)   :  Z[n] = ( Y[n] * X[n] ) / IPP_MAX_8U
//
//    Sqr(X,Y,Z)     :  Y[n] = X[n] * X[n]
//
*/
IPPAPI(IppStatus, ippiAddC_8u_C1RSfs, (
                     const Ipp8u* pSrc, int srcStep, Ipp8u value,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize, int scaleFactor))
IPPAPI(IppStatus, ippiSubC_8u_C1RSfs, (
                     const Ipp8u* pSrc, int srcStep, Ipp8u value,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize, int scaleFactor))
IPPAPI(IppStatus, ippiMulC_8u_C1RSfs, (
                     const Ipp8u* pSrc, int srcStep, Ipp8u value,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize, int scaleFactor))
IPPAPI(IppStatus, ippiMulCScale_8u_C1R,(
                     const Ipp8u* pSrc, int srcStep, Ipp8u value,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))

IPPAPI(IppStatus, ippiAddC_8u_C3RSfs, (
                     const Ipp8u* pSrc, int srcStep, const Ipp8u value[3],
                           Ipp8u* pDst, int dstStep, IppiSize roiSize, int scaleFactor))
IPPAPI(IppStatus, ippiSubC_8u_C3RSfs, (
                     const Ipp8u* pSrc, int srcStep, const Ipp8u value[3],
                           Ipp8u* pDst, int dstStep, IppiSize roiSize, int scaleFactor))
IPPAPI(IppStatus, ippiMulC_8u_C3RSfs, (
                     const Ipp8u* pSrc, int srcStep, const Ipp8u value[3],
                           Ipp8u* pDst, int dstStep, IppiSize roiSize, int scaleFactor))
IPPAPI(IppStatus, ippiMulCScale_8u_C3R,(
                     const Ipp8u* pSrc, int srcStep, const Ipp8u value[3],
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))

IPPAPI(IppStatus, ippiAdd_8u_C1RSfs,  (
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize, int scaleFactor))
IPPAPI(IppStatus, ippiSub_8u_C1RSfs,  (
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize, int scaleFactor))
IPPAPI(IppStatus, ippiMul_8u_C1RSfs,  (
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst,  int dstStep, IppiSize roiSize, int scaleFactor))
IPPAPI(IppStatus, ippiMulScale_8u_C1R,(
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst,  int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiSqr_8u_C1RSfs, (
                     const Ipp8u* pSrc, int srcStep,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize, int scaleFactor))

IPPAPI(IppStatus, ippiAdd_8u_C3RSfs,  (
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize, int scaleFactor))
IPPAPI(IppStatus, ippiSub_8u_C3RSfs,  (
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst,  int dstStep, IppiSize roiSize, int scaleFactor))
IPPAPI(IppStatus, ippiMul_8u_C3RSfs,  (
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst,  int dstStep, IppiSize roiSize, int scaleFactor))
IPPAPI(IppStatus, ippiMulScale_8u_C3R,(
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst,  int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiSqr_8u_C3RSfs, (
                     const Ipp8u* pSrc, int srcStep,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize, int scaleFactor))

/* ////////////////////////////////////////////////////////////////////////////
//  Names:     ippiAndC, ippiAnd
//             ippiOrC,  ippiOr
//             ippiXorC, ippiXor
//                       ippiNot
//                       ippiRShiftC
//                       ippiLShiftC
//
//  Purpose:   Logical Operations upon every element of
//             the source image ROI
//
//  Arguments:
//    pSrc                 pointer to the source ROI
//    pSrc1                pointer to the first source ROI
//    pSrc2                pointer to the second source ROI
//    pDst                 pointer to the destination ROI
//    srcStep,dstStep,     scan line size (bytes)
//    src1Step,src2Step     of the source/destination image
//    value                constant operand
//    roiSize              size of the image ROI
//
//  Return:                Reason:
//    ippStsNullPtrErr        pointer(s) to the source/destination ROI is NULL
//    ippStsSizeErr        ROI sizes are negative or zero
//    ippStsNoErr             no problems
//
//  Note:
//    AndC(v,X,Y)    :  Y[n] = X[n] & v
//    And(X,Y,Z)     :  Z[n] = Y[n] & X[n]
//
//    OrC(v,X,Y)     :  Y[n] = X[n] | v
//    Or(X,Y,Z)      :  Z[n] = Y[n] | X[n]
//
//    XorC(v,X,Y)    :  Y[n] = X[n] ^ v
//    Xor(X,Y,Z)     :  Z[n] = Y[n] ^ X[n]
//
//    Xor(X,Y)       :  Y[n] = ~X[n]
//
//    RShiftC(X,Y,v)  :  Y[n] = X[n] >> v
//    LShiftC(X,Y,v)  :  Y[n] = X[n] << v
*/
IPPAPI(IppStatus, ippiAndC_8u_C1R, (
                     const Ipp8u* pSrc, int srcStep, Ipp8u value,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiOrC_8u_C1R,  (
                     const Ipp8u* pSrc, int srcStep, Ipp8u value,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiXorC_8u_C1R, (
                     const Ipp8u* pSrc, int srcStep, Ipp8u value,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))

IPPAPI(IppStatus, ippiAndC_8u_C3R, (
                     const Ipp8u* pSrc, int srcStep, const Ipp8u value[3],
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiOrC_8u_C3R,  (
                     const Ipp8u* pSrc, int srcStep, const Ipp8u value[3],
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiXorC_8u_C3R, (
                     const Ipp8u* pSrc, int srcStep, const Ipp8u value[3],
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))

IPPAPI(IppStatus, ippiAnd_8u_C1R, (
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiOr_8u_C1R,  (
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiXor_8u_C1R, (
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiNot_8u_C1R, (
                     const Ipp8u* pSrc, int srcStep,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiRShiftC_8u_C1R,(
                     const Ipp8u* pSrc, int srcStep, Ipp32u value,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiLShiftC_8u_C1R,(
                     const Ipp8u* pSrc, int srcStep, Ipp32u value,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))

IPPAPI(IppStatus, ippiAnd_8u_C3R, (
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiOr_8u_C3R,  (
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiXor_8u_C3R, (
                     const Ipp8u* pSrc1, int src1Step,
                     const Ipp8u* pSrc2, int src2Step,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiNot_8u_C3R, (
                     const Ipp8u* pSrc, int srcStep,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiRShiftC_8u_C3R,(
                     const Ipp8u* pSrc, int srcStep, const Ipp32u value[3],
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))
IPPAPI(IppStatus, ippiLShiftC_8u_C3R,(
                     const Ipp8u* pSrc, int srcStep, const Ipp32u value[3],
                           Ipp8u* pDst, int dstStep, IppiSize roiSize))


/*=============== Intel IPP Camera Interface API definition ================*/
IPPAPI(IppStatus, ippiResizeCscRotate_8u_C2R, (const Ipp8u *pSrc, 
            int srcStep, Ipp16u *pDst, int dstStep, IppiSize roiSize, 
            int scaleFactor, IppCameraInterpolation interpolation,
            IppCameraCsc colorConversion,IppCameraRotation rotation))

IPPAPI(IppStatus, ippiYCbCr422ToYCbCr420Rotate_8u_C2P3R, 
	   (const Ipp8u * pSrc, int srcStep, Ipp8u *pDst[3], int dstStep[3],
	   IppiSize roiSize, IppCameraRotation rotation))

IPPAPI(IppStatus, ippiYCbCr422ToYCbCr420Rotate_8u_P3R, 
	   (const Ipp8u *pSrc[3], int srcStep[3], Ipp8u *pDst[3], int dstStep[3],
		IppiSize roiSize, IppCameraRotation rotation))

IPPAPI(IppStatus, ippiYCbCr420RszCscRotRGB_8u_P3C3R,
	   (const Ipp8u *pSrc[3], int srcStep[3], IppiSize srcSize, void *pDst, 
		int dstStep, IppiSize dstSize, IppCameraCsc	colorConversion, 
		IppCameraInterpolation interpolation, IppCameraRotation rotation,
		int rcpRatiox, int rcpRatioy))

IPPAPI(IppStatus, ippiYCbCr422RszCscRotRGB_8u_P3C3R,
	   (const Ipp8u *pSrc[3], int srcStep[3], IppiSize srcSize, void *pDst, 
	   int dstStep, IppiSize dstSize, IppCameraCsc colorConversion, 
	   IppCameraInterpolation interpolation, IppCameraRotation rotation, 
	   int rcpRatiox, int rcpRatioy))

IPPAPI(IppStatus, ippiYCbCr420RszRot_8u_P3R,
	   (const Ipp8u *pSrc[3], int srcStep[3], IppiSize srcSize, Ipp8u *pDst[3], 
		int dstStep[3], IppiSize dstSize, IppCameraInterpolation interpolation,
		IppCameraRotation rotation,	int rcpRatiox, int rcpRatioy))

IPPAPI(IppStatus, ippiYCbCr422RszRot_8u_P3R, 
	   (const Ipp8u *pSrc[3], int srcStep[3], IppiSize srcSize, Ipp8u *pDst[3], 
	    int dstStep[3], IppiSize dstSize, IppCameraInterpolation interpolation, 
	    IppCameraRotation rotation, int rcpRatiox, int rcpRatioy))

/*===================Raw Data Processing=======================*/
IPPAPI (IppStatus, ippiInitAlloc_10RGGBtoYCbCr_RotRsz_P3R, 
		(const IppiCAMCfg *pCAMCfg, const IppiRawPixProcCfg_P3R  *pRPPCfg,
		IppiRawPixProcSpec_P3R **pRPPSpec))

IPPAPI (IppStatus, ippi10RGGBtoYCbCr_RotRsz_8u_P3R,
		(Ipp8u *pSrc, Ipp8u *pDst[3], IppiRawPixProcSpec_P3R  *pRPPSpec))

IPPAPI (IppStatus, ippiFree_10RGGBtoYCbCr_RotRsz_P3R,
		(IppiRawPixProcSpec_P3R *pRPPSpec))

/*=================== Auto Exposure Control ======================*/
IPPAPI (IppStatus, ippiInitAlloc_AE_10RGGB_8u_C1R, 
		(const IppiAEConfig* pAEConfig,  IppiAESpec_C1R** pAESpec))

IPPAPI (IppStatus, ippiFree_AE_10RGGB_8u_C1R, (IppiAESpec_C1R *pAESpec))

IPPAPI (IppStatus, ippiAE_10RGGB_8u_C1R, (const Ipp8u *pSrc, int srcStep, 
		IppiSize srcSize, Ipp16u *pGain, Ipp16u *pIntegrationTime, 
		IppiAESpec_C1R *pAESpec))

/*==================== Auto White Balance ========================*/
IPPAPI (IppStatus, ippiInitAlloc_AWB_10RGGB_8u_C1R, (IppiAWBCfg* pAWBCfg, IppiAWBSpec_C1R** pAWBSpec));

IPPAPI (IppStatus, ippiFree_AWB_10RGGB_8u_C1R, (IppiAWBSpec_C1R * pAWBSpec));

IPPAPI (IppStatus, ippiAWB_10RGGB_8u_C1R, (const Ipp8u *pSrc, int srcStep, IppiSize srcSize,
		Ipp16u *pRGain, Ipp16u *pGGain, Ipp16u *pBGain, IppiAWBSpec_C1R * pAWBSpec));

/*==================== Auto Focusing ==============================*/
IPPAPI (IppStatus, ippiAF, (const Ipp8u *pSrc,int srcStep,IppAFColorFormat srcFmt,IppiSize roiSize,
						  IppCameraNoiseReduction pNoiseReduction,IppiAFResult *pAFResults,IppiAFSpec_C1R *pAFSpec))

IPPAPI (IppStatus, ippiInitAlloc_AF, (IppAFConfig *pConfig, IppiAFSpec_C1R ** pAFSpec))

IPPAPI (IppStatus,ippiAFSharp, (const Ipp8u *pSrc,int srcStep,IppAFColorFormat srcFmt,
							   IppiSize roiSize,IppCameraNoiseReduction pNoiseReduction,Ipp32s *pSharpness, void *pReserved))

IPPAPI (IppStatus, ippiFree_AF, (IppiAFSpec_C1R** pAFSpec))

/*==================== Image Processing Functions ==============================*/
IPPAPI(IppStatus, ippiRGBToYUV420_8u_C3P3R,(
                     const Ipp8u* pSrc, int srcStep ,
                           Ipp8u* pDst[3], int dstStep[3], IppiSize roiSize))

#ifdef __cplusplus
}
#endif

#endif /* __IPPIP_H__ */
/* ////////////////////////// End of file "ippIP.h" ////////////////////////// */
