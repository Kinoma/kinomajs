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

#include "KPCubicGL.h"
#include "KPMatrix.h"
#include "KPPixelOps.h"

#include <glu.h>
#include <stdlib.h>
#include <math.h>

#include <ConditionalMacros.h>
#include <ImageCompression.h>


#define kDegreesPerRadian	57.295779513082320877		/* 180 / pi */
#define kPi_2				1.5707963267948966192		/* pi / 2 */
#define kLn2				0.69314718055994530942		/* log(2) */
#define k1_Ln2				1.4426950408889634074		/* 1/log(2) = log2(e) */
#define RTHF				0.70710678118654752440f		/* sqrt(1/2) */



/********************************************************************************
 * KPControllerViewMatrixToGLMatrix
 *
 * This will take the given matrix from the KPPanoController and produce a
 * composite camera matrix suitable for OpenGL.
 *
 * Usually the rotational component is separated from the projection component.
 ********************************************************************************/

void
KPControllerViewMatrixToGLMatrix(const float V[3][3], float G[4][4])
{
	float	W[4][4], C[4][4];
#ifndef USE_HITHER_YON
	float	ifl = 1.0f / (float)sqrt(KPSDeterminantMatrix(V[0], 3));	/* inverse focal length */
#else /* USE_HITHER_YON */
	float	fl		= (float)sqrt(KPSDeterminantMatrix(V[0], 3));		/* focal length */
	float	hither	= fl * 0.5f;
	float	yon		= fl * 2.0f;
	float	iyh		= 0.5f / (hither * yon);
#endif /* USE_HITHER_YON */
	
	/* Embed 3x3 into 4x4, with transposition @@@ we might also need inversion */
	W[0][0] = V[0][0];		W[0][1] = V[1][0];		W[0][2] = V[2][0];	W[0][3] = 0;
	W[1][0] = V[0][1];		W[1][1] = V[1][1];		W[1][2] = V[2][1];	W[1][3] = 0;
	W[2][0] = V[0][2];		W[2][1] = V[1][2];		W[2][2] = V[2][2];	W[2][3] = 0;
	W[3][0] = 0;			W[3][1] = 0;			W[3][2] = 0;		W[3][3] = 1;
	
	/* Make GL Clip Matrix */
	C[0][0] = 1;	C[0][1] = 0;	C[0][2] = 0;						C[0][3] = 0;
	C[1][0] = 0;	C[1][1] = 1;	C[1][2] = 0;						C[1][3] = 0;
	C[2][0] = 0;	C[2][1] = 0;	C[2][2] = 0;						C[2][3] = -1;
#ifndef USE_HITHER_YON
	C[3][0] = 0;	C[3][1] = 0;	C[3][2] = -0.75f * ifl;				C[3][3] = 1.25f * ifl;
#else /* USE_HITHER_YON */
	C[3][0] = 0;	C[3][1] = 0;	C[3][2] = -(yon - hither) * iyh;	C[3][3] = (yon + hither) * iyh;
#endif /* USE_HITHER_YON */

	KPSLinearTransform(W[0], C[0], G[0], 4, 4, 4);
}


/********************************************************************************
 * KPSetGLCameraFromCubicPanoController
 * Suitable for viewing a unit cube.
 ********************************************************************************/

void
KPSetGLCameraFromCubicPanoController(const KPPanoController *ctlr)
{
	struct _CamParams {	/* These need to be in this order - do not change */
		long	width;
		long	height;
		float	roll;
		float	pan;
		float	tilt;
		float	fov;
	} cam;
	float	R3[3][3], aspect;
	GLfloat	R4[4][4];
	
	KPGetPanTiltFOVOfPanoController(ctlr, &cam.pan);
	cam.roll  = KPGetOrientationOfPanoController(ctlr) * kPi_2;
	KPGetWindowSizeOfPanoController(ctlr, &cam.width);
	
	glViewport(0, 0, cam.width, cam.height);	/* @@@ This might be already set somewhere else */

	/* Set projection matrix */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	aspect = (double)(cam.width - 1) / (double)(cam.height - 1);
	gluPerspective(cam.fov * kDegreesPerRadian, aspect, 2.0e-2, 2.0);

	/* Set camera orientation */
	KPSSetRotationPanTiltRoll3x3(cam.pan, cam.tilt, cam.roll, R3[0]);
	KPSEmbedTransposed3x3In4x4Matrix(R3[0], R4[0]);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(R4[0]);
}



