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
/**
	\file	FskBlur.c
	\brief	Image blur.
*/
#define __FSKBITMAP_PRIV__	/* To get access to the FskBitmap data structure */

#include "FskMemory.h"
#include "FskBlur.h"
#include "FskPixelOps.h"
#include "FskArch.h"
#include <math.h>

#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
FskInstrumentedTypeRecord gblurTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "blur"};
#define blurlog(...)  do { FskInstrumentedTypePrintfDebug  (&gblurTypeInstrumentation, __VA_ARGS__); } while(0)
#else
#define blurlog(...)
#endif


/* The strategy is to use a Gaussian for smaller sigma,
 * and use a triply-iterated box filter (quadratic) for larger sigma.
 */

#define kQuadToGaussianSigma 	1.05f									/**< Scale factor between    triply-iterated box blur with and equivalent Gaussian sigma */
#define kCubicToGaussianSigma	1.2f									/**< Scale factor between quadruply-iterated box blur with and equivalent Gaussian sigma */
#define kGaussianSigmaToQuadraticRadius	(1.0f / kQuadToGaussianSigma)	/**< Scale factor between Gaussian sigma and quadratic radius. */
#define kGaussianSigmaToBoxRadius		1.7320508f						/**< Scale factor between Gaussian sigma and box radius. */
#ifdef SUPPORT_NEON
#define kSigmaThresholdForBox	16.f									/**< NEON code is 10x faster than C; this is the crossover point. */
#else /* C code */
#define kSigmaThresholdForBox	(5.f * kQuadToGaussianSigma)			/**< Use iterated box when sigma is greater than this. */
#endif /* SUPPORT_NEON*/

/* Sigma cutoff is determined by the equation
 * erfc(x / (sqrt(2) * sigma) == tolerance yields
 * 3.29 for 1/1000
 * 3.10 for 1/512
 * 2.89 for 1/256
 * 2.66 for 1/128
 * 2.58 for 1/100
 * 2.42 for 1/64
 * 2.00 for 1/22
 */
#define kSigmaCutoff 2.5f												/**< tolerance about 1/78 = .0128. */




#if 0
#pragma mark ======== GAUSSIAN BLUR, SEPARABLE SYMMETRIC FILTER ========
#endif

/*******************************************************************************
 *******************************************************************************
 **                Generic separable symmetric filter.                        **
 *******************************************************************************
 ******************************************************************************/

#define kFilterFracBits	12												/**< The number of fractional bits used for the filter coefficients. */

typedef struct SeparableSymmetricFilter SeparableSymmetricFilter;

typedef void (*LoadLine)(SeparableSymmetricFilter *filter, const void *src);
typedef void (*WriteLine)(SeparableSymmetricFilter *filter, void *dst);

typedef	void(*Hfilter)(UInt8 *dst,const UInt8 *src,const UInt16 *f,const int numComponets,int Row,int Radius);
typedef	void(*Vfilter)(UInt8 *dst,const UInt8 *src,const UInt16 *f,const int numComponets,int Row,int Radius);

struct SeparableSymmetricFilter {
	/* These need to be initialized */
	int 	hKernelRadius;		/**< The radius of the kernel used for filtering horizontally. */
	int 	vKernelRadius;		/**< The radius of the kernel used for filtering vertically. */
	int 	hKernelWidth;		/**< The width  of the kernel used for filtering horizontally, 2*(r+1). */
	int 	vKernelHeight;		/**< The width  of the kernel used for filtering  vertically,  2*(r+1). */
	int		imageWidth;			/**< The width  of the image. */
	int		imageHeight;		/**< The height of the image. */
	int		srcRowBytes;		/**< The stride between adjacent pixels vertically in the source. */
	int		dstRowBytes;		/**< The stride between adjacent pixels vertically in the destination. */
	int		pixelBytes;			/**< The stride between adjacent pixels horizontally, both in the source and the destination. */
	int		numComponents;		/**< The number of components to be filtered. */
	UInt16	*hKernel;			/**< The horizontal filter kernel, of size [hKernelRadius+1]. */
	UInt16	*vKernel;			/**< The  vertical  filter kernel, of size [vKernelRadius+1]. */
	UInt8	*hBuffer;			/**< Padded buffer for horizontal filtering. */
	UInt8	*vBuffer;			/**< Vertical buffer of vKernelHeight. */
	UInt8	*dBuffer;			/**< Destination buffer. */
	UInt32	*hacBuffer;			/**< Horizontal accumulation buffer. */
	UInt32	*vacBuffer;			/**< Vertical   accumulation buffer. */

	FskMemPtr	tmpMem;			/**< A pointer to temporary memory, so that it can be deallocated later. */
	LoadLine	loadLine;		/**< The function that is called to load the next line from the source. */
	WriteLine	writeLine;		/**< The function that is called to write the next line into the destination. */

	Hfilter		hfil;			/**< The horizontal filter function. */
	Vfilter		vfil;			/**< The  vertical  filter function. */

	/* These get initialized by ApplySeparableSymmetricFilter() */
	int		vBufShiftBytes;		/**< The number of bytes in the vertical buffer to copy when shifting out the old scanline. */
	int		vBufRowBytes;		/**< The number of bytes in the vertical buffer per scanline. */
	UInt8	*vBufCenterLine;	/**< Pointer to the center line in the vertical buffer. */
};


/*******************************************************************************
 * Horizontal Filter.
 * This applies a symmetric filter to a scanline.
 * This assumes that the borders of the src are appropriately padded.
 ******************************************************************************/
#ifdef SUPPORT_NEON
extern void HVFilterRun_arm_v7_s(UInt8 *dst,const UInt8 *src,const UInt16 *f,const int numComponents,int Row,int Radius);
#endif /* SUPPORT_NEON */

static void	HVFilterRun(UInt8 *dst,const UInt8 *src,const UInt16 *f,const int numComponents,int Row,int Radius)
{
	int w;
	for (w = Row; w--;) {
		const UInt8 *sm = src, *sp = src;
		const UInt16 *f_ptr = f;
		UInt32 acc = *src++ * *f_ptr++;
		int k;
		for (k = Radius; k--;) {
			sm -= numComponents;
			sp += numComponents;
			acc += ((UInt32)(*sm) + *sp) * *f_ptr++;
		}
		acc += (1 << (kFilterFracBits - 1));
		acc >>= kFilterFracBits;
		*dst++ = (UInt8)acc;
	}

}

static void
HorizontalFilter(
	const SeparableSymmetricFilter	*filter,
	UInt8 *dst	/* Intermediate buffer */
)
{
	const UInt8     *src = filter->hBuffer;
	const UInt16 *f = filter->hKernel;
	const int numComponents = filter->numComponents;

	blurlog( "@@@@@@@@@@@@@@@@@@@ calling HorizontalFilterfilter->Hfil!!!%x,%x,%x\n",numComponents,filter->vBufRowBytes,filter->hKernelRadius);
	filter->hfil(dst,src,f,numComponents,filter->vBufRowBytes,filter->hKernelRadius);
}


/*******************************************************************************
 * Vertical Filter.
 * This applies a symmetric filter to a swath of scanlines.
 * This assumes that the borders are appropriately padded.
 ******************************************************************************/

static void
VerticalFilter(
	const SeparableSymmetricFilter *filter
) {
	UInt8 *dst = filter->dBuffer;
	const UInt8     *src = filter->vBufCenterLine;
	const UInt16 *f = filter->vKernel;
	const int vBufRowBytes = filter->vBufRowBytes;
	blurlog( "@@@@@@@@@@@@@@@@@@@ calling VerticalFilterfilter->Hfil!!!%x,%x,%x\n",vBufRowBytes,filter->vBufRowBytes,filter->vKernelRadius);
	filter->vfil(dst,src,f,vBufRowBytes,filter->vBufRowBytes,filter->vKernelRadius);
}


/*******************************************************************************
 * PadLine
 ******************************************************************************/

static void
PadLine(SeparableSymmetricFilter *filter)
{
	UInt8 *d;
	const UInt8 *s;
#ifdef EXTEND_EDGES
	int n = filter->hKernelRadius * filter->numComponents, i;
	for (i = n, d = filter->hBuffer, s = d + filter->numComponents; i--;)
		*--d = *--s;																					/* Propagate left pixel leftward */
	for (i = n, d = filter->hBuffer + filter->vBufRowBytes, s = d - filter->numComponents; i--;)
		*d++ = *s++;																					/* Propagate right pixel rightward */
#else	/* REFLECT_EDGES is better for blur */
	int r, c, j = filter->numComponents << 1;
	for (r = filter->hKernelRadius, d = filter->hBuffer, s = d + filter->numComponents; r--; s += j)
		for (c = filter->numComponents; c--;)
			*--d = *--s;																				/* Reflect over left edge */
	for (r = filter->hKernelRadius, d = filter->hBuffer + filter->vBufRowBytes, s = d - filter->numComponents; r--; s -= j)
		for (c = filter->numComponents; c--;)
			*d++ = *s++;																				/* Reflect over right edge */
#endif /* EDGES */
}


