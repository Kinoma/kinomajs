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
#ifndef __FSKBUTTONSHADE__
#define __FSKBUTTONSHADE__

#ifndef __FSKBITMAP__
	#include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* These are the available composition operators */
/* Should these #defines be shared in a more public place? */
#define kFskButtonCopy			0	/* a.k.a. Normal */
#define kFskButtonMultiply		1
#define kFskButtonScreen		2
#define kFskButtonOverlay		3
#define kFskButtonDarken		4
#define kFskButtonLighten		5
#define kFskButtonColorDodge	6
#define kFskButtonColorBurn		7
#define kFskButtonHardlight		8
#define kFskButtonSoftLight		11
#define kFskButtonDifference	12
#define kFskButtonExclusion		13

/***************************************************************************************************
 * FskButtonShade
 * The shading bitmap must be in the format kFskBitmapFormat16AG.
 * The src and dst bitmaps have requirements in order for the formats to be compatible:
 *    ---------- src ----------      ---------- dst ----------
 *    kFskBitmapFormat16RGB565LE --> kFskBitmapFormat32A16RGB565LE
 *    kFskBitmapFormat32ARGB     --> kFskBitmapFormat32ARGB (same)
 *    kFskBitmapFormat32ABGR     --> kFskBitmapFormat32ABGR (same)
 *    kFskBitmapFormat32BGRA     --> kFskBitmapFormat32BGRA (same)
 *    kFskBitmapFormat32RGBA     --> kFskBitmapFormat32RGBA (same)
 **************************************************************************************************/

FskAPI(FskErr) FskButtonShade(
	int					compositionOperator,
	FskBitmap			shadeBM,				/* Apply this image ...               */
	FskBitmap			srcBM,					/* ... to this image ...              */
	FskConstPoint		srcLoc,					/* ... at this location ...           */
	FskBitmap			dstBM,					/* ... and write it to this image ... */
	FskConstPoint		dstLoc					/* ... at this location.              */
);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKBUTTONSHADE__ */