/********************************************************************************
 * KPSetGLGeometryFromImage3D
 * Determine the 3D coordinates
 ********************************************************************************/

void
KPSetGLGeometryFromImage3D(long numSrcs, const KPImage3D *srcs, GLfloat *xyz)
{
//	float	f = KPSDeterminantMatrix(srcs->M[0], 3);
//	float	M[3][3];
	float	sr[4][3];
	
	sr[0][2] = sr[1][2] = sr[2][2] = sr[3][2] = 1.0f;
	sr[0][0] = sr[0][1] = sr[1][1] = sr[3][0] = 0.0f;
	sr[1][0] = sr[2][0] = srcs->width  - 1;
	sr[2][1] = sr[3][1] = srcs->height - 1;
	for ( ; numSrcs--; srcs++, xyz += 4*3) {
//		KPSScaleMatrix(f, srcs->M[0], M[0], 3, 3);
//		/* we may have to invert Y */
//		KPSLinearTransform(sr[0], M[0], xyz, 3, 3, 3);
		KPSLinearTransform(sr[0], srcs->M[0], xyz, 4, 3, 3);
	}
}


/* Stuff we need to do:
 *		Convert cufa data to vertices and texture coordinates
 *		Convert matrices to vertices and texture coordinates
 *		Should we use MipMaps?
 */



/********************************************************************************
 * defaultFaceOrientations
 ********************************************************************************/

static float defaultFaceOrientations[6][6] = {	/* These would have 4 instead of 6 components with quaternions */
	/*	------- to -------		------- up -------	*/
	{	 0,		 0,		-1,		 0,		+1,		 0	},	/* Front */
	{	+1,		 0,		 0,		 0,		+1,		 0	},	/* Right */
	{	 0,		 0,		+1,		 0,		+1,		 0	},	/* Back */
	{	-1,		 0,		 0,		 0,		+1,		 0	},	/* Left */
	{	 0,		+1,		 0,		 0,		 0,		+1	},	/* Top */
	{	 0,		-1, 	 0,		 0,		 0,		-1	}	/* Bottom */
};


/********************************************************************************
 * CopyTo24RGB
 *	Copy from any pixel format (except 24RGB currently) to 24RGB.
 ********************************************************************************/