/*******************************************************************************
 * ShiftBuffer - shift all buffer lines up, e.g. for vKernelRadius=2,
 *  ABCDE      FGHIJ
 *	FGHIJ      KLMNO
 *	KLMNO  ->  PQRST
 *	PQRST      UVWXY
 *	UVWXY      UVWXY
 ******************************************************************************/

#define ShiftBuffer(filter) FskMemMove(filter->vBuffer, filter->vBuffer + filter->vBufRowBytes, filter->vBufShiftBytes)


/*******************************************************************************
 * SmearBuffer - replicate the uppermost Prime-initialized line upward,
 * e.g. for vKernelRadius=2,
 *  -----         -----
 *  -----         ABCDE
 *  -----   -->   ABCDE
 *  ABCDE         ABCDE
 *  FGHIJ         FGHIJ
 ******************************************************************************/

static void
SmearBuffer(SeparableSymmetricFilter *filter)
{
#ifdef EXTEND_EDGES
	int i = filter->vKernelRadius * filter->vBufRowBytes;
	UInt8 *d = filter->vBufCenterLine + filter->vBufRowBytes;	/* This is where we loaded the first line */
	const UInt8 *s = d + filter->vBufRowBytes;					/* This is just beyond the first line loaded */

	while (i--)			/* Duplicate the first loaded line to all but the first buffer line */
		*--d = *--s;
#else /* REFLECT_EDGES is better for blur */
	UInt8 *d = filter->vBufCenterLine + filter->vBufRowBytes;	/* This is where we loaded the first line */
	const UInt8 *s = d + filter->vBufRowBytes;					/* This is just beyond the first line loaded */
	int r, i, j = filter->vBufRowBytes << 1;
	for (r = filter->vKernelRadius; r--; s += j)
		for (i = filter->vBufRowBytes; i--;)
			*--d = *--s;
#endif /* EDGES */
}


/*******************************************************************************
 * Generic separable symmetric filter.
 ******************************************************************************/

static FskErr
ApplySeparableSymmetricFilter(
	SeparableSymmetricFilter *filter,
	const UInt8	*src,
	UInt8		*dst
) {
	int i;
	UInt8 *vBufLastLine, *b;
	FskErr err = kFskErrNone;

	/* We don't deal with filters that are real blurry relative to the height of the image */
	if (filter->vKernelRadius > filter->imageHeight)
		return kFskErrInvalidParameter;

	/* Initialize frequently used subexpressions. */
	filter->vBufCenterLine = filter->vBuffer + filter->vKernelRadius * filter->vBufRowBytes;
	filter->vBufShiftBytes = (filter->vKernelHeight - 1) * filter->vBufRowBytes;	/* Set pump shift count */

	/* Prime. Fill the last vKernelRadius lines, then copy the first filled line
	 * to all before it, except for the first. The first time through the Pump loop,
	 * the lines will be shifted so that all lines will be initialized.
	 */
	b = filter->vBufCenterLine + filter->vBufRowBytes;
	for (i = filter->vKernelRadius; i--; src += filter->srcRowBytes, b += filter->vBufRowBytes) {
		filter->loadLine(filter, src);
		PadLine(filter);
		HorizontalFilter(filter, b);
	}
	SmearBuffer(filter);

	/* Pump: Shift the buffer, filter one new line horizontal, then vertically. */
	vBufLastLine = filter->vBuffer + filter->vBufShiftBytes / sizeof(*filter->vBuffer);
	for (i = filter->imageHeight - filter->vKernelRadius; i-- > 0; src += filter->srcRowBytes, dst += filter->dstRowBytes) {
		ShiftBuffer(filter);
		filter->loadLine(filter, src);
		PadLine(filter);
		HorizontalFilter(filter, vBufLastLine);
		VerticalFilter(filter);
		filter->writeLine(filter, dst);
	}

	/* Flush */
	for (i = filter->vKernelRadius; i--; dst += filter->dstRowBytes) {
		ShiftBuffer(filter);
#ifndef EXTEND_EDGES /* If not extending edges: reflect edges, which is better for blurring */
		FskMemCopy(filter->vBuffer + (filter->vKernelRadius << 1) * filter->vBufRowBytes, filter->vBuffer + ((i << 1) + 1) * filter->vBufRowBytes, filter->vBufRowBytes);
#endif /* EDGES */
		VerticalFilter(filter);
		filter->writeLine(filter, dst);
	}

	return err;
}


/*******************************************************************************
 *******************************************************************************
 **                          Load and Write Procs                             **
 *******************************************************************************
 ******************************************************************************/


static void
Load8(SeparableSymmetricFilter *filter, const void *src)
{
	FskMemCopy(filter->hBuffer, src, filter->imageWidth * filter->pixelBytes);
}

static void
Write8(SeparableSymmetricFilter *filter, void *dst)
{
	FskMemCopy(dst, filter->dBuffer, filter->imageWidth * filter->pixelBytes);
}

static void
Load16RGB565SE(SeparableSymmetricFilter *filter, const void *src)
{
	const UInt16 *s = (const UInt16*)src;
	UInt8 *d = filter->hBuffer;
	int n;
	for (n = filter->imageWidth; n--; ++s, d += 3) {
		UInt32 pix = *s;
		fskConvert16RGB565SE24RGB(pix, *d);
	}
}

static void
Write16RGB565SE(SeparableSymmetricFilter *filter, void *dst)
{
	const UInt8 *s = filter->dBuffer;
	UInt16 *d = (UInt16*)dst;
	int n;
	for (n = filter->imageWidth; n--; s += 3, ++d) {
		UInt32 pix;
		fskConvert24RGB16RGB565SE(*s, pix);
		*d = (UInt16)pix;
	}
}

static void
Load16RGB565DE(SeparableSymmetricFilter *filter, const void *src)
{
	const UInt16 *s = (const UInt16*)src;
	UInt8 *d = filter->hBuffer;
	int n;
	for (n = filter->imageWidth; n--; ++s, d += 3) {
		UInt32 pix = *s;
		fskConvert16RGB565DE24RGB(pix, *d);
	}
}

static void
Write16RGB565DE(SeparableSymmetricFilter *filter, void *dst)
{
	const UInt8 *s = filter->dBuffer;
	UInt16 *d = (UInt16*)dst;
	int n;
	for (n = filter->imageWidth; n--; s += 3, ++d) {
		UInt32 pix;
		fskConvert24RGB16RGB565DE(*s, pix);
		*d = (UInt16)pix;
	}
}

#define Load565LE  FskName2(Load,  fsk16RGB565LEFormatKind)
#define Load565BE  FskName2(Load,  fsk16RGB565BEFormatKind)
#define Write565LE FskName2(Write, fsk16RGB565LEFormatKind)
#define Write565BE FskName2(Write, fsk16RGB565BEFormatKind)


/*********************************************************************************
 *********************************************************************************
 **									Gaussian Blur								**
 *********************************************************************************
 *********************************************************************************/


/***************************************************************************//**
 * InitGaussianFilterTable
 * The filter table is filled with (radius + 1) entries, but twice as much memory
 * is needed altogether, including temporary memory.
 *	\param[in]	radius	The nonzero radius of the filter, i.e. after what distance is it chopped off.
 *	\param[in]	sigma	The standard deviation of the Gaussian.
 *	\param[out]	filtab	The storage for the filter, of size (radius + 1) * 2 * sizeof(UInt16) bytes, 2-aligned.
 ******************************************************************************/

static void
InitGaussianFilterTable(int radius, float sigma, UInt16 *filtab)
{
	float *fKernel, *f, g0, g1, g2;
	UInt16 *s;
	int i;
	unsigned int acc;

	/* Compute Gaussian kernel using only one exp; from GPU Gems 3. */
	fKernel = (float*)(filtab);
	g0 = 1;								/* Compute forward quotients */
	g1 = (float)exp(-0.5f / (sigma * sigma));
	g2 = g1 * g1;
	for (i = radius + 1, f = fKernel; i--;) {
		*f++ = g0;
		g0 *= g1;
		g1 *= g2;
	}

	/* Normalize filter */
	for (i = radius, f = fKernel + 1, g0 = *fKernel * 0.5f; i--;)
		g0 += *f++;							/* Accumulate coefficients * 0.5 */
	g0 = (1 << kFilterFracBits) / g0 / 2;	/* Compute normalization factor */
	for (i = radius + 1, f = fKernel; i--;)
		*f++ *= g0;							/* Normalize coefficients */
	for (i = radius + 1, s = filtab, f = fKernel; i--;)
		*s++ = (UInt16)(*f++ + 0.5f);		/* Convert from Float32 to Uint16 */
	for (i = radius + 1, s = filtab, acc = 0; i--;)
		acc += *s++;						/* Accumulate fixed point coefficients */
	acc = (acc << 1) - *filtab;				/* Double the sum of all coefficients except the central one */
	acc -= 1 << kFilterFracBits;			/* Compute excess */
	*filtab -= acc;							/* Remove excess from central coefficient */
}


