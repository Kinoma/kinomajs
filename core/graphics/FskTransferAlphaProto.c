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

/********************************************************************************
 * FskUnityColorize8A
 ********************************************************************************/

static void
FskName2(FskUnityColorize8A,DstPixelKind)(const FskRectBlitParams *p)
{
	const UInt8									*s				= (const UInt8 *)p->srcBaseAddr;
	SInt32										sRBump			= p->srcRowBytes - p->dstWidth;
	SInt32										dRB				= p->dstRowBytes;
	char										*d0				= (char *)p->dstBaseAddr;
	register const UInt8						*sEnd;
	register FskName3(Fsk,DstPixelKind,Type)	*d;
	char										*dEnd;
	register UInt8								sPix;
	#if FskName3(fsk,DstPixelKind,Bytes) == 3
		FskName3(Fsk,DstPixelKind,Type)				colorScratch;
		register FskName3(Fsk,DstPixelKind,Type)	color;
	#else /* FskName3(fsk,DstPixelKind,Bytes) != 3*/
		UInt32								colorScratch;
		register UInt32						color;
	#endif /* FskName3(fsk,DstPixelKind,Bytes) != 3*/

	FskName2(fskConvert24RGB,DstPixelKind)(p->red, colorScratch);
	color = colorScratch;

	dEnd = d0 + (dRB * p->dstHeight);
	if (p->alpha == 255) {
		for (; d0 != dEnd; s += sRBump, d0 += dRB) {
			for (sEnd = s + p->dstWidth, d = (FskName3(Fsk,DstPixelKind,Type)*)d0; s != sEnd; ) {
				if ((sPix = *s++) != 0) {
					#if FskName3(fsk,DstPixelKind,Bytes) == 3
						if (sPix == 255) {
							*d++ = color;
							continue;
						}
						else
							FskName2(FskBlend,DstPixelKind)(d, color, sPix);
					#else /* FskName3(fsk,DstPixelKind,Bytes) != 3*/
						if (sPix == 255) {
							*d++ = (FskName3(Fsk,DstPixelKind,Type))color;
							continue;
						}
						else
							FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))color, sPix);
					#endif /* FskName3(fsk,DstPixelKind,Bytes) != 3*/
				}
				d = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes));
			}
		}
	}
	else {
		for (; d0 != dEnd; s += sRBump, d0 += dRB) {
			for (sEnd = s + p->dstWidth, d = (FskName3(Fsk,DstPixelKind,Type)*)d0; s != sEnd;
				d = (FskName3(Fsk,DstPixelKind,Type)*)((char*)d + FskName3(fsk,DstPixelKind,Bytes))
			) {
				UInt32 blendLevel = *s++ * p->alpha + 128; blendLevel += blendLevel >> 8;
				sPix = (UInt8)(blendLevel >> 8);	/* this can never be 255 */
				if (sPix != 0) {
					#if FskName3(fsk,DstPixelKind,Bytes) == 3
						FskName2(FskBlend,DstPixelKind)(d, color, sPix);
					#else /* FskName3(fsk,DstPixelKind,Bytes) != 3*/
						FskName2(FskBlend,DstPixelKind)(d, (FskName3(Fsk,DstPixelKind,Type))color, sPix);
					#endif /* FskName3(fsk,DstPixelKind,Bytes) != 3*/
				}
			}
		}
	}
}


#endif /* #if (FskName3(fsk,DstPixelKind,PixelPacking) == fskUniformChunkyPixelPacking) */


#undef DstPixelKind