static void
CopyTo24RGB(const void *vSrc, long srcRowBytes, long srcPixelFormat, unsigned char *dst, long dstRowBytes, long width, long height)
{
	const unsigned char	*src				= (const unsigned char*)vSrc;
	long				dstBump				= dstRowBytes - width * 3;
	long				i, srcBump, t;
	
	switch (srcPixelFormat) {
			#if TARGET_RT_BIG_ENDIAN
		case kpFormat32ARGB:
			#else /* TARGET_RT_LITTLE_ENDIAN */
		case kpFormat32BGRA:
			#endif /* TARGET_RT_LITTLE_ENDIAN */
			for (srcBump = srcRowBytes - width * kp32ARGBBytes ; height--; src += srcBump, dst += dstBump) {
				for (i = width; i--; src += kp32ARGBBytes, dst += kp24RGBBytes) {
					t = *((kp32ARGBType*)src);
					kpConvert32ARGB24RGB(t, *dst);
				}
			}
			break;

			#if TARGET_RT_BIG_ENDIAN
		case kpFormat32BGRA:
			#else /* TARGET_RT_LITTLE_ENDIAN */
		case kpFormat32ARGB:
			#endif /* TARGET_RT_LITTLE_ENDIAN */
			for (srcBump = srcRowBytes - width * kp32BGRABytes ; height--; src += srcBump, dst += dstBump) {
				for (i = width; i--; src += kp32BGRABytes, dst += kp24RGBBytes) {
					t = *((kp32BGRAType*)src);
					kpConvert32BGRA24RGB(t, *dst);
				}
			}
			break;

		case kpFormat24BGR:
			for (srcBump = srcRowBytes - width * kp24BGRBytes ; height--; src += srcBump, dst += dstBump) {
				for (i = width; i--; src += kp24BGRBytes, dst += kp24RGBBytes) {
					kpConvert24BGR24RGB(*src, *dst);
				}
			}
			break;

#if 0
		case kpFormat24RGB:
			for (srcBump = srcRowBytes - (width *= kp24BGRBytes) ; height--; src += srcBump, dst += dstBump)
				for (i = width; i--; src += kp24BGRBytes, dst += kp24RGBBytes)
					*dst++ = *src++;
			break;
#endif

			#if TARGET_RT_BIG_ENDIAN
		case kpFormat16RGB565BE:					/* Same-endian 565 */
			#else /* TARGET_RT_LITTLE_ENDIAN */
		case kpFormat16RGB565LE:
			#endif /* TARGET_RT_LITTLE_ENDIAN */
			for (srcBump = srcRowBytes - width * kp16RGB565SEBytes ; height--; src += srcBump, dst += dstBump) {
				for (i = width; i--; src += kp16RGB565SEBytes, dst += kp24RGBBytes) {
					t = *((kp16RGB565SEType*)src);
					kpConvert16RGB565SE24RGB(t, *dst);
				}
			}
			break;

			#if TARGET_RT_BIG_ENDIAN
		case kpFormat16RGB565LE:					/* Different-endian 565 */
			#else /* TARGET_RT_LITTLE_ENDIAN */
		case kpFormat16RGB565BE:
			#endif /* TARGET_RT_LITTLE_ENDIAN */
			for (srcBump = srcRowBytes - width * kp16RGB565DEBytes ; height--; src += srcBump, dst += dstBump) {
				for (i = width; i--; src += kp16RGB565DEBytes, dst += kp24RGBBytes) {
					t = *((kp16RGB565DEType*)src);
					kpConvert16RGB565DE24RGB(t, *dst);
				}
			}
			break;

		case kpFormat16RGB5515LE:
			#if TARGET_RT_LITTLE_ENDIAN
				for (srcBump = srcRowBytes - width * kp16RGB5515SEBytes ; height--; src += srcBump, dst += dstBump) {
					for (i = width; i--; src += kp16RGB5515SEBytes, dst += kp24RGBBytes) {
						t = *((kp16RGB5515SEType*)src);
						kpConvert16RGB5515SE24RGB(t, *dst);
					}
				}
			#else /* TARGET_RT_BIG_ENDIAN */
				for (srcBump = srcRowBytes - width * kp16RGB5515DEBytes ; height--; src += srcBump, dst += dstBump) {
					for (i = width; i--; src += kp16RGB5515DEBytes, dst += kp24RGBBytes) {
						t = *((kp16RGB5515DEType*)src);
						kpConvert16RGB5515DE24RGB(t, *dst);
					}
				}
			#endif /* TARGET_RT_BIG_ENDIAN */
			break;

		case kpFormat16RGB555BE:
			#if TARGET_RT_BIG_ENDIAN
				for (srcBump = srcRowBytes - width * kp16RGB555SEBytes ; height--; src += srcBump, dst += dstBump) {
					for (i = width; i--; src += kp16RGB555SEBytes, dst += kp24RGBBytes) {
						t = *((kp16RGB555SEType*)src);
						kpConvert16RGB555SE24RGB(t, *dst);
					}
				}
			#else /* TARGET_RT_LITTLE_ENDIAN */
				for (srcBump = srcRowBytes - width * kp16RGB555DEBytes ; height--; src += srcBump, dst += dstBump) {
					for (i = width; i--; src += kp16RGB555DEBytes, dst += kp24RGBBytes) {
						t = *((kp16RGB555DEType*)src);
						kpConvert16RGB555DE24RGB(t, *dst);
					}
				}
			#endif /* TARGET_RT_BIG_ENDIAN */
			break;
	}

}


/********************************************************************************
 * CopyImage3DToGLRGB
 ********************************************************************************/

static void
CopyImage3DToGLRGB(const KPImage3D *im, unsigned char *dst, long dstRowBytes, long dstX, long dstY)
{
	dst += dstY * dstRowBytes + dstX * kp24RGBBytes;
	CopyTo24RGB(im->baseAddr, im->rowBytes, im->pixelFormat, dst, dstRowBytes, im->width, im->height);
}


/********************************************************************************
 * CopyRect24
 ********************************************************************************/