/***************************************************************************//**
 * InitGaussianFilter
 ******************************************************************************/

static FskErr
InitGaussianFilter(
	FskConstBitmap				srcBM,
	FskConstBitmap				dstBM,
	FskConstRectangle			rect,
	float						sigmaX,
	float						sigmaY,
	SeparableSymmetricFilter	*filter
) {
	int useOneKernel;
	int hKernelBytes, vKernelBytes, hBufferBytes, vBufferBytes, dBufferBytes, neededBytes;
	FskErr err;

	BAIL_IF_FALSE(srcBM->pixelFormat == dstBM->pixelFormat, err, kFskErrMismatch);
	filter->imageWidth  = rect->width;
	filter->imageHeight = rect->height;
	filter->srcRowBytes = srcBM->rowBytes;
	filter->dstRowBytes = dstBM->rowBytes;
	filter->pixelBytes  = srcBM->depth >> 3;

	switch(srcBM->pixelFormat) {
		/* 8 bit components */
		case kFskBitmapFormat8A:
		case kFskBitmapFormat8G:			filter->numComponents = 1;	filter->loadLine = Load8;		filter->writeLine = Write8;			break;
		case kFskBitmapFormat16AG:			filter->numComponents = 2;	filter->loadLine = Load8;		filter->writeLine = Write8;			break;
		case kFskBitmapFormat24BGR:
		case kFskBitmapFormat24RGB:			filter->numComponents = 3;	filter->loadLine = Load8;		filter->writeLine = Write8;			break;
		case kFskBitmapFormat32ARGB:
		case kFskBitmapFormat32BGRA:
		case kFskBitmapFormat32RGBA:
		case kFskBitmapFormat32ABGR:		filter->numComponents = 4;	filter->loadLine = Load8;		filter->writeLine = Write8;			break;
		/* Miscellaneous sized components */
		case kFskBitmapFormat16RGB565BE:	filter->numComponents = 3;	filter->loadLine = Load565BE;	filter->writeLine = Write565BE;		break;
		case kFskBitmapFormat16RGB565LE:
		case kFskBitmapFormat16BGR565LE:	filter->numComponents = 3;	filter->loadLine = Load565LE;	filter->writeLine = Write565LE;		break;
//		case kFskBitmapFormat16RGB5515LE:	filter->numComponents = 4;	filter->loadLine = Load5515LE;	filter->writeLine = Write5515LE;	break;
//		case kFskBitmapFormat16RGBA4444LE:	filter->numComponents = 4;	filter->loadLine = Load4444LE;	filter->writeLine = Write4444LE;	break;
//		case kFskBitmapFormat32A16RGB565LE:	filter->numComponents = 4;	filter->loadLine = LoadG565LE;	filter->writeLine = WriteG565LE;	break;
		default:							return kFskErrUnsupportedPixelType;
	}
	blurlog("test:width=%x,height=%x,sigmaX=%f,sigmaY=%f,", (int)rect->width, (int)rect->height, sigmaX, sigmaY);
	/* Determine storage needs.
	 * If the same sigma is used for X and Y, we use the same table.
	 */
	useOneKernel = sigmaX == sigmaY;
	filter->hKernelRadius = (int)(sigmaX * kSigmaCutoff);
	filter->vKernelRadius = (int)(sigmaY * kSigmaCutoff);
	filter->vKernelHeight = 2 * filter->vKernelRadius + 1;
	filter->vBufRowBytes  = filter->imageWidth * filter->numComponents * sizeof(*filter->vBuffer);
	hKernelBytes = ((filter->hKernelRadius + 1) * sizeof(*filter->hKernel) + 3) & ~3;	/* Since we cast this as a float temporarily, assure proper alignment */
	vKernelBytes = ((filter->vKernelRadius + 1) * sizeof(*filter->vKernel) + 3) & ~3;	/* Since we cast this as a float temporarily, assure proper alignment */
	hBufferBytes = (filter->imageWidth + 2 * filter->hKernelRadius) * filter->numComponents * sizeof(*filter->hBuffer);
	vBufferBytes = filter->vKernelHeight * filter->vBufRowBytes;
	dBufferBytes = filter->vBufRowBytes;

	/* Allocate enough storage for the blur kernels, and the H and V buffers,
	 * as well as temporary memory for the computation of the blur kernels.
	 * Without the last requirement, the computation would look like this:
	 *    neededBytes = hKernelBytes + vKernelBytes + hBufferBytes + vBufferBytes + dBufferBytes;
	 */
	neededBytes = hBufferBytes + vBufferBytes + dBufferBytes;
	if (!useOneKernel) {
		if (neededBytes < vKernelBytes)
			neededBytes = vKernelBytes;
		neededBytes += vKernelBytes;
	}
	if (neededBytes < hKernelBytes)
		neededBytes = hKernelBytes;
	neededBytes += hKernelBytes;

	/* Allocate memory and initialize pointers */
	filter->tmpMem = NULL;
	err = FskMemPtrNew(neededBytes, &filter->tmpMem);
	BAIL_IF_ERR(err);
	filter->hKernel = (UInt16*)(filter->tmpMem);
	filter->vKernel = useOneKernel ? filter->hKernel :
									 (UInt16*)((char*)(filter->hKernel) + hKernelBytes);
	filter->hBuffer = (UInt8*)((char*)(filter->vKernel) + vKernelBytes);
	filter->vBuffer = (UInt8*)((char*)(filter->hBuffer) + hBufferBytes);
	filter->dBuffer = (UInt8*)((char*)(filter->vBuffer) + vBufferBytes);
	filter->hBuffer += filter->hKernelRadius * filter->numComponents;	/* Advance beyond pad */

	InitGaussianFilterTable(filter->hKernelRadius, sigmaX, filter->hKernel);
	if (!useOneKernel)
		InitGaussianFilterTable(filter->vKernelRadius, sigmaY, filter->vKernel);

	#ifdef SUPPORT_NEON
	int implementation = FskHardwareGetARMCPU_All();

	if (implementation == FSK_ARCH_ARM_V7) {
		filter->hfil = HVFilterRun_arm_v7_s;
		filter->vfil = HVFilterRun_arm_v7_s;
	}
	else {
		filter->hfil = HVFilterRun;
		filter->vfil = HVFilterRun;
	}
	#else /* !SUPPORT_NEON */
	filter->hfil = HVFilterRun;
	filter->vfil = HVFilterRun;
	#endif /* SUPPORT_NEON */
bail:
	return err;
}


/*******************************************************************************
 * CleanupGaussianFilter
 ******************************************************************************/

static void
CleanupGaussianFilter(SeparableSymmetricFilter *filter)
{
	FskMemPtrDisposeAt(&filter->tmpMem);
}


/*******************************************************************************
 * BlurClip
 ******************************************************************************/

static FskErr BlurClip(FskConstBitmap srcBM, FskRectangle srcBounds, FskConstBitmap dstBM, FskPoint dstPt) {
	FskErr err	= kFskErrNone;

	BAIL_IF_FALSE(srcBM->pixelFormat == dstBM->pixelFormat, err, kFskErrMismatch);
	dstPt->x -= srcBounds->x;	dstPt->y -= srcBounds->y;														/* Convert from dstPoint to dstVector */
	BAIL_IF_FALSE(FskRectangleIntersect(&srcBM->bounds, srcBounds, srcBounds), err, kFskErrNothingRendered);	/* Clip against src */
	FskRectangleOffset(srcBounds, dstPt->x, dstPt->y);															/* Translate rect to dst */
	BAIL_IF_FALSE(FskRectangleIntersect(&dstBM->bounds, srcBounds, srcBounds), err, kFskErrNothingRendered);	/* Clip against dst */
	FskRectangleOffset(srcBounds, -dstPt->x, -dstPt->y);														/* Translate rect back to src */
	dstPt->x += srcBounds->x;	dstPt->y += srcBounds->y;														/* Convert from dstVector to dstPoint */

bail:
	return err;
}

/*******************************************************************************
 * Gaussian Blur
 ******************************************************************************/

FskErr
FskGaussianBlur(FskConstBitmap srcBM, FskConstRectangle srcRect, FskBitmap dstBM, FskConstPoint dstLoc, float sigmaX, float sigmaY)
{
	FskErr				err			= kFskErrNone;
	const UInt8			*src		= NULL;
	UInt8				*dst		= NULL;
	FskRectangleRecord	srcBounds	= srcRect ? *srcRect : srcBM->bounds;
	FskPointRecord		dstPt;
	SeparableSymmetricFilter filter;

	if (dstLoc)	dstPt = *dstLoc;
	else		FskPointSet(&dstPt, dstBM->bounds.x, dstBM->bounds.y);

	BAIL_IF_ERR(err = BlurClip(srcBM, &srcBounds, dstBM, &dstPt));
	BAIL_IF_ERR(err = InitGaussianFilter(srcBM, dstBM, &srcBounds, sigmaX, sigmaY, &filter));

	FskBitmapReadBegin((FskBitmap)srcBM, (const void**)(const void*)&src, NULL, NULL);
	FskBitmapWriteBegin(          dstBM, (      void**)(      void*)&dst, NULL, NULL);
	src += srcBounds.y * srcBM->rowBytes + srcBounds.x * (srcBM->depth >> 3);
	dst +=     dstPt.y * dstBM->rowBytes +     dstPt.x * (dstBM->depth >> 3);
	err = ApplySeparableSymmetricFilter(&filter, src, dst);
	FskBitmapWriteEnd(dstBM);
	FskBitmapReadEnd((FskBitmap)srcBM);

	CleanupGaussianFilter(&filter);

bail:
	return err;
}


