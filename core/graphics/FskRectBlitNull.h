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
#if !COMPILER_CAN_GENERATE_CONST_PROC_POINTERS

/********************************************************************************
 ********************************************************************************
 **	NULLify procs
 **
 ** It would be nice to generate these all automatically in the above files.
 ** The following DOES work in MetroWerks and Visual Studio 8:
 **		static const FskRectTransferProc FskName3(FskCopy,SrcPixelKind,YUV420)	= NULL;
 **		const FskRectTransferProc FskName3(FskCopy,SrcPixelKind,YUV420)			= NULL;
 ** whereas XCode and Visual Studio7 complain that the resultant variable is not constant,
 ** in spite of the fact that the keyword "const" is used explicitly.
 ** These are invoked in the files "FskRectBlitUnity.c" and "FskRectBlitToYUV.c" for these compilers.
 **
 ** Every compiler complains about these two:
 **		static void FskName3(FskCopy,SrcPixelKind,YUV420)(const FskRectBlitParams *params)	= NULL;
 **		#define FskCopy##SrcPixelKind##YUV420									NULL
 ** Until all compilers can generate thee code automatically, we have the following:
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * NULLify all procs with non-YUV420 sources to YUV420 destinations
 ********************************************************************************/

/* 32 ARGB -> YUV420 */
#define FskCopy32ARGBYUV420							NULL
#define FskBilinearCopy32ARGBYUV420					NULL
#define FskBlend32ARGBYUV420						NULL
#define FskBilinearBlend32ARGBYUV420				NULL
#define FskAlpha32ARGBYUV420						NULL
#define FskBilinearAlpha32ARGBYUV420				NULL
#define FskAlphaBlend32ARGBYUV420					NULL
#define FskBilinearAlphaBlend32ARGBYUV420			NULL
#define FskTintCopy32ARGBYUV420						NULL
#define FskBilinearTintCopy32ARGBYUV420				NULL

/* 32 BGRA -> YUV420 */
#define FskCopy32BGRAYUV420							NULL
#define FskBilinearCopy32BGRAYUV420					NULL
#define FskBlend32BGRAYUV420						NULL
#define FskBilinearBlend32BGRAYUV420				NULL
#define FskAlpha32BGRAYUV420						NULL
#define FskBilinearAlpha32BGRAYUV420				NULL
#define FskAlphaBlend32BGRAYUV420					NULL
#define FskBilinearAlphaBlend32BGRAYUV420			NULL
#define FskTintCopy32BGRAYUV420						NULL
#define FskBilinearTintCopy32BGRAYUV420				NULL

/* 32 RGBA -> YUV420 */
#define FskCopy32RGBAYUV420							NULL
#define FskBilinearCopy32RGBAYUV420					NULL
#define FskBlend32RGBAYUV420						NULL
#define FskBilinearBlend32RGBAYUV420				NULL
#define FskAlpha32RGBAYUV420						NULL
#define FskBilinearAlpha32RGBAYUV420				NULL
#define FskAlphaBlend32RGBAYUV420					NULL
#define FskBilinearAlphaBlend32RGBAYUV420			NULL
#define FskTintCopy32RGBAYUV420						NULL
#define FskBilinearTintCopy32RGBAYUV420				NULL

/* 32 ABGR -> YUV420 */
#define FskCopy32ABGRYUV420							NULL
#define FskBilinearCopy32ABGRYUV420					NULL
#define FskBlend32ABGRYUV420						NULL
#define FskBilinearBlend32ABGRYUV420				NULL
#define FskAlpha32ABGRYUV420						NULL
#define FskBilinearAlpha32ABGRYUV420				NULL
#define FskAlphaBlend32ABGRYUV420					NULL
#define FskBilinearAlphaBlend32ABGRYUV420			NULL
#define FskTintCopy32ABGRYUV420						NULL
#define FskBilinearTintCopy32ABGRYUV420				NULL

/* 24 BGR -> YUV420 */
#define FskCopy24BGRYUV420							NULL
#define FskBilinearCopy24BGRYUV420					NULL
#define FskBlend24BGRYUV420							NULL
#define FskBilinearBlend24BGRYUV420					NULL
#define FskAlpha24BGRYUV420							NULL
#define FskBilinearAlpha24BGRYUV420					NULL
#define FskAlphaBlend24BGRYUV420					NULL
#define FskBilinearAlphaBlend24BGRYUV420			NULL
#define FskTintCopy24BGRYUV420						NULL
#define FskBilinearTintCopy24BGRYUV420				NULL

