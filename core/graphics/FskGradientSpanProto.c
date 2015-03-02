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
#if (FskName3(fsk,DstPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)


/*******************************************************************************
 * RampFillSpan - Used for linear gradients when all alphas == 255
 *******************************************************************************/

static void
FskName2(RampFillSpan,DstPixelKind)(FskSpan *span)
{
	LinearGradientSpanData	*gd	= (LinearGradientSpanData*)(span->spanData);
	UInt8					*p;
	#if FskName3(fsk,DstPixelKind,Bytes) != 3
		register UInt32		pix;
	#endif							/* 2 or 4 byte pixels */

	if (span->setPixel == NULL) {	/* We can write directly into the frame buffer */
		for (p = (UInt8*)(span->p); span->dx-- > 0; gd->r += gd->dr, gd->g += gd->dg, gd->b += gd->db, p += FskName3(fsk,DstPixelKind,Bytes)) {
			#if FskName3(fsk,DstPixelKind,Bytes) != 3
				FskName2(fskConvertFixed,DstPixelKind)(gd->r, gd->g, gd->b, kRampFillBits, pix);
				*((FskName3(Fsk,DstPixelKind,Type)*)p) = (FskName3(Fsk,DstPixelKind,Type))pix;
			#else					/* 3 byte pixels */
				FskName2(fskConvertFixed,DstPixelKind)(gd->r, gd->g, gd->b, kRampFillBits, *p);
			#endif					/* 3 byte pixels */
		}
		span->p = p;				/* Update span pixel pointer */
	}
	else {							/* We need to call a procedure to write to the frame buffer */
		FskPixelType	saveColor = span->fillColor;
		for (p = (UInt8*)(span->p); span->dx-- > 0; gd->r += gd->dr, gd->g += gd->dg, gd->b += gd->db, span->p = p += FskName3(fsk,DstPixelKind,Bytes)) {
			#if FskName3(fsk,DstPixelKind,Bytes) != 3
				FskName2(fskConvertFixed,DstPixelKind)(gd->r, gd->g, gd->b, kRampFillBits, pix);
				*((FskName3(Fsk,DstPixelKind,Type)*)(void*)(&span->fillColor)) = (FskName3(Fsk,DstPixelKind,Type))pix;
			#else					/* 3 byte pixels */
				FskName2(fskConvertFixed,DstPixelKind)(gd->r, gd->g, gd->b, kRampFillBits, span->fillColor);
			#endif					/* 3 byte pixels */
			span->setPixel(span);
		}
		span->fillColor = saveColor;
	}
}


/*******************************************************************************
 * AlphaRampFillSpan - Used for linear gradients when some alpha < 255
 *******************************************************************************/

static void
FskName2(AlphaRampFillSpan,DstPixelKind)(FskSpan *span)
{
	LinearGradientSpanData		*gd				= (LinearGradientSpanData*)(span->spanData);
	UInt8						*p;
	#if FskName3(fsk,DstPixelKind,Bytes) == 2
		register UInt32			pix;
	#endif		/* 2 byte pixels */
	FskPixelType				saveColor		= span->fillColor;

	if (span->blendPixel == NULL) {	/* We can write directly into the frame buffer */
		for (p = (UInt8*)(span->p); span->dx-- > 0; gd->r += gd->dr, gd->g += gd->dg, gd->b += gd->db, gd->a += gd->da, span->p = p += FskName3(fsk,DstPixelKind,Bytes)) {
			#if FskName3(fsk,DstPixelKind,Bytes) == 4															/* Build ARGB color */
				span->fillColor.rgba.r = (UInt8)(gd->r >> (kRampFillBits - 8));
				span->fillColor.rgba.g = (UInt8)(gd->g >> (kRampFillBits - 8));
				span->fillColor.rgba.b = (UInt8)(gd->b >> (kRampFillBits - 8));
				span->fillColor.rgba.a = (UInt8)(gd->a >> (kRampFillBits - 8));
				if (span->fillColor.rgba.a != 255) {
					span->fillColor.rgba.r = FskAlphaMul(span->fillColor.rgba.r, span->fillColor.rgba.a);
					span->fillColor.rgba.g = FskAlphaMul(span->fillColor.rgba.g, span->fillColor.rgba.a);
					span->fillColor.rgba.b = FskAlphaMul(span->fillColor.rgba.b, span->fillColor.rgba.a);
				}
				FskName3(fskConvert,fsk32RGBAFormatKind,DstPixelKind)(span->fillColor.p32);
				FskName2(FskAlphaBlackSourceOver,DstPixelKind)((UInt32*)p, span->fillColor.p32);
			#elif FskName3(fsk,DstPixelKind,Bytes) == 3															/* Build 3 byte color */
				FskName2(fskConvertFixed,DstPixelKind)(gd->r, gd->g, gd->b, kRampFillBits, span->fillColor.p24);
				FskBlend24((Fsk24BitType*)p, span->fillColor.p24, (UInt8)(gd->a >> (kRampFillBits - 8)));
			#elif FskName3(fsk,DstPixelKind,Bytes) == 2															/* Build 2 byte color */
				FskName2(fskConvertFixed,DstPixelKind)(gd->r, gd->g, gd->b, kRampFillBits, pix);
				FskName2(FskBlend,DstPixelKind)((UInt16*)p, (UInt16)pix, (UInt8)(gd->a >> (kRampFillBits - 8)));
			#endif
		}
	}
	else {							/* We need to call a blendPixel() function to write into the frame buffer */
		for (p = (UInt8*)(span->p); span->dx-- > 0; gd->r += gd->dr, gd->g += gd->dg, gd->b += gd->db, gd->a += gd->da, span->p = p += FskName3(fsk,DstPixelKind,Bytes)) {
			UInt8 alpha = (UInt8)(gd->a >> (kRampFillBits - 8));
			#if FskName3(fsk,DstPixelKind,Bytes) == 4															/* Build ARGB color */
				span->fillColor.rgba.r = (UInt8)(gd->r >> (kRampFillBits - 8));
				span->fillColor.rgba.g = (UInt8)(gd->g >> (kRampFillBits - 8));
				span->fillColor.rgba.b = (UInt8)(gd->b >> (kRampFillBits - 8));
				span->fillColor.rgba.a = alpha;
				if (span->fillColor.rgba.a != 255) {
					span->fillColor.rgba.r = FskAlphaMul(span->fillColor.rgba.r, span->fillColor.rgba.a);
					span->fillColor.rgba.g = FskAlphaMul(span->fillColor.rgba.g, span->fillColor.rgba.a);
					span->fillColor.rgba.b = FskAlphaMul(span->fillColor.rgba.b, span->fillColor.rgba.a);
				}
				FskName3(fskConvert,fsk32RGBAFormatKind,DstPixelKind)(span->fillColor.p32);
			#elif FskName3(fsk,DstPixelKind,Bytes) == 3															/* Build 3 byte color */
				FskName2(fskConvertFixed,DstPixelKind)(gd->r, gd->g, gd->b, kRampFillBits, span->fillColor.p24);
			#elif FskName3(fsk,DstPixelKind,Bytes) == 2															/* Build 2 byte color */
				FskName2(fskConvertFixed,DstPixelKind)(gd->r, gd->g, gd->b, kRampFillBits, pix);
				span->fillColor.p16 = (UInt16)pix;
			#endif
			span->blendPixel(span, alpha);
		}
	}
	span->fillColor = saveColor;
}


