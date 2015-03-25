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
#ifndef __FSKEDGEENHANCEDTEXT__
#define __FSKEDGEENHANCEDTEXT__

#ifndef __FSKBITMAP__
# include "FskBitmap.h"
#endif /* __FSKBITMAP__ */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */




/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****	Efficient APIs - neither memory allocation nor color conversion		*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/* Bilevel, 1 pixel border */
FskAPI(FskErr)
FskEdgeEnhancedBilevelText(
	const void			*txtBaseAddr,	/* The base address of the text */
	SInt32				txtRowBytes,	/* Y-stride for the text */
	FskConstRectangle	txtRect,		/* Text rect */
	void				*dstBaseAddr,	/* The base address of the destination */
	UInt32				dstPixelBytes,	/* The number of bytes per pixel */
	SInt32				dstRowBytes,	/* Y-stride */
	FskConstPoint		dstPoint,		/* Location of text */
	FskConstRectangle	dstClip,		/* Clip to this rect */
	const void			*textColor,		/* The color of the internal glyph, preformatted for the dst */
	const void			*edgeColor,		/* The color of the edge of the glyph, preformatted for the dst */
	void				*tmpMem			/* At least (txtRowBytes + 2) * 4 bytes (can be NULL) */
);


/* Grayscale, 1 pixel border */
FskAPI(FskErr)
FskEdgeEnhancedGrayscaleText(
	const void			*txtBaseAddr,	/* The base address of the text */
	SInt32				txtRowBytes,	/* Y-stride for the text */
	FskConstRectangle	txtRect,		/* Text rect */
	void				*dstBaseAddr,	/* The base address of the destination */
	UInt32				dstPixelFormat,	/* The format of the destination pixel */
	SInt32				dstRowBytes,	/* Y-stride */
	FskConstPoint		dstPoint,		/* Location of text */
	FskConstRectangle	dstClip,		/* Clip to this rect */
	const void			*textColor,		/* The color of the internal glyph, preformatted for the dst */
	const void			*edgeColor,		/* The color of the edge of the glyph, preformatted for the dst */
	SInt32				blendLevel,		/* The blend level from 0 to 255 */
	void				*tmpMem			/* At least (txtRect->width * 4 + 8) bytes (cannot be NULL) */
);


/* Grayscale, 2 pixel border */
FskAPI(FskErr)
FskEdgeEnhancedDoubleGrayscaleText(
	const void			*txtBaseAddr,	/* The base address of the text */
	SInt32				txtRowBytes,	/* Y-stride for the text */
	FskConstRectangle	txtRect,		/* Text rect */
	void				*dstBaseAddr,	/* The base address of the destination */
	UInt32				dstPixelFormat,	/* The format of the destination pixel */
	SInt32				dstRowBytes,	/* Y-stride */
	FskConstPoint		dstPoint,		/* Location of text */
	FskConstRectangle	dstClip,		/* Clip to this rect */
	const void			*textColor,		/* The color of the internal glyph, preformatted for the dst */
	const void			*edgeColor,		/* The color of the edge of the glyph, preformatted for the dst */
	SInt32				blendLevel,		/* The blend level from 0 to 255 */
	void				*tmpMem			/* At least ((txtRect->width + 4) * 6) bytes (cannot be NULL) */
);




/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****	Inefficient, but easy APIs; these allocate memory and convert color	*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/* Bilevel, 1 pixel border */
FskAPI(FskErr)
FskTransferEdgeEnhancedBilevelText(
	FskConstBitmap		txtBM,			/* The source bitmap containing the bilevel text */
	FskConstRectangle	txtRect,		/* Text rect */
	FskBitmap			dstBM,			/* The destination bitmap */
	FskConstPoint		dstPoint,		/* Location of text */
	FskConstRectangle	dstClip,		/* Clip to this rect */
	FskConstColorRGB	textColor,		/* The color of the internal glyph, preformatted for the dst */
	FskConstColorRGB	edgeColor
);


FskAPI(FskErr)
FskTransferEdgeEnhancedGrayscaleText(
	FskConstBitmap		txtBM,			/* The source bitmap containing the bilevel text */
	FskConstRectangle	txtRect,		/* Text rect */
	FskBitmap			dstBM,			/* The destination bitmap */
	FskConstPoint		dstPoint,		/* Location of text */
	FskConstRectangle	dstClip,		/* Clip to this rect */
	FskConstColorRGBA	textColor,		/* The color of the internal glyph, preformatted for the dst */
	FskConstColorRGBA	edgeColor,
	SInt32				blendLevel
);


FskAPI(FskErr)
FskTransferEdgeEnhancedDoubleGrayscaleText(
	FskConstBitmap		txtBM,			/* The source bitmap containing the bilevel text */
	FskConstRectangle	txtRect,		/* Text rect */
	FskBitmap			dstBM,			/* The destination bitmap */
	FskConstPoint		dstPoint,		/* Location of text */
	FskConstRectangle	dstClip,		/* Clip to this rect */
	FskConstColorRGBA	textColor,		/* The color of the internal glyph, preformatted for the dst */
	FskConstColorRGBA	edgeColor,
	SInt32				blendLevel
);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __FSKEDGEENHANCEDTEXT__ */