/* 24 RGB -> YUV420 */
#define FskCopy24RGBYUV420							NULL
#define FskBilinearCopy24RGBYUV420					NULL
#define FskBlend24RGBYUV420							NULL
#define FskBilinearBlend24RGBYUV420					NULL
#define FskAlpha24RGBYUV420							NULL
#define FskBilinearAlpha24RGBYUV420					NULL
#define FskAlphaBlend24RGBYUV420					NULL
#define FskBilinearAlphaBlend24RGBYUV420			NULL
#define FskTintCopy24RGBYUV420						NULL
#define FskBilinearTintCopy24RGBYUV420				NULL

/* 16 RGB 565 SE -> YUV420 */
#define FskCopy16RGB565SEYUV420						NULL
#define FskBilinearCopy16RGB565SEYUV420				NULL
#define FskBlend16RGB565SEYUV420					NULL
#define FskBilinearBlend16RGB565SEYUV420			NULL
#define FskAlpha16RGB565SEYUV420					NULL
#define FskBilinearAlpha16RGB565SEYUV420			NULL
#define FskAlphaBlend16RGB565SEYUV420				NULL
#define FskBilinearAlphaBlend16RGB565SEYUV420		NULL
#define FskTintCopy16RGB565SEYUV420					NULL
#define FskBilinearTintCopy16RGB565SEYUV420			NULL

/* 16 RGB 565 DE -> YUV420 */
#define FskCopy16RGB565DEYUV420						NULL
#define FskBilinearCopy16RGB565DEYUV420				NULL
#define FskBlend16RGB565DEYUV420					NULL
#define FskBilinearBlend16RGB565DEYUV420			NULL
#define FskAlpha16RGB565DEYUV420					NULL
#define FskBilinearAlpha16RGB565DEYUV420			NULL
#define FskAlphaBlend16RGB565DEYUV420				NULL
#define FskBilinearAlphaBlend16RGB565DEYUV420		NULL
#define FskTintCopy16RGB565DEYUV420					NULL
#define FskBilinearTintCopy16RGB565DEYUV420			NULL

/* 16 BGR 565 SE -> YUV420 */
#define FskCopy16BGR565SEYUV420						NULL
#define FskBilinearCopy16BGR565SEYUV420				NULL
#define FskBlend16BGR565SEYUV420					NULL
#define FskBilinearBlend16BGR565SEYUV420			NULL
#define FskAlpha16BGR565SEYUV420					NULL
#define FskBilinearAlpha16BGR565SEYUV420			NULL
#define FskAlphaBlend16BGR565SEYUV420				NULL
#define FskBilinearAlphaBlend16BGR565SEYUV420		NULL
#define FskTintCopy16BGR565SEYUV420					NULL
#define FskBilinearTintCopy16BGR565SEYUV420			NULL

/* 16 BGR 565 DE -> YUV420 */
#define FskCopy16BGR565DEYUV420						NULL
#define FskBilinearCopy16BGR565DEYUV420				NULL
#define FskBlend16BGR565DEYUV420					NULL
#define FskBilinearBlend16BGR565DEYUV420			NULL
#define FskAlpha16BGR565DEYUV420					NULL
#define FskBilinearAlpha16BGR565DEYUV420			NULL
#define FskAlphaBlend16BGR565DEYUV420				NULL
#define FskBilinearAlphaBlend16BGR565DEYUV420		NULL
#define FskTintCopy16BGR565DEYUV420					NULL
#define FskBilinearTintCopy16BGR565DEYUV420			NULL

/* 16 RGB 5515 SE -> YUV420 */
#define FskCopy16RGB5515SEYUV420					NULL
#define FskBilinearCopy16RGB5515SEYUV420			NULL
#define FskBlend16RGB5515SEYUV420					NULL
#define FskBilinearBlend16RGB5515SEYUV420			NULL
#define FskAlpha16RGB5515SEYUV420					NULL
#define FskBilinearAlpha16RGB5515SEYUV420			NULL
#define FskAlphaBlend16RGB5515SEYUV420				NULL
#define FskBilinearAlphaBlend16RGB5515SEYUV420		NULL
#define FskTintCopy16RGB5515SEYUV420				NULL
#define FskBilinearTintCopy16RGB5515SEYUV420		NULL

/* 16 RGB 5515 DE -> YUV420 */
#define FskCopy16RGB5515DEYUV420					NULL
#define FskBilinearCopy16RGB5515DEYUV420			NULL
#define FskBlend16RGB5515DEYUV420					NULL
#define FskBilinearBlend16RGB5515DEYUV420			NULL
#define FskAlpha16RGB5515DEYUV420					NULL
#define FskBilinearAlpha16RGB5515DEYUV420			NULL
#define FskAlphaBlend16RGB5515DEYUV420				NULL
#define FskBilinearAlphaBlend16RGB5515DEYUV420		NULL
#define FskTintCopy16RGB5515DEYUV420				NULL
#define FskBilinearTintCopy16RGB5515DEYUV420		NULL