static void
CopyRect24(register const unsigned char *src, long srcRowBytes, register unsigned char *dst, long dstRowBytes, long width, long height)
{
	register long w;
	long srcBump, dstBump;
	
	if (srcRowBytes < 0)	src -= (height - 1) * (srcRowBytes = -srcRowBytes);	/* Get rowbytes positive so overlap logic works */
	if (dstRowBytes < 0)	dst -= (height - 1) * (dstRowBytes = -dstRowBytes);	/* Get rowbytes positive so overlap logic works */
	
	width *= 3;																	/* Bytes per width */
	srcBump = srcRowBytes - width;
	dstBump = dstRowBytes - width;
	
	if ((src > dst) || ((src == dst) && (srcRowBytes >= dstRowBytes))) {		/* Copy forward */
		for ( ; height--; src += srcBump, dst += dstBump)
			for (w = width; w--; )
				*dst++ = *src++;
	}
	else {																		/* Copy backward */
		src += (height - 1) * srcRowBytes + width;
		dst += (height - 1) * dstRowBytes + width;
		for ( ; height--; src -= srcBump, dst -= dstBump)
			for (w = width; w--; )
				*--dst = *--src;
	}
}


/********************************************************************************
 * New24RGBGWorldFromPtr
 ********************************************************************************/

static OSStatus
New24RGBGWorldFromPtr(void *baseAddr, long rowBytes, short width, short height, GWorldPtr *gwp)
{
	Rect		r;
	OSStatus	err;
	
	MacSetRect(&r, 0, 0, width, height);
	if ((err = QTNewGWorldFromPtr(gwp, k24RGBPixelFormat, &r, NULL, NULL, kICMTempThenAppMemory, baseAddr, rowBytes)) != noErr) {
		if ((err = QTNewGWorldFromPtr(gwp, k32ARGBPixelFormat, &r, NULL, NULL, kICMTempThenAppMemory, baseAddr, rowBytes)) == noErr) {
			PixMapHandle pm = GetGWorldPixMap(*gwp);
			(**pm).pixelSize	= 24;
			(**pm).pixelFormat	= k24RGBPixelFormat;
		}
	}
	
	return err;
}


/********************************************************************************
 * ResizeFace24RGB
 *	Perform an anti-aliased face resizing
 ********************************************************************************/

static long
ResizeFace24RGB(unsigned char *face, long faceRowBytes, long frDim, long toDim)
{
	ImageDescriptionHandle	idh				= NULL;
	unsigned char			*src			= NULL;
	GWorldPtr				srcGW			= NULL;
	GWorldPtr				dstGW			= NULL;
	long					srcRowBytes;
	PixMapHandle			srcPM;
	Rect					srcRect, dstRect;
	MatrixRecord			matrix;
	ImageSequence			seqID;
	long					err;
	
	srcRowBytes = frDim * 3;
	BAIL_IF_NULL((src = malloc(frDim * srcRowBytes)), err, memFullErr);					/* Allocate temporary source */
	CopyRect24(face, faceRowBytes, src, srcRowBytes, frDim, frDim);						/* Copy the pixels to this tem buffer */
	BAIL_IF_ERR(err = New24RGBGWorldFromPtr(src,  srcRowBytes,  frDim, frDim, &srcGW));	/* Create a GWorld for this */
	BAIL_IF_ERR(err = New24RGBGWorldFromPtr(face, faceRowBytes, toDim, toDim, &dstGW));	/* Create a GWorld for the destination */

	/* Use the Raw codec to do anti-aliasing */
	srcPM = GetGWorldPixMap(srcGW);
	BAIL_IF_ERR(err = MakeImageDescriptionForPixMap(srcPM, &idh));
	MacSetRect(&srcRect, 0, 0, frDim, frDim);
	MacSetRect(&dstRect, 0, 0, toDim, toDim);
	RectMatrix(&matrix, &srcRect, &dstRect);
	DecompressSequenceBegin(&seqID, idh, dstGW, NULL, NULL, &matrix, srcCopy, nil, 0, codecMaxQuality, nil);
	DecompressSequenceFrameS(seqID, GetPixBaseAddr(srcPM), (**idh).dataSize, 0, nil, nil);
	CDSequenceEnd(seqID);

bail:
	if (idh   != NULL)	DisposeHandle((Handle)idh);
	if (srcGW != NULL)	DisposeGWorld(srcGW);
	if (dstGW != NULL)	DisposeGWorld(dstGW);
	if (src   != NULL)	free(src);	
	return err;
}


/********************************************************************************
 * GetNiceGLTextureDimension
 ********************************************************************************/

static long
GetNiceGLTextureDimension(long inDim)
{
	long	outDim;

	#ifdef HAVE_EXP2
		outDim = round(exp2(round(log2(inDim))));
	#else /* !HAVE_EXP2 */
		outDim = round(exp(round(log(inDim) * k1_Ln2) * kLn2));
	#endif /* !HAVE_EXP2 */
	
	return outDim;
}


