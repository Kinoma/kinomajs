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
#if (FskName3(fsk,DstPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)


static void
FskName2(BlendFillColor,DstPixelKind)(FskSpan *span, UInt8 alpha)
{
#if FskName3(fsk,DstPixelKind,Bytes) == 4
	UInt32 color;
	if (!span->colorIsPremul) {																/* Straight alpha */
		color = (span->fillColor.p32 & ~(255 << FskName3(fsk,DstPixelKind,AlphaPosition)))	/* Replace ... */
				| (alpha << FskName3(fsk,DstPixelKind,AlphaPosition));						/* ...alpha */
		FskName2(FskAlpha,DstPixelKind)((UInt32*)(span->p), color);							/* Call the straight alpha over proc */
	}
	else {
		color = FskAlphaScale32(alpha, span->fillColor.p32);
		FskName2(FskAlphaBlackSourceOver,DstPixelKind)((UInt32*)(span->p), color);			/* Call the premultiplied alpha over proc */
	}
#else
	FskName2(FskBlend,DstPixelKind)(
		(FskName3(Fsk,DstPixelKind,Type)*)(span->p),					/* Cast the pointer to the appropriate type */
		span->fillColor.FskName2(p, FskName3(fsk,DstPixelKind,Bytes)),	/* Get the fill color in the appropriate format */
		alpha															/* Alpha */
	);
#endif
}

#endif /* (FskName3(fsk,DstPixelKind,PixelPacking) == fskUniformChunkyPixelPacking) */

#undef DstPixelKind