/* 16 BGR 5515 SE -> YUV420 */
#define FskCopy16BGR5515SEYUV420					NULL
#define FskBilinearCopy16BGR5515SEYUV420			NULL
#define FskBlend16BGR5515SEYUV420					NULL
#define FskBilinearBlend16BGR5515SEYUV420			NULL
#define FskAlpha16BGR5515SEYUV420					NULL
#define FskBilinearAlpha16BGR5515SEYUV420			NULL
#define FskAlphaBlend16BGR5515SEYUV420				NULL
#define FskBilinearAlphaBlend16BGR5515SEYUV420		NULL
#define FskTintCopy16BGR5515SEYUV420				NULL
#define FskBilinearTintCopy16BGR5515SEYUV420		NULL

/* 16 BGR 5515 DE -> YUV420 */
#define FskCopy16BGR5515DEYUV420					NULL
#define FskBilinearCopy16BGR5515DEYUV420			NULL
#define FskBlend16BGR5515DEYUV420					NULL
#define FskBilinearBlend16BGR5515DEYUV420			NULL
#define FskAlpha16BGR5515DEYUV420					NULL
#define FskBilinearAlpha16BGR5515DEYUV420			NULL
#define FskAlphaBlend16BGR5515DEYUV420				NULL
#define FskBilinearAlphaBlend16BGR5515DEYUV420		NULL
#define FskTintCopy16BGR5515DEYUV420				NULL
#define FskBilinearTintCopy16BGR5515DEYUV420		NULL

/* 16 AG -> YUV420 */
#define FskCopy16AGYUV420							NULL
#define FskBilinearCopy16AGYUV420					NULL
#define FskBlend16AGYUV420							NULL
#define FskBilinearBlend16AGYUV420					NULL
#define FskAlpha16AGYUV420							NULL
#define FskBilinearAlpha16AGYUV420					NULL
#define FskAlphaBlend16AGYUV420						NULL
#define FskBilinearAlphaBlend16AGYUV420				NULL
#define FskTintCopy16AGYUV420						NULL
#define FskBilinearTintCopy16AGYUV420				NULL

/* 16 GA -> YUV420 */
#define FskCopy16GAYUV420							NULL
#define FskBilinearCopy16GAYUV420					NULL
#define FskBlend16GAYUV420							NULL
#define FskBilinearBlend16GAYUV420					NULL
#define FskAlpha16GAYUV420							NULL
#define FskBilinearAlpha16GAYUV420					NULL
#define FskAlphaBlend16GAYUV420						NULL
#define FskBilinearAlphaBlend16GAYUV420				NULL
#define FskTintCopy16GAYUV420						NULL
#define FskBilinearTintCopy16GAYUV420				NULL

/* 8G -> YUV420 */
#define FskCopy8GYUV420								NULL
#define FskBilinearCopy8GYUV420						NULL
#define FskBlend8GYUV420							NULL
#define FskBilinearBlend8GYUV420					NULL
#define FskAlpha8GYUV420							NULL
#define FskBilinearAlpha8GYUV420					NULL
#define FskAlphaBlend8GYUV420						NULL
#define FskBilinearAlphaBlend8GYUV420				NULL
#define FskTintCopy8GYUV420							NULL
#define FskBilinearTintCopy8GYUV420					NULL

/* 16 RGBA 4444 SE -> YUV420 */
#define FskCopy16RGBA4444SEYUV420					NULL
#define FskBilinearCopy16RGBA4444SEYUV420			NULL
#define FskBlend16RGBA4444SEYUV420					NULL
#define FskBilinearBlend16RGBA4444SEYUV420			NULL
#define FskAlpha16RGBA4444SEYUV420					NULL
#define FskBilinearAlpha16RGBA4444SEYUV420			NULL
#define FskAlphaBlend16RGBA4444SEYUV420				NULL
#define FskBilinearAlphaBlend16RGBA4444SEYUV420		NULL
#define FskTintCopy16RGBA4444SEYUV420				NULL
#define FskBilinearTintCopy16RGBA4444SEYUV420		NULL

/* 16 RGBA 4444 DE -> YUV420 */
#define FskCopy16RGBA4444DEYUV420					NULL
#define FskBilinearCopy16RGBA4444DEYUV420			NULL
#define FskBlend16RGBA4444DEYUV420					NULL
#define FskBilinearBlend16RGBA4444DEYUV420			NULL
#define FskAlpha16RGBA4444DEYUV420					NULL
#define FskBilinearAlpha16RGBA4444DEYUV420			NULL
#define FskAlphaBlend16RGBA4444DEYUV420				NULL
#define FskBilinearAlphaBlend16RGBA4444DEYUV420		NULL
#define FskTintCopy16RGBA4444DEYUV420				NULL
#define FskBilinearTintCopy16RGBA4444DEYUV420		NULL