/*******************************************************************************
 * SetSpanPixel - used for radial gradients
 *******************************************************************************/

static void
FskName2(SetSpanPixel,DstPixelKind)(FskSpan *span)
{
	RadialGradientSpanData		*gd		= (RadialGradientSpanData*)(span->spanData);
	#if FskName3(fsk,DstPixelKind,Bytes) != 3
		register UInt32			pix;
	#endif							/* !3 byte pixels */

	if (span->setPixel == NULL) {	/* We can write directly into the frame buffer */
		#if FskName3(fsk,DstPixelKind,Bytes) != 3
			FskName2(fskConvert24RGB,DstPixelKind)(gd->red, pix);
			*((FskName3(Fsk,DstPixelKind,Type)*)(span->p)) = (FskName3(Fsk,DstPixelKind,Type))pix;
		#else						/* 3 byte pixels */
			FskName2(fskConvert24RGB,DstPixelKind)(gd->red, *((Fsk24BitType*)(span->p)));
		#endif						/* 3 byte pixels */
	}
	else {							/* We need to call a procedure to write to the frame buffer */
		FskPixelType saveColor = span->fillColor;
		#if FskName3(fsk,DstPixelKind,Bytes) != 3
			FskName2(fskConvert24RGB,DstPixelKind)(gd->red, pix);
			*((FskName3(Fsk,DstPixelKind,Type)*)(void*)(&span->fillColor)) = (FskName3(Fsk,DstPixelKind,Type))pix;
		#else						/* 3 byte pixels */
			FskName2(fskConvert24RGB,DstPixelKind)(gd->red, span->fillColor.p24);
		#endif						/* 3 byte pixels */
		span->setPixel(span);
		span->fillColor = saveColor;
	}
	span->p = (char*)(span->p) + FskName3(fsk,DstPixelKind,Bytes);
}


/*******************************************************************************
 * BlendSpanPixel - used for radial gradients with alpha
 *******************************************************************************/

static void
FskName2(BlendSpanPixel,DstPixelKind)(FskSpan *span)
{
	RadialGradientSpanData		*gd		= (RadialGradientSpanData*)(span->spanData);
	#if FskName3(fsk,DstPixelKind,Bytes) != 3
		register UInt32			pix;
	#else
		Fsk24BitType			pix;
	#endif							/* !3 byte pixels */

	if (span->blendPixel == NULL) {	/* We can write directly into the frame buffer */
		#if FskName3(fsk,DstPixelKind,Bytes) != 3
			FskName2(fskConvert24RGB,DstPixelKind)(gd->red, pix);
			FskName2(FskBlend,DstPixelKind)((FskName3(Fsk,DstPixelKind,Type)*)(span->p), (FskName3(Fsk,DstPixelKind,Type))pix, gd->alpha);
		#else						/* 3 byte pixels */
			FskName2(fskConvert24RGB,DstPixelKind)(gd->red, pix);
			FskName2(FskBlend,DstPixelKind)((FskName3(Fsk,DstPixelKind,Type)*)(span->p), pix, gd->alpha);
		#endif						/* 3 byte pixels */
	}
	else {							/* We need to call a procedure to write to the frame buffer */
		FskPixelType saveColor = span->fillColor;
		#if FskName3(fsk,DstPixelKind,Bytes) != 3
			FskName2(fskConvert24RGB,DstPixelKind)(gd->red, pix);
			*((FskName3(Fsk,DstPixelKind,Type)*)(void*)(&span->fillColor)) = (FskName3(Fsk,DstPixelKind,Type))pix;
		#else						/* 3 byte pixels */
			FskName2(fskConvert24RGB,DstPixelKind)(gd->red, span->fillColor);
		#endif						/* 3 byte pixels */
		span->blendPixel(span, gd->alpha);
		span->fillColor = saveColor;
	}
	span->p = (char*)(span->p) + FskName3(fsk,DstPixelKind,Bytes);
}

#endif /* #if (FskName3(fsk,DstPixelKind,PixelPacking) == fskUniformChunkyPixelPacking) */


#undef DstPixelKind