#if 0
#pragma mark ======== BOX BLUR ========
#endif

/*********************************************************************************
 *********************************************************************************
 **								Iterated Box Blur								**
 *********************************************************************************
 *********************************************************************************/

/* When a box blur of width W is applied three times, it is a quadratic blur
 * that closely approximates a Gaussian Blur with sigma W/2, when W is large enough.
 * Below, we show the phase shift and standard deviation (sigma equivalent),
 * when a box blur of a particular width (W) is applied three times.
 *  W   phase  stddev
 *  2   1.5    0.866
 *  3   3.0    1.41
 *  4   4.5    1.94
 *  5   6.0    2.45
 *  6   7.5    3.0
 *  7   9.0    3.5
 *  8  10.5    4.0
 *  9  12.0    4.5
 * 10  13.5    5.0
 * 11  15.0    5.5
 * 12  16.5    6.0
 * 13  18.0    6.5
 * 14  19.5    7.0
 * 15  21.0    7.5
 * 16  22.5    8.0
 * The phase increases by 1.5 pixels when the box width increases by 1 pixel.
 * In order to avoid shifting by 1/2 pixel, we choose standard deviations that have integral phase shifts,
 * i.e. those that are integral-plus-one-half. Below sigma=stddev=4.25 (or 3.25 at the smallest),
 * we use a Gaussian.  Otherwise, we quantize sigma to the closest integer-plus-one-half,
 * apply three box blurs of width 2*sigma, and shift the phase by ((sigma - 1/2) * 3).
 * phase = (int)sigma;
 * width = phase << 1;
 * phase += width;
 * ++width;
 */


typedef UInt16 BoxAccType;	/* If we limit the radius to 128, we can use UInt16 accumulators, which is faster on some devices */

#if 0
	#define COMPUTE_BOX_NORM(r)						FskFracDiv(1, ((r) << 1) + 1)
	#define NORMALIZE_BOX_SUM(acc, norm)			(UInt8)FskFracMul(acc, norm)
	#define NORMALIZE_BOX_SUM565(acc, norm, bits)	FskFixedNMul(acc, norm, 30 bits)	/* Bits needs to have a sign */
#else
	#define COMPUTE_BOX_NORM(r)						(((1U << 24U) + (UInt32)(r)) / (((UInt32)(r) << 1U) + 1U))
	#define NORMALIZE_BOX_SUM(norm, acc)			(UInt8)(((UInt32)(acc) * (UInt32)(norm) + (1U << (24U - 1U))) >> 24U)
	#define NORMALIZE_BOX_SUM565(acc, norm, bits)	(((UInt32)(acc) * (UInt32)(norm) + (1U << (24U bits - 1U))) >> (24U bits))	/* Bits needs to have a sign */
#endif


/*******************************************************************************
 * VBoxAddLineToAccumulators
 *	\param[in]		width	the width of the line, including all components.
 *	\param[in,out]	acp		accumulator array of size [width].
 *	\param[in]		rPlus	row[width] to be added.
 *	\param[in]		rPlus	row[width] to be subtracted.
 *	\param[in]		norm	fraction by which the accumulator is scaled to produce a dst pixel.
 *	\param[out]		dst		row[width] pixels to be produced.
 ******************************************************************************/

static void VBoxAddLineToAccumulators(UInt32 width, const UInt8 *src, BoxAccType *acp) {
	while (width--)
		*acp++ += *src++;
}


/*******************************************************************************
 * VBoxAddLineTwiceToAccumulators
 *	\param[in]		width	the width of the line, including all components.
 *	\param[in,out]	acp		accumulator array of size [width].
 *	\param[in]		rPlus	row[width] to be added.
 *	\param[in]		rPlus	row[width] to be subtracted.
 *	\param[in]		norm	fraction by which the accumulator is scaled to produce a dst pixel.
 *	\param[out]		dst		row[width] pixels to be produced.
 ******************************************************************************/

static void VBoxAddLineTwiceToAccumulators(UInt32 width, const UInt8 *src, BoxAccType *acp) {
	while (width--)
		*acp++ += (BoxAccType)(*src++) << 1;
}


/*******************************************************************************
 * VBoxBlur8
 *	\param[in]		width	the width of the line, including all components.
 *	\param[in,out]	acp		accumulator array of size [width].
 *	\param[in]		rPlus	row[width] to be added.
 *	\param[in]		rPlus	row[width] to be subtracted.
 *	\param[in]		norm	fraction by which the accumulator is scaled to produce a dst pixel.
 *	\param[out]		dst		row[width] pixels to be produced.
 ******************************************************************************/

static void VBoxBlur8(UInt32 width, BoxAccType *acp, const UInt8 *rPlus, const UInt8 *rMinus, FskFract norm, UInt8 *dst) {
	for (; width--; ++rPlus, ++rMinus, ++acp, ++dst) {
		UInt32 acc = *acp + *rPlus;
		*dst = NORMALIZE_BOX_SUM(norm, acc);
		*acp = (BoxAccType)(acc -= *rMinus);
	}
}


/*******************************************************************************
 * VBoxBlur565
 *	\param[in]		width	the width of the line, including all components.
 *	\param[in,out]	acp		accumulator array of size [width].
 *	\param[in]		rPlus	row[width] to be added.
 *	\param[in]		rPlus	row[width] to be subtracted.
 *	\param[in]		norm	fraction by which the accumulator is scaled to produce a dst pixel.
 *	\param[out]		dst		row[width] pixels to be produced.
 ******************************************************************************/

static void VBoxBlur565(UInt32 width, BoxAccType *acp, const UInt8 *rPlus, const UInt8 *rMinus, FskFract norm, UInt16 *dst) {
	for (; width--; rPlus += 3, rMinus += 3, acp += 3, ++dst) {
		UInt32 acc, pix;
		acc = acp[0]; acc += rPlus[0]; pix  = NORMALIZE_BOX_SUM565(acc, norm, +8-fsk16RGB565SERedBits)   << fsk16RGB565SERedPosition;   acc -= rMinus[0]; acp[0] = (BoxAccType)acc;
		acc = acp[1]; acc += rPlus[1]; pix |= NORMALIZE_BOX_SUM565(acc, norm, +8-fsk16RGB565SEGreenBits) << fsk16RGB565SEGreenPosition; acc -= rMinus[1]; acp[1] = (BoxAccType)acc;
		acc = acp[2]; acc += rPlus[2]; pix |= NORMALIZE_BOX_SUM565(acc, norm, +8-fsk16RGB565SEBlueBits)  << fsk16RGB565SEBluePosition;  acc -= rMinus[2]; acp[2] = (BoxAccType)acc;
		*dst = (UInt16)pix;
	}
}


/****************************************************************************//**
 * VBoxBlurImage8
 ********************************************************************************/

static void VBoxBlurImage8(UInt32 width, UInt32 height, UInt32 kernelRadius, const UInt8 *s1, SInt32 srb, UInt8 *d, SInt32 drb, BoxAccType *acc) {
	BoxAccType	*myAcc	= NULL;
	SInt32		midHeight = height - (kernelRadius << 1);
	FskFract	norm;
	UInt32		h;
	const UInt8	*s0;

	h = width * sizeof(*acc);
	if (acc == NULL) {
		if (kFskErrNone != FskMemPtrNew(h, (void**)(void*)&myAcc))
			return;
		acc = myAcc;
	}
	FskMemSet(acc, 0, h);

	if (midHeight >= 0) {
		norm = COMPUTE_BOX_NORM(kernelRadius);

		/* Prime */
		for (h = kernelRadius; h--; s1 += srb)
			VBoxAddLineTwiceToAccumulators(width, s1, acc);
		for (h = kernelRadius, s0 = s1 - srb; h--; s1 += srb, s0 -= srb, d += drb)
			VBoxBlur8(width, acc, s1, s0, norm, d);
		s0 += srb;

		/* Pump */
		for (h = midHeight; h--; s1 += srb, s0 += srb, d += drb)
			VBoxBlur8(width, acc, s1, s0, norm, d);

		/* Flush */
		for (h = kernelRadius, s1 -= srb; h--; s0 += srb, s1 -= srb, d += drb)
			VBoxBlur8(width, acc, s1, s0, norm, d);
	}
	else {
		norm = FskFracDiv(1, height);
		for (h = height; h--; s1 += srb)
			VBoxAddLineToAccumulators(width, s1, acc);
		for (h = width; h--; ++d, ++acc)
			*d = NORMALIZE_BOX_SUM(norm, *acc);
		for (s0 = d - width, d += (drb -= width); --height; s0 += drb, d += drb)
			for (h = width; h--;)
				*d++ = *s0++;
	}

	FskMemPtrDispose((void*)myAcc);
}