/* 32A 16 RGB  SE -> YUV420 */
#define FskCopy32A16RGB565SEYUV420					NULL
#define FskBilinearCopy32A16RGB565SEYUV420			NULL
#define FskBlend32A16RGB565SEYUV420					NULL
#define FskBilinearBlend32A16RGB565SEYUV420			NULL
#define FskAlpha32A16RGB565SEYUV420					NULL
#define FskBilinearAlpha32A16RGB565SEYUV420			NULL
#define FskAlphaBlend32A16RGB565SEYUV420			NULL
#define FskBilinearAlphaBlend32A16RGB565SEYUV420	NULL
#define FskTintCopy32A16RGB565SEYUV420				NULL
#define FskBilinearTintCopy32A16RGB565SEYUV420		NULL

/* 32A 16 RGB  DE -> YUV420 */
#define FskCopy32A16RGB5654444DEYUV420					NULL
#define FskBilinearCopy32A16RGB5654444DEYUV420			NULL
#define FskBlend32A16RGB5654444DEYUV420					NULL
#define FskBilinearBlend32A16RGB5654444DEYUV420			NULL
#define FskAlpha32A16RGB5654444DEYUV420					NULL
#define FskBilinearAlpha32A16RGB5654444DEYUV420			NULL
#define FskAlphaBlend32A16RGB5654444DEYUV420			NULL
#define FskBilinearAlphaBlend32A16RGB5654444DEYUV420	NULL
#define FskTintCopy32A16RGB5654444DEYUV420				NULL
#define FskBilinearTintCopy32A16RGB5654444DEYUV420		NULL



/********************************************************************************
 * Skirt around alpha mode for non-32 bit sources.
 ********************************************************************************/

#define FskUnityAlpha24BGR32ARGB			NULL
#define FskUnityAlpha24BGR32BGRA			NULL
#define FskUnityAlpha24BGR32RGBA			NULL
#define FskUnityAlpha24BGR32ABGR			NULL
#define FskUnityAlpha24BGR24BGR				NULL
#define FskUnityAlpha24BGR24RGB				NULL
#define FskUnityAlpha24BGR16RGB565SE		NULL
#define FskUnityAlpha24BGR16RGB565DE		NULL
#define FskUnityAlpha24BGR16BGR565SE		NULL
#define FskUnityAlpha24BGR16BGR565DE		NULL
#define FskUnityAlpha24BGR16RGB5515SE		NULL
#define FskUnityAlpha24BGR16RGB5515DE		NULL
#define FskUnityAlpha24BGR16BGR5515SE		NULL
#define FskUnityAlpha24BGR16BGR5515DE		NULL
#define FskUnityAlpha24BGR16RGBA4444SE		NULL
#define FskUnityAlpha24BGR8G				NULL

#define FskUnityAlpha24RGB32ARGB			NULL
#define FskUnityAlpha24RGB32BGRA			NULL
#define FskUnityAlpha24RGB32RGBA			NULL
#define FskUnityAlpha24RGB32ABGR			NULL
#define FskUnityAlpha24RGB24BGR				NULL
#define FskUnityAlpha24RGB24RGB				NULL
#define FskUnityAlpha24RGB16RGB565SE		NULL
#define FskUnityAlpha24RGB16RGB565DE		NULL
#define FskUnityAlpha24RGB16BGR565SE		NULL
#define FskUnityAlpha24RGB16BGR565DE		NULL
#define FskUnityAlpha24RGB16RGB5515SE		NULL
#define FskUnityAlpha24RGB16RGB5515DE		NULL
#define FskUnityAlpha24RGB16BGR5515SE		NULL
#define FskUnityAlpha24RGB16BGR5515DE		NULL
#define FskUnityAlpha24RGB8G				NULL

#define FskUnityAlpha16RGB565SE32ARGB		NULL
#define FskUnityAlpha16RGB565SE32BGRA		NULL
#define FskUnityAlpha16RGB565SE32RGBA		NULL
#define FskUnityAlpha16RGB565SE32ABGR		NULL
#define FskUnityAlpha16RGB565SE24BGR		NULL
#define FskUnityAlpha16RGB565SE24RGB		NULL
#define FskUnityAlpha16RGB565SE16RGB565SE	NULL
#define FskUnityAlpha16RGB565SE16RGB565DE	NULL
#define FskUnityAlpha16RGB565SE16BGR565SE	NULL
#define FskUnityAlpha16RGB565SE16BGR565DE	NULL
#define FskUnityAlpha16RGB565SE16RGB5515SE	NULL
#define FskUnityAlpha16RGB565SE16RGB5515DE	NULL
#define FskUnityAlpha16RGB565SE16BGR5515SE	NULL
#define FskUnityAlpha16RGB565SE16BGR5515DE	NULL
#define FskUnityAlpha16RGB565SE16RGBA4444SE	NULL
#define FskUnityAlpha16RGB565SE8G			NULL

