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
#ifndef __FSKGRAPHICS_H__
#define __FSKGRAPHICS_H__

#ifndef __FSK__
# include "Fsk.h"
#endif /* __FSK__ */

#ifndef __FSKFIXEDMATH__
# include "FskFixedMath.h"
#endif /* __FSKFIXEDMATH__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Graphics mode */
enum {
	kFskGraphicsModeCopy		= 1,			/**< Overwrite destination without regard to its content. */
	kFskGraphicsModeAlpha,						/**< Alpha blend src into destination with source-over compositing. */
	kFskGraphicsModeColorize,					/**< Multiply every pixel by the color in the mode parameters. */

	kFskGraphicsModeMask		= 0x00ffffff,	/**< This mask will strip off the non-composition-mode bits. */
	kFskGraphicsModeBilinear	= 0x01000000,	/**< Choose bilinear versus point sampling quality. */
	kFskGraphicsModeAffine		= 0x02000000	/**< Limit transformation to affine instead of projective in FskProjectBitmap. */
};


/* RGB */
typedef struct {
	unsigned char		r;
	unsigned char		g;
	unsigned char		b;
} FskColorRGBRecord, *FskColorRGB;
typedef const FskColorRGBRecord *FskConstColorRGB;
#define FskColorRGBSet(c, R, G, B) do { (c)->r = (R); (c)->g = (G); (c)->b = (B); } while(0)


/* RGBA */
typedef struct {
	unsigned char		r;
	unsigned char		g;
	unsigned char		b;
	unsigned char		a;
} FskColorRGBARecord, *FskColorRGBA;
typedef const FskColorRGBARecord *FskConstColorRGBA;
#define FskColorRGBASet(c, R, G, B, A) do { (c)->r = (R); (c)->g = (G); (c)->b = (B); (c)->a = (A); } while(0)


/* Graphics Mode */
typedef struct {
	UInt32				dataSize;
	SInt32				blendLevel;
} FskGraphicsModeParametersRecord, *FskGraphicsModeParameters;
typedef const FskGraphicsModeParametersRecord *FskConstGraphicsModeParameters;


/* Graphics Mode for Video */
typedef struct {
    FskGraphicsModeParametersRecord header;
	UInt32				kind;				// always 'cbcb'
	FskFixed			contrast;			// 0 means no adjustment
	FskFixed			brightness;			// 0 means no adjustment
	void				*sprites;			// FskVideoSpriteWorld
} FskGraphicsModeParametersVideoRecord, *FskGraphicsModeParametersVideo;
typedef const FskGraphicsModeParametersVideoRecord *FskConstGraphicsModeParametersVideo;


enum {
	kFskTextAlignCenter = 0,
	kFskTextAlignLeft,
	kFskTextAlignRight,
	kFskTextAlignTop,
	kFskTextAlignBottom,
	kFskTextAlignScale		// used by UI.Align
};

enum {
	kFskTextPlain			= 0,
	kFskTextBold			= 1L<<0,
	kFskTextItalic			= 1L<<1,
	kFskTextUnderline		= 1L<<2,
	kFskTextOutline			= 1L<<3,
	kFskTextStrike			= 1L<<4,
	kFskTextOutlineHeavy	= 1L<<5,

	kFskTextTruncateEnd		= 1L<<8,
	kFskTextTruncateCenter	= 1L<<9
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKGRAPHICS_H__ */