/****************************************************************************//**
 * VBoxBlurImage565
 ********************************************************************************/

static void VBoxBlurImage565(UInt32 width, UInt32 height, UInt32 kernelRadius, const UInt8 *s1, SInt32 srb, UInt8 *d, SInt32 drb, BoxAccType *acc) {
	BoxAccType	*myAcc	= NULL;
	SInt32		midHeight = height - (kernelRadius << 1);
	FskFract	norm;
	UInt32		h;
	const UInt8	*s0;

	h = width * 3 * sizeof(*acc);
	if (acc == NULL) {
		if (kFskErrNone != FskMemPtrNew(h, (void**)(void*)&myAcc))
			return;
		acc = myAcc;
	}
	FskMemSet(acc, 0, h);

	if (midHeight >= 0) {
		norm = COMPUTE_BOX_NORM(kernelRadius);

		/* Prime */
		for (h = kernelRadius; h--; s1 += srb)
			VBoxAddLineTwiceToAccumulators(width*3, s1, acc);
		for (h = kernelRadius, s0 = s1 - srb; h--; s1 += srb, s0 -= srb, d += drb)
			VBoxBlur565(width, acc, s1, s0, norm, (UInt16*)d);
		s0 += srb;

		/* Pump */
		for (h = midHeight; h--; s1 += srb, s0 += srb, d += drb)
			VBoxBlur565(width, acc, s1, s0, norm, (UInt16*)d);

		/* Flush */
		for (h = kernelRadius, s1 -= srb; h--; s0 += srb, s1 -= srb, d += drb)
			VBoxBlur565(width, acc, s1, s0, norm, (UInt16*)d);
	}
	else {
		UInt16 *d0, *d1;
		norm = FskFracDiv(1, height);
		for (h = height; h--; s1 += srb)
			VBoxAddLineToAccumulators(width*3, s1, acc);
		for (h = width, d1 = (UInt16*)d; h--; ++d1, acc += 3) {
			UInt32 pix;
			pix  = NORMALIZE_BOX_SUM565(acc[0], norm, -8+fsk16RGB565SERedBits)   << fsk16RGB565SERedPosition;
			pix |= NORMALIZE_BOX_SUM565(acc[1], norm, -8+fsk16RGB565SEGreenBits) << fsk16RGB565SEGreenPosition;
			pix |= NORMALIZE_BOX_SUM565(acc[2], norm, -8+fsk16RGB565SEBlueBits)  << fsk16RGB565SEBluePosition;
			*d1 = (UInt16)pix;
		}
		for (d0 = d1 - width, d1 = (UInt16*)((char*)d1 + (drb -= width * sizeof(*d1))); --height; d0 = (UInt16*)((char*)d0 + drb), d1 = (UInt16*)((char*)d1 + drb))
			for (h = width; h--;)
				*d1++ = *d0++;
	}
	FskMemPtrDispose((void*)myAcc);
}


/***************************************************************************//**
 * HBoxBlur8N.
 *	\param[in]	lineWidth		the number of destination pixels to generate.
 *	\param[in]	pixBytes		the number of bytes per pixel.
 *	\param[in]	kernelRadius	the number of   source    pixels to sum on either side of a central pixel for each output pixel.
 *	\param[in]	src				pointer to the source pixels,      of size (lineWidth * 4).
 *	\param[out]	dst				pointer to the destination pixels, of size (lineWidth * 4).
 ******************************************************************************/

static void HBoxBlur8N(UInt32 lineWidth, UInt32 pixBytes, UInt32 kernelRadius, FskFract	norm, const UInt8 *src, UInt8 *dst) {
	SInt32	midWidth	= lineWidth - (kernelRadius << 1);
	UInt32	acc[4], w, c;

	acc[0] = acc[1] = acc[2] = acc[3] = 0;
	if (midWidth >= 0) {
		const UInt8	*stl;

		/* Prime */
		for (w = kernelRadius; w--; src += pixBytes) for (c = 0; c < pixBytes; ++c)
			acc[c] += src[c] << 1;																	/* Reflect pixels over the edge */
		stl = src - pixBytes;
		for (w = kernelRadius; w--; src += pixBytes, stl -= pixBytes, dst += pixBytes) for (c = 0; c < pixBytes; ++c) {
			dst[c] = NORMALIZE_BOX_SUM(acc[c] += src[c], norm);										/* Add the "right" pixe and write next pixel */
			acc[c] -= stl[c];
		}
		stl += pixBytes;

		/* Pump */
		for (w = midWidth; w--; src += pixBytes, stl += pixBytes, dst += pixBytes) for (c = 0; c < pixBytes; ++c) {
			dst[c] = NORMALIZE_BOX_SUM(acc[c] += src[c], norm);										/* Add the "right" pixe and write next pixel */
			acc[c] -= stl[c];																		/* Remove the "left" pixel */
		}

		/* Flush */
		for (w = kernelRadius, src -= pixBytes; w--; stl += pixBytes, src -= pixBytes, dst += pixBytes) for (c = 0; c < pixBytes; ++c) {
			dst[c] = NORMALIZE_BOX_SUM(acc[c] += src[c], norm);										/* Add the "right" pixe and write next pixel */
			acc[c] -= stl[c];																		/* Remove the "left" pixel */
		}
	}
	else {
		for (w = lineWidth; w--; src += pixBytes) for (c = 0; c < pixBytes; ++c)
			acc[c] += src[c];
		for (c = 0; c < pixBytes; ++c)
			acc[c] = (acc[c] + (lineWidth >> 1)) / lineWidth;										/* Average of all pixels, rounded */
		for (w = lineWidth; w--; dst += pixBytes) for (c = 0; c < pixBytes; ++c)
			dst[c] = (UInt8)acc[c];																	/* Set all dst pixels to the average */
	}
}


#define USE_OPTIMIZED_HBLUR
#ifdef USE_OPTIMIZED_HBLUR	/* 6% faster on Mac, 12% faster on ARM */
/***************************************************************************//**
 * HBoxBlur8.
 *	\param[in]	lineWidth		the number of destination pixels to generate.
 *	\param[in]	kernelRadius	the number of   source    pixels to sum on either side of a central pixel for each output pixel.
 *	\param[in]	src				pointer to the source pixels,      of size (lineWidth * 4).
 *	\param[out]	dst				pointer to the destination pixels, of size (lineWidth * 4).
 ******************************************************************************/

static void HBoxBlur8(UInt32 lineWidth, UInt32 kernelRadius, FskFract norm, const UInt8 *src, UInt8 *dst) {
	SInt32	midWidth	= lineWidth - (kernelRadius << 1);
	UInt32	acc, w;

	acc = 0;
	if (midWidth >= 0) {
		const UInt8	*stl;

		/* Prime */
		for (w = kernelRadius; w--; ++src)
			acc += *src;
		acc <<= 1;																					/* Reflect pixels over the edge */
		stl = src - 1;
		for (w = kernelRadius; w--; ++src, --stl, ++dst) {
			*dst = NORMALIZE_BOX_SUM(acc += *src, norm);											/* Add the "right" pixel and write next pixel */
			acc -= *stl;																			/* Subtract the left pixel */
		}
		stl += 1;

		/* Pump */
		for (w = midWidth; w--; ++src, ++stl, ++dst) {
			*dst = NORMALIZE_BOX_SUM(acc += *src, norm);											/* Add the "right" pixel and write next pixel */
			acc -= *stl;																			/* Remove the "left" pixel */
		}

		/* Flush */
		for (w = kernelRadius, --src; w--; ++stl, --src, ++dst) {
			*dst = NORMALIZE_BOX_SUM(acc += *src, norm);											/* Add the "right" pixel and write next pixel */
			acc -= *stl;																			/* Remove the "left" pixel */
		}
	}
	else {
		for (w = lineWidth; w--; ++src)
			acc += *src;
		acc = (acc + (lineWidth >> 1)) / lineWidth;													/* Average of all pixels, rounded */
		for (w = lineWidth; w--; ++dst)
			*dst = (UInt8)acc;																		/* Set all dst pixels to the average */
	}
}