#define FskUnityAlpha16RGB565DE32ARGB		NULL
#define FskUnityAlpha16RGB565DE32BGRA		NULL
#define FskUnityAlpha16RGB565DE32RGBA		NULL
#define FskUnityAlpha16RGB565DE32ABGR		NULL
#define FskUnityAlpha16RGB565DE24BGR		NULL
#define FskUnityAlpha16RGB565DE24RGB		NULL
#define FskUnityAlpha16RGB565DE16RGB565SE	NULL
#define FskUnityAlpha16RGB565DE16RGB565DE	NULL
#define FskUnityAlpha16RGB565DE16BGR565SE	NULL
#define FskUnityAlpha16RGB565DE16BGR565DE	NULL
#define FskUnityAlpha16RGB565DE16RGB5515SE	NULL
#define FskUnityAlpha16RGB565DE16RGB5515DE	NULL
#define FskUnityAlpha16RGB565DE16BGR5515SE	NULL
#define FskUnityAlpha16RGB565DE16BGR5515DE	NULL
#define FskUnityAlpha16RGB565DE16RGBA4444SE	NULL
#define FskUnityAlpha16RGB565DE8G			NULL

#define FskUnityAlpha16BGR565SE32ARGB		NULL
#define FskUnityAlpha16BGR565SE32BGRA		NULL
#define FskUnityAlpha16BGR565SE32RGBA		NULL
#define FskUnityAlpha16BGR565SE32ABGR		NULL
#define FskUnityAlpha16BGR565SE24BGR		NULL
#define FskUnityAlpha16BGR565SE24RGB		NULL
#define FskUnityAlpha16BGR565SE16RGB565SE	NULL
#define FskUnityAlpha16BGR565SE16RGB565DE	NULL
#define FskUnityAlpha16BGR565SE16BGR565SE	NULL
#define FskUnityAlpha16BGR565SE16BGR565DE	NULL
#define FskUnityAlpha16BGR565SE16RGB5515SE	NULL
#define FskUnityAlpha16BGR565SE16RGB5515DE	NULL
#define FskUnityAlpha16BGR565SE16BGR5515SE	NULL
#define FskUnityAlpha16BGR565SE16BGR5515DE	NULL
#define FskUnityAlpha16BGR565SE8G			NULL

#define FskUnityAlpha16BGR565DE32ARGB		NULL
#define FskUnityAlpha16BGR565DE32BGRA		NULL
#define FskUnityAlpha16BGR565DE32RGBA		NULL
#define FskUnityAlpha16BGR565DE32ABGR		NULL
#define FskUnityAlpha16BGR565DE24BGR		NULL
#define FskUnityAlpha16BGR565DE24RGB		NULL
#define FskUnityAlpha16BGR565DE16RGB565SE	NULL
#define FskUnityAlpha16BGR565DE16RGB565DE	NULL
#define FskUnityAlpha16BGR565DE16BGR565SE	NULL
#define FskUnityAlpha16BGR565DE16BGR565DE	NULL
#define FskUnityAlpha16BGR565DE16RGB5515SE	NULL
#define FskUnityAlpha16BGR565DE16RGB5515DE	NULL
#define FskUnityAlpha16BGR565DE16BGR5515SE	NULL
#define FskUnityAlpha16BGR565DE16BGR5515DE	NULL
#define FskUnityAlpha16BGR565DE8G			NULL

#define FskUnityAlpha16RGB5515SE32ARGB		NULL
#define FskUnityAlpha16RGB5515SE32BGRA		NULL
#define FskUnityAlpha16RGB5515SE32RGBA		NULL
#define FskUnityAlpha16RGB5515SE32ABGR		NULL
#define FskUnityAlpha16RGB5515SE24BGR		NULL
#define FskUnityAlpha16RGB5515SE24RGB		NULL
#define FskUnityAlpha16RGB5515SE16RGB565SE	NULL
#define FskUnityAlpha16RGB5515SE16RGB565DE	NULL
#define FskUnityAlpha16RGB5515SE16BGR565SE	NULL
#define FskUnityAlpha16RGB5515SE16BGR565DE	NULL
#define FskUnityAlpha16RGB5515SE16RGB5515SE	NULL
#define FskUnityAlpha16RGB5515SE16RGB5515DE	NULL
#define FskUnityAlpha16RGB5515SE16BGR5515SE	NULL
#define FskUnityAlpha16RGB5515SE16BGR5515DE	NULL
#define FskUnityAlpha16RGB5515SE8G			NULL