/********************************************************************************
 * KPConvertImages3DToCanonicalGLTextures
 ********************************************************************************/

long
KPConvertImages3DToCanonicalGLTextures(long numSrcs, const KPImage3D *srcs, long *glDim, unsigned char **glRGB)
{
	float			focalLength = sqrt(fabs(1.0 / KPSDeterminantMatrix(srcs->M[0], 3)));
	long			faceDim		= (long)(focalLength * 2 + 1 + 0.5);		/* We should bump this up to a power of two */
	long			niceDim		= GetNiceGLTextureDimension(faceDim);		/* Nearby power of two */
	long			maxDim		= (faceDim > niceDim) ? faceDim : niceDim;
	long			rowBytes	= maxDim * kp24RGBBytes;					/* R, G, B */
	long			faceBytes	= rowBytes * maxDim;
	long			texBytes	= 6 * faceBytes;
	long			err			= 0;
	unsigned char	*face[6], *f;
	float			M[3][3], N[3][3], v[3];
	long			ofst[2];
	int				i;

	/* Allocate image memory */
	*glRGB = malloc(texBytes);
	if (*glRGB == NULL) {
		*glDim = 0;
		return -1;
	}
	*glDim = faceDim;
	
	/* Initialize face pointers */	
	for (i = 0; i < 6; i++)
		face[i] = *glRGB + i * faceBytes;
	
	/* Assemble faces from tiles and convert to 24 RGB */
	for ( ; numSrcs--; srcs++) {
		KPSScaleMatrix(focalLength, srcs->M[0], M[0], 3, 3);
		KPSCrossProductVector3D(M[0], M[1], v);
		for (i = 6; i--; )
			if (fabs(KPSDotProductVector(v, defaultFaceOrientations[i], 3) - 1.0f) < 1.0e-4)
				break;
		N[2][0] = defaultFaceOrientations[i][0];	N[2][1] = defaultFaceOrientations[i][1];	N[2][2] = defaultFaceOrientations[i][2];
		N[1][0] = defaultFaceOrientations[i][3];	N[1][1] = defaultFaceOrientations[i][4];	N[1][2] = defaultFaceOrientations[i][5];
		KPSCrossProductVector3D(N[1], N[2], N[0]);
		KPSTransposeSquareMatrixInPlace(N[0], 3);
		KPSLinearTransform(M[2], N[0], v, 1, 3, 3);
		ofst[0] = (long)(v[0] + focalLength + 0.5);
		ofst[1] = (long)(v[1] + focalLength + 0.5);
		CopyImage3DToGLRGB(srcs, face[i], rowBytes, ofst[0], ofst[1]);
	}
	
	/* Resize faces to powers of two */
	if (faceDim != niceDim) {
		for (i = 6; i--; ) {
			if ((err = ResizeFace24RGB(face[i], rowBytes, faceDim, niceDim)) != 0)
				goto bail;
		}
		*glDim = niceDim;
		
		/* Squeeze out extra bytes */
		if (niceDim < faceDim) {
			long	niceRowBytes	= niceDim * kp24RGBBytes;
			long	niceFaceBytes	= niceDim * niceRowBytes;
			for (i = 0, f = face[0]; i < 6; i++, f += niceFaceBytes)
				CopyRect24(face[i], rowBytes, f, niceRowBytes, niceDim, niceDim);
			if ((face[0] = realloc(*glRGB, 6 * niceFaceBytes)) != NULL)
				*glRGB = face[0];
		}
	}
	
bail:
	return err;
}


/********************************************************************************
 * KPDisposeCanonicalGLTextures
 ********************************************************************************/

void
KPDisposeCanonicalGLTextures(unsigned char *pix)
{
	if (pix != NULL)
		free(pix);
}


/********************************************************************************
 * defaultFaceImmersion
 ********************************************************************************/

static const QTVRCubicFaceData defaultFaceImmersion[6] = {
/*	QW		QX		QY		QZ		CenterX	CenterY	Aspect	Skew	*/
	1.0f,	0.0f,	0.0f,	0.0f,	0.0f,	0.0f,	1.0f,	0.0f,	/* Front */
	RTHF,	0.0f,	-RTHF,	0.0f,	0.0f,	0.0f,	1.0f,	0.0f,	/* Right */
	0.0f,	0.0f,	1.0f,	0.0f,	0.0f,	0.0f,	1.0f,	0.0f,	/* Back */
	-RTHF,	0.0f,	-RTHF,	0.0f,	0.0f,	0.0f,	1.0f,	0.0f,	/* Left */
	RTHF,	RTHF,	0.0f,	0.0f,	0.0f,	0.0f,	1.0f,	0.0f,	/* Top */
	RTHF,	-RTHF,	0.0f,	0.0f,	0.0f,	0.0f,	1.0f,	0.0f,	/* Bottom */
};