static void HBoxBlur8888(UInt32 lineWidth, UInt32 kernelRadius, FskFract norm, const UInt8 *src, UInt8 *dst) {
	SInt32	midWidth	= lineWidth - (kernelRadius << 1);
	UInt32	acc[4], w;

	acc[0] = acc[1] = acc[2] = acc[3] = 0;
	if (midWidth >= 0) {
		const UInt8	*stl;

		/* Prime */
		for (w = kernelRadius; w--; src += 4) {
			acc[0] += src[0];
			acc[1] += src[1];
			acc[2] += src[2];
			acc[3] += src[3];
		}
		acc[0] <<= 1; acc[1] <<= 1; acc[2] <<= 1; acc[3] <<= 1;										/* Reflect over edge by doubling */
		stl = src - 4;
		for (w = kernelRadius; w--; src += 4, stl -= 4, dst += 4) {
			dst[0] = NORMALIZE_BOX_SUM(acc[0] += src[0], norm);	acc[0] -= stl[0];					/* Add right pixel, write next pixel, subtract left pixel */
			dst[1] = NORMALIZE_BOX_SUM(acc[1] += src[1], norm);	acc[1] -= stl[1];
			dst[2] = NORMALIZE_BOX_SUM(acc[2] += src[2], norm);	acc[2] -= stl[2];
			dst[3] = NORMALIZE_BOX_SUM(acc[3] += src[3], norm);	acc[3] -= stl[3];
		}
		stl += 4;

		/* Pump */
		for (w = midWidth; w--; src += 4, stl += 4, dst += 4) {
			dst[0] = NORMALIZE_BOX_SUM(acc[0] += src[0], norm);	acc[0] -= stl[0];					/* Add right pixel, write next pixel, subtract left pixel */
			dst[1] = NORMALIZE_BOX_SUM(acc[1] += src[1], norm);	acc[1] -= stl[1];
			dst[2] = NORMALIZE_BOX_SUM(acc[2] += src[2], norm);	acc[2] -= stl[2];
			dst[3] = NORMALIZE_BOX_SUM(acc[3] += src[3], norm);	acc[3] -= stl[3];
		}

		/* Flush */
		for (w = kernelRadius, src -= 4; w--; stl += 4, src -= 4, dst += 4) {
			dst[0] = NORMALIZE_BOX_SUM(acc[0] += src[0], norm);	acc[0] -= stl[0];					/* Add right pixel, write next pixel, subtract left pixel */
			dst[1] = NORMALIZE_BOX_SUM(acc[1] += src[1], norm);	acc[1] -= stl[1];
			dst[2] = NORMALIZE_BOX_SUM(acc[2] += src[2], norm);	acc[2] -= stl[2];
			dst[3] = NORMALIZE_BOX_SUM(acc[3] += src[3], norm);	acc[3] -= stl[3];
		}
	}
	else {
		for (w = lineWidth; w--; src += 4) {
			acc[0] += src[0];
			acc[1] += src[1];
			acc[2] += src[2];
			acc[3] += src[3];
		}
		acc[0] = (acc[0] + (lineWidth >> 1)) / lineWidth;											/* Average of all pixels, rounded */
		acc[1] = (acc[1] + (lineWidth >> 1)) / lineWidth;
		acc[2] = (acc[2] + (lineWidth >> 1)) / lineWidth;
		acc[3] = (acc[3] + (lineWidth >> 1)) / lineWidth;
		for (w = lineWidth; w--; dst += 4) {
			dst[0] = (UInt8)acc[0];																	/* Set all dst pixels to the average */
			dst[1] = (UInt8)acc[1];
			dst[2] = (UInt8)acc[2];
			dst[3] = (UInt8)acc[3];
		}
	}
}
#endif /* USE_OPTIMIZED_HBLUR */


/***************************************************************************//**
 * HBoxBlur565.
 * This works great if used horizontally, but blows out the cache if used vertically.
 *	\param[in]	lineWidth		the number of destination pixels to generate.
 *	\param[in]	kernelRadius	the number of   source    pixels to sum on either side of a central pixel for each output pixel.
 *	\param[in]	src				pointer to the source pixels,      of size (lineWidth).
 *	\param[out]	dst				pointer to the destination pixels, of size (lineWidth * 3).
 ******************************************************************************/

static void HBoxBlur565(UInt32 lineWidth, UInt32 kernelRadius, FskFract norm, const UInt16 *src, UInt8 *dst) {
	SInt32	midWidth	= lineWidth - (kernelRadius << 1);
	UInt32	acc[3], w, pix;

	if (midWidth >= 0) {
		const UInt16	*stl;

		/* Prime */
		for (w = kernelRadius, acc[0] = acc[1] = acc[2] = 0; w--; ++src) {
			pix = *src;
			acc[0] += FskMoveField(pix, fsk16RGB565SERedBits,   fsk16RGB565SERedPosition,   1);		/* Reflect over edge */
			acc[1] += FskMoveField(pix, fsk16RGB565SEGreenBits, fsk16RGB565SEGreenPosition, 1);
			acc[2] += FskMoveField(pix, fsk16RGB565SEBlueBits,  fsk16RGB565SEBluePosition,  1);
		}
		stl = src - 1;
		for (w = kernelRadius; w--; ++src, --stl, dst += 3) {
			pix = *src;
			acc[0] += FskMoveField(pix, fsk16RGB565SERedBits,   fsk16RGB565SERedPosition,   0);
			acc[1] += FskMoveField(pix, fsk16RGB565SEGreenBits, fsk16RGB565SEGreenPosition, 0);
			acc[2] += FskMoveField(pix, fsk16RGB565SEBlueBits,  fsk16RGB565SEBluePosition,  0);
			dst[0] = (UInt8)NORMALIZE_BOX_SUM565(acc[0], norm, -8+fsk16RGB565SERedBits);
			dst[1] = (UInt8)NORMALIZE_BOX_SUM565(acc[1], norm, -8+fsk16RGB565SEGreenBits);
			dst[2] = (UInt8)NORMALIZE_BOX_SUM565(acc[2], norm, -8+fsk16RGB565SEBlueBits);					/* Write next pixel, with 8 bits per component */
			pix = *stl;
			acc[0] -= FskMoveField(pix, fsk16RGB565SERedBits,   fsk16RGB565SERedPosition,   0);
			acc[1] -= FskMoveField(pix, fsk16RGB565SEGreenBits, fsk16RGB565SEGreenPosition, 0);
			acc[2] -= FskMoveField(pix, fsk16RGB565SEBlueBits,  fsk16RGB565SEBluePosition,  0);
		}
		++stl;

		/* Pump */
		for (w = midWidth; w--; ++src, ++stl, dst += 3) {
			pix = *src;
			acc[0] += FskMoveField(pix, fsk16RGB565SERedBits,   fsk16RGB565SERedPosition,   0);
			acc[1] += FskMoveField(pix, fsk16RGB565SEGreenBits, fsk16RGB565SEGreenPosition, 0);
			acc[2] += FskMoveField(pix, fsk16RGB565SEBlueBits,  fsk16RGB565SEBluePosition,  0);		/* Add the "right" pixel */
			dst[0] = (UInt8)NORMALIZE_BOX_SUM565(acc[0], norm, -8+fsk16RGB565SERedBits);
			dst[1] = (UInt8)NORMALIZE_BOX_SUM565(acc[1], norm, -8+fsk16RGB565SEGreenBits);
			dst[2] = (UInt8)NORMALIZE_BOX_SUM565(acc[2], norm, -8+fsk16RGB565SEBlueBits);					/* Write next pixel, with 8 bits per component */
			pix = *stl;
			acc[0] -= FskMoveField(pix, fsk16RGB565SERedBits,   fsk16RGB565SERedPosition,   0);
			acc[1] -= FskMoveField(pix, fsk16RGB565SEGreenBits, fsk16RGB565SEGreenPosition, 0);
			acc[2] -= FskMoveField(pix, fsk16RGB565SEBlueBits,  fsk16RGB565SEBluePosition,  0);		/* Remove the "left" pixel */
		}

		/* Flush */
		for (w = kernelRadius, --src; w--; ++stl, --src, dst += 3) {
			pix = *src;
			acc[0] += FskMoveField(pix, fsk16RGB565SERedBits,   fsk16RGB565SERedPosition,   0);
			acc[1] += FskMoveField(pix, fsk16RGB565SEGreenBits, fsk16RGB565SEGreenPosition, 0);
			acc[2] += FskMoveField(pix, fsk16RGB565SEBlueBits,  fsk16RGB565SEBluePosition,  0);		/* Add the "right" pixel */
			dst[0] = (UInt8)NORMALIZE_BOX_SUM565(acc[0], norm, -8+fsk16RGB565SERedBits);
			dst[1] = (UInt8)NORMALIZE_BOX_SUM565(acc[1], norm, -8+fsk16RGB565SEGreenBits);
			dst[2] = (UInt8)NORMALIZE_BOX_SUM565(acc[2], norm, -8+fsk16RGB565SEBlueBits);					/* Write next pixel, with 8 bits per component */
			pix = *stl;
			acc[0] -= FskMoveField(pix, fsk16RGB565SERedBits,   fsk16RGB565SERedPosition,   0);
			acc[1] -= FskMoveField(pix, fsk16RGB565SEGreenBits, fsk16RGB565SEGreenPosition, 0);
			acc[2] -= FskMoveField(pix, fsk16RGB565SEBlueBits,  fsk16RGB565SEBluePosition,  0);		/* Remove the "left" pixel */
		}
	}
	else {
		for (w = lineWidth, acc[0] = acc[1] = acc[2] = 0; w--; ++src) {
			pix = *src;
			acc[0] += FskMoveField(pix, fsk16RGB565SERedBits,   fsk16RGB565SERedPosition,   0);
			acc[1] += FskMoveField(pix, fsk16RGB565SEGreenBits, fsk16RGB565SEGreenPosition, 0);
			acc[2] += FskMoveField(pix, fsk16RGB565SEBlueBits,  fsk16RGB565SEBluePosition,  0);
		}
		acc[0] = ((acc[0] << (8 - fsk16RGB565SERedBits)) + (lineWidth >> 1)) / lineWidth;
		acc[1] = ((acc[1] << (8 - fsk16RGB565SERedBits)) + (lineWidth >> 1)) / lineWidth;
		acc[2] = ((acc[2] << (8 - fsk16RGB565SERedBits)) + (lineWidth >> 1)) / lineWidth;			/* Average of all pixels, rounded, 8 bits per component */
		for (w = lineWidth; w--; dst += 3) {
			dst[0] = (UInt8)acc[0];	dst[1] = (UInt8)acc[1];	dst[2] = (UInt8)acc[2];					/* Set all dst pixels to the average */
		}
	}
}