#define FskUnityAlpha16RGB5515DE32ARGB		NULL
#define FskUnityAlpha16RGB5515DE32BGRA		NULL
#define FskUnityAlpha16RGB5515DE32RGBA		NULL
#define FskUnityAlpha16RGB5515DE32ABGR		NULL
#define FskUnityAlpha16RGB5515DE24BGR		NULL
#define FskUnityAlpha16RGB5515DE24RGB		NULL
#define FskUnityAlpha16RGB5515DE16RGB565SE	NULL
#define FskUnityAlpha16RGB5515DE16RGB565DE	NULL
#define FskUnityAlpha16RGB5515DE16BGR565SE	NULL
#define FskUnityAlpha16RGB5515DE16BGR565DE	NULL
#define FskUnityAlpha16RGB5515DE16RGB5515SE	NULL
#define FskUnityAlpha16RGB5515DE16RGB5515DE	NULL
#define FskUnityAlpha16RGB5515DE16BGR5515SE	NULL
#define FskUnityAlpha16RGB5515DE16BGR5515DE	NULL
#define FskUnityAlpha16RGB5515DE8G			NULL

#define FskUnityAlpha16BGR5515SE32ARGB		NULL
#define FskUnityAlpha16BGR5515SE32BGRA		NULL
#define FskUnityAlpha16BGR5515SE32RGBA		NULL
#define FskUnityAlpha16BGR5515SE32ABGR		NULL
#define FskUnityAlpha16BGR5515SE24BGR		NULL
#define FskUnityAlpha16BGR5515SE24RGB		NULL
#define FskUnityAlpha16BGR5515SE16RGB565SE	NULL
#define FskUnityAlpha16BGR5515SE16RGB565DE	NULL
#define FskUnityAlpha16BGR5515SE16BGR565SE	NULL
#define FskUnityAlpha16BGR5515SE16BGR565DE	NULL
#define FskUnityAlpha16BGR5515SE16RGB5515SE	NULL
#define FskUnityAlpha16BGR5515SE16RGB5515DE	NULL
#define FskUnityAlpha16BGR5515SE16BGR5515SE	NULL
#define FskUnityAlpha16BGR5515SE16BGR5515DE	NULL
#define FskUnityAlpha16BGR5515SE8G			NULL

#define FskUnityAlpha16BGR5515DE32ARGB		NULL
#define FskUnityAlpha16BGR5515DE32BGRA		NULL
#define FskUnityAlpha16BGR5515DE32RGBA		NULL
#define FskUnityAlpha16BGR5515DE32ABGR		NULL
#define FskUnityAlpha16BGR5515DE24BGR		NULL
#define FskUnityAlpha16BGR5515DE24RGB		NULL
#define FskUnityAlpha16BGR5515DE16RGB565SE	NULL
#define FskUnityAlpha16BGR5515DE16RGB565DE	NULL
#define FskUnityAlpha16BGR5515DE16BGR565SE	NULL
#define FskUnityAlpha16BGR5515DE16BGR565DE	NULL
#define FskUnityAlpha16BGR5515DE16RGB5515SE	NULL
#define FskUnityAlpha16BGR5515DE16RGB5515DE	NULL
#define FskUnityAlpha16BGR5515DE16BGR5515SE	NULL
#define FskUnityAlpha16BGR5515DE16BGR5515DE	NULL
#define FskUnityAlpha16BGR5515DE8G			NULL

#define FskUnityAlpha8G32ARGB				NULL
#define FskUnityAlpha8G32BGRA				NULL
#define FskUnityAlpha8G32RGBA				NULL
#define FskUnityAlpha8G32ABGR				NULL
#define FskUnityAlpha8G24BGR				NULL
#define FskUnityAlpha8G24RGB				NULL
#define FskUnityAlpha8G16RGB565SE			NULL
#define FskUnityAlpha8G16RGB565DE			NULL
#define FskUnityAlpha8G16BGR565SE			NULL
#define FskUnityAlpha8G16BGR565DE			NULL
#define FskUnityAlpha8G16RGB5515SE			NULL
#define FskUnityAlpha8G16RGB5515DE			NULL
#define FskUnityAlpha8G16BGR5515SE			NULL
#define FskUnityAlpha8G16BGR5515DE			NULL
#define FskUnityAlpha8G16RGBA4444SE			NULL
#define FskUnityAlpha8G8G					NULL