/********************************************************************************
 * KPCubicFaceDataToCaconicalGLTexture
 *	Usage:
 *		long			glDim	= 0;
 *		unsigned char	*glRGB	= NULL;
 *		for (cufa = FirstCufa(), tile = FirstTile(); cufa && tile; cufa = NextCufa(cufa), tile = NextTile(tile))
 *			KPCubicFaceDataToGLTexture(cufa, tile, tileRowBytes, tileWidth, tileHeight, &glDim, &glRGB);
 *		...
 *		KPDisposeCanonicalGLTextures(glRGB);
 * On the first call, storage is allocated for six faces stored contiguously in 24 bit RGB format.
 ********************************************************************************/

long
KPCubicFaceDataToGLTexture(
	const QTVRCubicFaceData	*cufa,				/* Either the actual address of a cufa, or the integers {0,1,2,3,4,5} indicating which face */
	const unsigned char		*tile,
	long					tileRowBytes,
	long					tilePixelFormat,
	long					tileWidth,
	long					tileHeight,
	long					*glDim,
	unsigned char			**glRGB
)
{
	float			normalizer, focalLength, quat[4], center[2], aspectSkew[2], dot;
	long			faceIndex, faceWidth, faceHeight, faceRowBytes, faceBytes, tileCenter[2], faceCenter[2], tileUL[2];
	unsigned char	*face;
	
	if (((faceIndex = (long)cufa) < 0) || (faceIndex > 5)) {			/* cufa holds orientation data */
		faceIndex = -1;

		/* Compute the resolution-dependent tile center, and face dimensions */
		normalizer  = (tileHeight - 1) * 0.5f;							/* The normalizer converts between resolution-dependent and resolution-independent parameters */
		focalLength = KPSNormalizeVector(cufa->orientation, quat, 4);	/* Actually, the square root of the normalized focal length */
		focalLength = focalLength * focalLength * normalizer;			/* Compute the resolution-dependent focal length */
		KPSScaleVector(normalizer, cufa->center,  center,     2);		/* Compute the resolution-dependent center */
		faceWidth    = faceHeight = (long)(focalLength * 2.0f + 1.5f);	/* Since faces are 90 degrees, w = 2 * f + 1 (note that we round, too) */
	}
	else {																/* cufa is an enum, indicating which face */
		faceWidth  = tileWidth;
		faceHeight = tileHeight;
		center[0] = center[1] = 0;
	}

	/* Allocate the texture memory as 6 contiguous faces */
	faceRowBytes = faceWidth * kp24RGBBytes;
	faceBytes    = faceRowBytes * faceHeight;
	if (*glRGB == NULL) {
		*glDim = faceWidth;												/* The number of pixels across each face */
		*glRGB = malloc(faceBytes * 6);									/* Allocate 6 contiguous faces */
		if (*glRGB == NULL) {
			*glDim = 0;
			return -1;
		}
	}
	
	/* Determine which face */
	if (faceIndex < 0) {
		for (faceIndex = 0; faceIndex < 6; faceIndex++) {
			dot = KPSDotProductVector(quat, defaultFaceImmersion[faceIndex].orientation, 4);
			if (fabs(dot - 1.0f) < 1.0e-4)
				break;
		}
	}

	/* Compute the location of th etile in teh face */
	tileCenter[0]	= (tileWidth  - 1) * 0.5f;
	tileCenter[1]	= (tileHeight - 1) * 0.5f;
	faceCenter[0]	= (faceWidth  - 1) * 0.5f;
	faceCenter[1]	= (faceHeight - 1) * 0.5f;
	tileUL[0]		= (long)round(faceCenter[0] - tileCenter[0] + center[0]);
	tileUL[1]		= (long)round(faceCenter[1] - tileCenter[1] - center[1]);
	face			= *glRGB
					+ faceIndex * faceBytes
					+ tileUL[1] * faceRowBytes
					+ tileUL[0] * kp24RGBBytes;

	/* Insert the tile into the face */
	CopyTo24RGB(tile, tileRowBytes, tilePixelFormat, face, faceRowBytes, tileWidth, tileHeight);

	return 0;
}