/********************************************************************************
 * Box Blur a bitmap.
 ********************************************************************************/

FskErr FskBoxBlur(FskConstBitmap srcBM, FskConstRectangle srcRect, FskBitmap dstBM, FskConstPoint dstLoc, UInt32 radiusX, UInt32 radiusY) {
	FskErr				err 		= kFskErrNone;
	FskBitmap			mid			= NULL;
	FskRectangleRecord	srcBounds	= srcRect ? *srcRect : srcBM->bounds;
	const UInt8			*s			= NULL;
	UInt8				*d			= NULL,
						*m			= NULL;
	UInt32				pixBytes	= dstBM->depth >> 3;
	FskFract			hNorm		= COMPUTE_BOX_NORM(radiusX);
	UInt32				h;
	SInt32				srb, mrb, drb;
	FskPointRecord		dstPt;
	#ifdef FAST_ZERO_RADIUS
		FskRectangleRecord	dstBounds;
	#endif /* FAST_ZERO_RADIUS */

	if (dstLoc)	dstPt = *dstLoc;
	else		FskPointSet(&dstPt, dstBM->bounds.x, dstBM->bounds.y);
	BAIL_IF_ERR(err = BlurClip(srcBM, &srcBounds, dstBM, &dstPt));
	BAIL_IF_ERR(err = FskBitmapReadBegin((FskBitmap)srcBM, (const void**)(const void*)(&s), &srb, NULL));
	BAIL_IF_ERR(err = FskBitmapWriteBegin(          dstBM, (      void**)(      void*)(&d), &drb, NULL));
	s += srcBounds.y * srcBM->rowBytes + srcBounds.x * (srcBM->depth >> 3);
	d +=     dstPt.y * dstBM->rowBytes +     dstPt.x * (dstBM->depth >> 3);
	#ifdef FAST_ZERO_RADIUS
		FskRectangleSet(&dstBounds, dstPt.x, dstPt.y, srcBounds.width, srcBounds.height);
	#endif /* FAST_ZERO_RADIUS */

	if (sizeof(BoxAccType) <= 2) {
		if (radiusX > 128)	radiusX = 128;	/* When the accumulators are only 16 bits, we limit the radius to avoid overflow */
		if (radiusY > 128)	radiusY = 128;
	}
	switch (dstBM->pixelFormat) {
		case kFskBitmapFormat32ABGR:
		case kFskBitmapFormat32ARGB:
		case kFskBitmapFormat32BGRA:
		case kFskBitmapFormat32RGBA:
		#ifdef USE_OPTIMIZED_HBLUR
			BAIL_IF_ERR(err = FskBitmapNew(srcBounds.width, srcBounds.height, dstBM->pixelFormat, &mid));
			BAIL_IF_ERR(err = FskBitmapWriteBegin(mid, (void**)(void*)(&m), &mrb, NULL));
			#ifdef FAST_ZERO_RADIUS
				if (!radiusX)	FskBitmapDraw(srcBM, &srcBounds, mid, NULL, NULL, NULL, kFskGraphicsModeCopy, NULL); else
			#endif /* FAST_ZERO_RADIUS */
			for (h = srcBounds.height; h--; s += srb, m += mrb)
				HBoxBlur8888(srcBounds.width, radiusX, hNorm, s, m);
			#ifdef FAST_ZERO_RADIUS
				if (!radiusY)	FskBitmapDraw(mid, NULL, dstBM, &dstBounds, NULL, NULL, kFskGraphicsModeCopy, NULL); else
			#endif /* FAST_ZERO_RADIUS */
			VBoxBlurImage8(srcBounds.width*pixBytes, srcBounds.height, radiusY, (const UInt8*)mid->bits, mid->rowBytes, d, drb, NULL);
			break;
		#endif /* USE_OPTIMIZED_HBLUR */
		case kFskBitmapFormat8A:
		case kFskBitmapFormat8G:
			BAIL_IF_ERR(err = FskBitmapNew(srcBounds.width, srcBounds.height, dstBM->pixelFormat, &mid));
			BAIL_IF_ERR(err = FskBitmapWriteBegin(mid, (void**)(void*)(&m), &mrb, NULL));
			#ifdef FAST_ZERO_RADIUS
				if (!radiusX)	FskBitmapDraw(srcBM, &srcBounds, mid, NULL, NULL, NULL, kFskGraphicsModeCopy, NULL); else
			#endif /* FAST_ZERO_RADIUS */
			for (h = srcBounds.height; h--; s += srb, m += mrb)
				HBoxBlur8(srcBounds.width, radiusX, hNorm, s, m);
			#ifdef FAST_ZERO_RADIUS
				if (!radiusY)	FskBitmapDraw(mid, NULL, dstBM, &dstBounds, NULL, NULL, kFskGraphicsModeCopy, NULL); else
			#endif /* FAST_ZERO_RADIUS */
			VBoxBlurImage8(srcBounds.width*pixBytes, srcBounds.height, radiusY, (const UInt8*)mid->bits, mid->rowBytes, d, drb, NULL);
			break;
		#ifdef USE_OPTIMIZED_HBLUR
		#endif /* USE_OPTIMIZED_HBLUR */
		case kFskBitmapFormat16AG:
		case kFskBitmapFormat24BGR:
		case kFskBitmapFormat24RGB:
			BAIL_IF_ERR(err = FskBitmapNew(srcBounds.width, srcBounds.height, dstBM->pixelFormat, &mid));
			BAIL_IF_ERR(err = FskBitmapWriteBegin(mid, (void**)(void*)(&m), &mrb, NULL));
			#ifdef FAST_ZERO_RADIUS
				if (!radiusX)	FskBitmapDraw(srcBM, &srcBounds, mid, NULL, NULL, NULL, kFskGraphicsModeCopy, NULL); else
			#endif /* FAST_ZERO_RADIUS */
			for (h = srcBounds.height; h--; s += srb, m += mrb)
				HBoxBlur8N(srcBounds.width, pixBytes, radiusX, hNorm, s, m);
			#ifdef FAST_ZERO_RADIUS
				if (!radiusY)	FskBitmapDraw(mid, NULL, dstBM, &dstBounds, NULL, NULL, kFskGraphicsModeCopy, NULL); else
			#endif /* FAST_ZERO_RADIUS */
			VBoxBlurImage8(srcBounds.width*pixBytes, srcBounds.height, radiusY, (const UInt8*)mid->bits, mid->rowBytes, d, drb, NULL);
			break;
		case FskName2(kFskBitmapFormat,fsk16RGB565SEKindFormat):
			BAIL_IF_ERR(err = FskBitmapNew(srcBounds.width, srcBounds.height, kFskBitmapFormatDefaultRGBA, &mid));
			BAIL_IF_ERR(err = FskBitmapWriteBegin(mid, (void**)(void*)(&m), &mrb, NULL));
			for (h = srcBounds.height; h--; s += srb, m += mrb)
				HBoxBlur565(srcBounds.width, radiusX, hNorm, (const UInt16*)s, (UInt8*)m);
			VBoxBlurImage565(srcBounds.width, srcBounds.height, radiusY, (const UInt8*)mid->bits, mid->rowBytes, d, drb, NULL);
			break;
		default:
			BAIL(kFskErrUnsupportedPixelType);
	}
bail:
	if (m)	FskBitmapWriteEnd(mid);
	if (d)	FskBitmapWriteEnd(dstBM);
	if (s)	FskBitmapReadEnd((FskBitmap)srcBM);
	FskBitmapDispose(mid);
	return err;
}


#if 0
#pragma mark ======== ITERATED BOX BLUR ========
#endif


/*******************************************************************************
 * Iterated Box Blur - Quadratic
 ******************************************************************************/