#define FskUnityAlphaYUV42032ARGB			NULL
#define FskUnityAlphaYUV42032BGRA			NULL
#define FskUnityAlphaYUV42032RGBA			NULL
#define FskUnityAlphaYUV42032ABGR			NULL
#define FskUnityAlphaYUV42024BGR			NULL
#define FskUnityAlphaYUV42024RGB			NULL
#define FskUnityAlphaYUV42016RGB565SE		NULL
#define FskUnityAlphaYUV42016RGB565DE		NULL
#define FskUnityAlphaYUV42016BGR565SE		NULL
#define FskUnityAlphaYUV42016BGR565DE		NULL
#define FskUnityAlphaYUV42016RGB5515SE		NULL
#define FskUnityAlphaYUV42016RGB5515DE		NULL
#define FskUnityAlphaYUV42016BGR5515SE		NULL
#define FskUnityAlphaYUV42016BGR5515DE		NULL
#define FskUnityAlphaYUV42016RGBA4444SE		NULL
#define FskUnityAlphaYUV4208G				NULL

#define FskUnityCopyYUV42032ARGB			NULL
#define FskUnityCopyYUV42032BGRA			NULL
#define FskUnityCopyYUV42032RGBA			NULL
#define FskUnityCopyYUV42032ABGR			NULL
#define FskUnityCopyYUV42024BGR				NULL
#define FskUnityCopyYUV42024RGB				NULL
#define FskUnityCopyYUV42016RGB565SE		NULL
#define FskUnityCopyYUV42016RGB565DE		NULL
#define FskUnityCopyYUV42016BGR565SE		NULL
#define FskUnityCopyYUV42016BGR565DE		NULL
#define FskUnityCopyYUV42016RGB5515SE		NULL
#define FskUnityCopyYUV42016RGB5515DE		NULL
#define FskUnityCopyYUV42016BGR5515SE		NULL
#define FskUnityCopyYUV42016BGR5515DE		NULL
#define FskUnityCopyYUV42016RGBA4444SE		NULL
#define FskUnityCopyYUV4208G				NULL

#define FskCopyYUV420i32ARGB					NULL
#define FskBilinearCopyYUV420i32ARGB			NULL
#define FskBlendYUV420i32ARGB					NULL
#define FskBilinearBlendYUV420i32ARGB			NULL
#define FskAlphaYUV420i32ARGB					NULL
#define FskBilinearAlphaYUV420i32ARGB			NULL
#define FskAlphaBlendYUV420i32ARGB				NULL
#define FskBilinearAlphaBlendYUV420i32ARGB  	NULL
#define FskTintCopyYUV420i32ARGB				NULL
#define FskBilinearTintCopyYUV420i32ARGB    	NULL
#define FskUnityCopyYUV420i32ARGB				NULL
#define FskUnityAlphaYUV420i32ARGB				NULL

#define FskCopyYUV420i16RGB565SE				NULL
#define FskBilinearCopyYUV420i16RGB565SE		NULL
#define FskBlendYUV420i16RGB565SE				NULL
#define FskBilinearBlendYUV420i16RGB565SE		NULL
#define FskAlphaYUV420i16RGB565SE				NULL
#define FskBilinearAlphaYUV420i16RGB565SE		NULL
#define FskAlphaBlendYUV420i16RGB565SE			NULL
#define FskBilinearAlphaBlendYUV420i16RGB565SE  NULL
#define FskTintCopyYUV420i16RGB565SE			NULL
#define FskBilinearTintCopyYUV420i16RGB565SE    NULL
#define FskUnityCopyYUV420i16RGB565SE			NULL
#define FskUnityAlphaYUV420i16RGB565SE			NULL

#define FskCopyYUV420i32BGRA					NULL
#define FskBilinearCopyYUV420i32BGRA			NULL
#define FskBlendYUV420i32BGRA					NULL
#define FskBilinearBlendYUV420i32BGRA			NULL
#define FskAlphaYUV420i32BGRA					NULL
#define FskBilinearAlphaYUV420i32BGRA			NULL
#define FskAlphaBlendYUV420i32BGRA				NULL
#define FskBilinearAlphaBlendYUV420i32BGRA  	NULL
#define FskTintCopyYUV420i32BGRA				NULL
#define FskBilinearTintCopyYUV420i32BGRA    	NULL
#define FskUnityCopyYUV420i32BGRA				NULL
#define FskUnityAlphaYUV420i32BGRA				NULL

#define FskCopyYUV420i32RGBA					NULL
#define FskBilinearCopyYUV420i32RGBA			NULL
#define FskBlendYUV420i32RGBA					NULL
#define FskBilinearBlendYUV420i32RGBA			NULL
#define FskAlphaYUV420i32RGBA					NULL
#define FskBilinearAlphaYUV420i32RGBA			NULL
#define FskAlphaBlendYUV420i32RGBA				NULL
#define FskBilinearAlphaBlendYUV420i32RGBA  	NULL
#define FskTintCopyYUV420i32RGBA				NULL
#define FskBilinearTintCopyYUV420i32RGBA    	NULL
#define FskUnityCopyYUV420i32RGBA				NULL
#define FskUnityAlphaYUV420i32RGBA				NULL

#define FskCopyYUV420i32ABGR					NULL
#define FskBilinearCopyYUV420i32ABGR			NULL
#define FskBlendYUV420i32ABGR					NULL
#define FskBilinearBlendYUV420i32ABGR			NULL
#define FskAlphaYUV420i32ABGR					NULL
#define FskBilinearAlphaYUV420i32ABGR			NULL
#define FskAlphaBlendYUV420i32ABGR				NULL
#define FskBilinearAlphaBlendYUV420i32ABGR  	NULL
#define FskTintCopyYUV420i32ABGR				NULL
#define FskBilinearTintCopyYUV420i32ABGR    	NULL
#define FskUnityCopyYUV420i32ABGR				NULL
#define FskUnityAlphaYUV420i32ABGR				NULL

#define FskCopyYUV420i32A16RGB565SE					NULL
#define FskBilinearCopyYUV420i32A16RGB565SE			NULL
#define FskBlendYUV420i32A16RGB565SE				NULL
#define FskBilinearBlendYUV420i32A16RGB565SE		NULL
#define FskAlphaYUV420i32A16RGB565SE				NULL
#define FskBilinearAlphaYUV420i32A16RGB565SE		NULL
#define FskAlphaBlendYUV420i32A16RGB565SE			NULL
#define FskBilinearAlphaBlendYUV420i32A16RGB565SE  	NULL
#define FskTintCopyYUV420i32A16RGB565SE				NULL
#define FskBilinearTintCopyYUV420i32A16RGB565SE    	NULL
#define FskUnityCopyYUV420i32A16RGB565SE			NULL
#define FskUnityAlphaYUV420i32A16RGB565SE			NULL

#define FskCopyYUV420i24BGR						NULL
#define FskBilinearCopyYUV420i24BGR				NULL
#define FskBlendYUV420i24BGR					NULL
#define FskBilinearBlendYUV420i24BGR			NULL
#define FskAlphaYUV420i24BGR					NULL
#define FskBilinearAlphaYUV420i24BGR			NULL
#define FskAlphaBlendYUV420i24BGR				NULL
#define FskBilinearAlphaBlendYUV420i24BGR  		NULL
#define FskTintCopyYUV420i24BGR					NULL
#define FskBilinearTintCopyYUV420i24BGR    		NULL
#define FskUnityCopyYUV420i24BGR				NULL
#define FskUnityAlphaYUV420i24BGR				NULL

#define FskCopyYUV420i8G						NULL
#define FskBilinearCopyYUV420i8G				NULL
#define FskBlendYUV420i8G						NULL
#define FskBilinearBlendYUV420i8G				NULL
#define FskAlphaYUV420i8G						NULL
#define FskBilinearAlphaYUV420i8G				NULL
#define FskAlphaBlendYUV420i8G					NULL
#define FskBilinearAlphaBlendYUV420i8G  		NULL
#define FskTintCopyYUV420i8G					NULL
#define FskBilinearTintCopyYUV420i8G    		NULL
#define FskUnityCopyYUV420i8G					NULL
#define FskUnityAlphaYUV420i8G					NULL

#define FskCopyYUV420iYUV420					NULL
#define FskBilinearCopyYUV420iYUV420			NULL
#define FskBlendYUV420iYUV420					NULL
#define FskBilinearBlendYUV420iYUV420			NULL
#define FskAlphaYUV420iYUV420					NULL
#define FskBilinearAlphaYUV420iYUV420			NULL
#define FskAlphaBlendYUV420iYUV420				NULL
#define FskBilinearAlphaBlendYUV420iYUV420  	NULL
#define FskTintCopyYUV420iYUV420				NULL
#define FskBilinearTintCopyYUV420iYUV420    	NULL
#define FskUnityCopyYUV420iYUV420				NULL
#define FskUnityAlphaYUV420iYUV420				NULL


#define FskCopyYUV420i24BGR						NULL
#define FskBilinearCopyYUV420i24BGR				NULL
#define FskBlendYUV420i24BGR					NULL
#define FskBilinearBlendYUV420i24BGR			NULL
#define FskAlphaYUV420i24BGR					NULL
#define FskBilinearAlphaYUV420i24BGR			NULL
#define FskAlphaBlendYUV420i24BGR				NULL
#define FskBilinearAlphaBlendYUV420i24BGR  		NULL
#define FskTintCopyYUV420i24BGR					NULL
#define FskBilinearTintCopyYUV420i24BGR    		NULL
#define FskUnityCopyYUV420i24BGR				NULL
#define FskUnityAlphaYUV420i24BGR				NULL


#endif /* !COMPILER_CAN_GENERATE_CONST_PROC_POINTERS */