FskErr FskIteratedBoxBlur(FskConstBitmap srcBM, FskConstRectangle srcRect, FskBitmap dstBM, FskConstPoint dstLoc, float sigmaX, float sigmaY) {
	FskErr				err 		= kFskErrNone;
	FskBitmap			mid			= NULL;
	FskRectangleRecord	srcBounds	= srcRect ? *srcRect : srcBM->bounds;
	UInt32				radiusX		= (UInt32)(sigmaX * kGaussianSigmaToQuadraticRadius),
						radiusY		= (UInt32)(sigmaY * kGaussianSigmaToQuadraticRadius);
	FskFract			hNorm		= COMPUTE_BOX_NORM(radiusX);
	const UInt8			*s			= NULL;
	UInt8				*d			= NULL,
						*m			= NULL;
	BoxAccType			*acc		= NULL;
	UInt32				pixBytes	= dstBM->depth >> 3;
	UInt32				h;
	SInt32				srb, mrb, drb;
	FskPointRecord		dstPt;

	if (dstLoc)	dstPt = *dstLoc;
	else		FskPointSet(&dstPt, dstBM->bounds.x, dstBM->bounds.y);
	BAIL_IF_ERR(err = BlurClip(srcBM, &srcBounds, dstBM, &dstPt));
	mrb = (FskName2(kFskBitmapFormat,fsk16RGB565SEKindFormat) == srcBM->pixelFormat) ? 3 : pixBytes;
	BAIL_IF_ERR(err = FskMemPtrNew(srcBounds.width * mrb * sizeof(*acc), (void**)(void*)&acc));
	BAIL_IF_ERR(err = FskBitmapReadBegin((FskBitmap)srcBM, (const void**)(const void*)(&s), &srb, NULL));
	BAIL_IF_ERR(err = FskBitmapWriteBegin(          dstBM, (      void**)(      void*)(&d), &drb, NULL));
	s += srcBounds.y * srcBM->rowBytes + srcBounds.x * (srcBM->depth >> 3);
	d +=     dstPt.y * dstBM->rowBytes +     dstPt.x * (dstBM->depth >> 3);
	if (sizeof(*acc) <= 2) {
		if (radiusX > 128)	radiusX = 128;	/* When the accumulators are only 16 bits, we limit the radius to avoid overflow */
		if (radiusY > 128)	radiusY = 128;
	}

	switch (dstBM->pixelFormat) {
		case kFskBitmapFormat32ABGR:
		case kFskBitmapFormat32ARGB:
		case kFskBitmapFormat32BGRA:
		case kFskBitmapFormat32RGBA:
		#ifdef USE_OPTIMIZED_HBLUR
			BAIL_IF_ERR(err = FskBitmapNew(srcBounds.width, srcBounds.height, dstBM->pixelFormat, &mid));
			BAIL_IF_ERR(err = FskBitmapWriteBegin(mid, (void**)(void*)(&m), &mrb, NULL));
			for (h = srcBounds.height; h--; s += srb, m += mrb) {
				HBoxBlur8888(srcBounds.width, radiusX, hNorm, s, m);
				HBoxBlur8888(srcBounds.width, radiusX, hNorm, m, (UInt8*)acc);
				HBoxBlur8888(srcBounds.width, radiusX, hNorm, (UInt8*)acc, m);
			}
			VBoxBlurImage8(srcBounds.width*pixBytes, srcBounds.height, radiusY, (UInt8*)mid->bits, mid->rowBytes, d, drb, acc);
			VBoxBlurImage8(srcBounds.width*pixBytes, srcBounds.height, radiusY, d, drb, (UInt8*)mid->bits, mid->rowBytes, acc);
			VBoxBlurImage8(srcBounds.width*pixBytes, srcBounds.height, radiusY, (UInt8*)mid->bits, mid->rowBytes, d, drb, acc);
			break;
		#endif /* USE_OPTIMIZED_HBLUR_8888 */
		case kFskBitmapFormat8A:
		case kFskBitmapFormat8G:
		#ifdef USE_OPTIMIZED_HBLUR
			BAIL_IF_ERR(err = FskBitmapNew(srcBounds.width, srcBounds.height, dstBM->pixelFormat, &mid));
			BAIL_IF_ERR(err = FskBitmapWriteBegin(mid, (void**)(void*)(&m), &mrb, NULL));
			for (h = srcBounds.height; h--; s += srb, m += mrb) {
				HBoxBlur8(srcBounds.width, radiusX, hNorm, s, m);
				HBoxBlur8(srcBounds.width, radiusX, hNorm, m, (UInt8*)acc);
				HBoxBlur8(srcBounds.width, radiusX, hNorm, (UInt8*)acc, m);
			}
			VBoxBlurImage8(srcBounds.width*pixBytes, srcBounds.height, radiusY, (UInt8*)mid->bits, mid->rowBytes, d, drb, acc);
			VBoxBlurImage8(srcBounds.width*pixBytes, srcBounds.height, radiusY, d, drb, (UInt8*)mid->bits, mid->rowBytes, acc);
			VBoxBlurImage8(srcBounds.width*pixBytes, srcBounds.height, radiusY, (UInt8*)mid->bits, mid->rowBytes, d, drb, acc);
			break;
		#endif /* USE_OPTIMIZED_HBLUR_8888 */
		case kFskBitmapFormat16AG:
		case kFskBitmapFormat24BGR:
		case kFskBitmapFormat24RGB:
			BAIL_IF_ERR(err = FskBitmapNew(srcBounds.width, srcBounds.height, dstBM->pixelFormat, &mid));
			BAIL_IF_ERR(err = FskBitmapWriteBegin(mid, (void**)(void*)(&m), &mrb, NULL));
			for (h = srcBounds.height; h--; s += srb, m += mrb) {
				HBoxBlur8N(srcBounds.width, pixBytes, radiusX, hNorm, s, m);
				HBoxBlur8N(srcBounds.width, pixBytes, radiusX, hNorm, m, (UInt8*)acc);
				HBoxBlur8N(srcBounds.width, pixBytes, radiusX, hNorm, (UInt8*)acc, m);
			}
			VBoxBlurImage8(srcBounds.width*pixBytes, srcBounds.height, radiusY, (UInt8*)mid->bits, mid->rowBytes, d, drb, acc);
			VBoxBlurImage8(srcBounds.width*pixBytes, srcBounds.height, radiusY, d, drb, (UInt8*)mid->bits, mid->rowBytes, acc);
			VBoxBlurImage8(srcBounds.width*pixBytes, srcBounds.height, radiusY, (UInt8*)mid->bits, mid->rowBytes, d, drb, acc);
			break;
		case FskName2(kFskBitmapFormat,fsk16RGB565SEKindFormat):
		{	UInt8 *n;
			BAIL_IF_ERR(err = FskBitmapNew(srcBounds.width, srcBounds.height*2, kFskBitmapFormatDefaultRGBA, &mid));	/* 2 tmp images stacked */
			BAIL_IF_ERR(err = FskBitmapWriteBegin(mid, (void**)(void*)(&m), &mrb, NULL));
			for (h = srcBounds.height; h--; s += srb, m += mrb) {
				HBoxBlur565(srcBounds.width,   radiusX, hNorm, (const UInt16*)s,  (UInt8*)m);			/* 5-6-5 --> 8-8-8 */
				HBoxBlur8N(srcBounds.width, 3, radiusX, hNorm, (const UInt8*)m,   (UInt8*)acc);			/* 8-8-8 --> 8-8-8 */
 				HBoxBlur8N(srcBounds.width, 3, radiusX, hNorm, (const UInt8*)acc, (UInt8*)m);			/* 8-8-8 --> 8-8-8 */
			}
			n = (m = (UInt8*)(mid->bits)) + srcBounds.height * mrb;										/* bottom image */
			VBoxBlurImage8(3*srcBounds.width, srcBounds.height, radiusY, m, mrb,     n,     mrb, acc);	/* 8-8-8 --> 8-8-8 */
 			VBoxBlurImage8(3*srcBounds.width, srcBounds.height, radiusY, n, mrb,     m,     mrb, acc);	/* 8-8-8 --> 8-8-8 */
 			VBoxBlurImage565(srcBounds.width, srcBounds.height, radiusY, m, mrb, (UInt8*)d, drb, acc);	/* 8-8-8 --> 5-6-5 */
		}	break;
		default:
			BAIL(kFskErrUnsupportedPixelType);
	}
bail:
	if (m)	FskBitmapWriteEnd(mid);
	if (d)	FskBitmapWriteEnd(dstBM);
	if (s)	FskBitmapReadEnd((FskBitmap)srcBM);
	FskBitmapDispose(mid);
	FskMemPtrDispose(acc);
	return err;
}


#if 0
#pragma mark ======== BLUR ========
#endif

/*******************************************************************************
 * Blur
 ******************************************************************************/
FskErr
FskBlur(FskConstBitmap srcBM, FskConstRectangle srcRect, FskBitmap dstBM, FskConstPoint dstLoc, float sigmaX, float sigmaY)
{
	if (sigmaX < kSigmaThresholdForBox || sigmaY < kSigmaThresholdForBox)
		return FskGaussianBlur(srcBM, srcRect, dstBM, dstLoc, sigmaX, sigmaY);
	else
		return FskIteratedBoxBlur(srcBM, srcRect, dstBM, dstLoc, sigmaX, sigmaY);
}
