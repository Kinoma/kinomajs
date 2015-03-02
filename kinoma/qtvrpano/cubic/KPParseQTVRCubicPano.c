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

#ifndef QT_READER_QTATOM
	#define QT_READER_QTATOM 1
#endif /* QT_READER_QTATOM */

#ifndef QT_READER_TRACKREF
	#define QT_READER_TRACKREF 1
#endif /* QT_READER_TRACKREF */

#include "QTReader.h"
#if __FSK_LAYER__
	#include "FskMatrix.h"
	#include "FskProjectImage.h"
	#define KPImage3D											FskImage3D
	#define KPSNormalizeVector(f, t, n)							FskSNormalizeVector(f, t, n)
	#define KPSScaleVector(s, f, t, n)							FskSScaleVector(s, f, t, n)
	#define KPSImmerseUsingQuaternion3x3(w, h, q, f, c, a, m)	FskSImmerseUsingQuaternion3x3(w, h, q, f, c, a, m)
#else /* !__FSK_LAYER__ */
	#include "KPMatrix.h"
	#include "KPProjectImage.h"
	#define KPSNormalizeVector FskSNormalizeVector
#endif /* !__FSK_LAYER__ */
#include "KPParseQTVRCubicPano.h"
#include "KPCubicPanoController.h"

#if _WIN32
# include <stddef.h>
#else /* !_WIN32 */
# include <stdint.h>
#endif /* !WIN32 */

#if __FSK_LAYER__
	#include "FskBitmap.h"
	#include "FskImage.h"
	#include "FskEndian.h"
#endif


/********************************************************************************
 ********************************************************************************
 *****							System Dependencies							*****
 ********************************************************************************
 ********************************************************************************/



#ifndef FOUR_CHAR_CODE
# if __FSK_LAYER__
#  define FOUR_CHAR_CODE(x)	(x)
# elif TARGET_RT_BIG_ENDIAN
#  define FOUR_CHAR_CODE(x)	(x)
# else /* TARGET_RT_LITTLE_ENDIAN */
#  define FOUR_CHAR_CODE(x)       (((unsigned long) ((x) & 0x000000FF)) << 24) | (((unsigned long) ((x) & 0x0000FF00)) << 8) | (((unsigned long) ((x) & 0x00FF0000)) >> 8) | (((unsigned long) ((x) & 0xFF000000)) >> 24)
# endif /* TARGET_RT_LITTLE_ENDIAN */
#endif /* FOUR_CHAR_CODE */

#ifndef USE_QUICKTIME
# if __FSK_LAYER__
#  define USE_QUICKTIME	0
# elif TARGET_OS_MAC || TARGET_OS_WIN32
#  define USE_QUICKTIME	1
# else /* !(TARGET_OS_MAC || TARGET_OS_WIN32) */
#  define USE_QUICKTIME	0
# endif /* !(TARGET_OS_MAC || TARGET_OS_WIN32) */
#endif /* USE_QUICKTIME */

#ifndef USE_KINOMA_DECODE
# if __FSK_LAYER__
#  define USE_FSK_DECODE 1
# else /* !__FSK_LAYER__ */
#  define USE_KINOMA_DECODE	0
# endif /* !__FSK_LAYER__ */
#endif /* USE_KINOMA_DECODE */

#if TARGET_OS_MAC
# define CopyPortBounds(p, r)	GetPortBounds(p, r)
#else /* !TARGET_OS_MAC */
# define CopyPortBounds(p, r)	*(r) = (p)->portRect
#endif /* !TARGET_OS_MAC */


#ifndef PRAGMA_MARK_SUPPORTED
	#if defined(macintosh) || defined(__MWERKS__)
		#define PRAGMA_MARK_SUPPORTED 1
	#else /* !PRAGMA_MARK_SUPPORTED */
		#define PRAGMA_MARK_SUPPORTED 0
	#endif /* !PRAGMA_MARK_SUPPORTED */
#endif /* PRAGMA_MARK_SUPPORTED */


/********************************************************************************
 ********************************************************************************
 *****							Macros and Typedefs							*****
 ********************************************************************************
 ********************************************************************************/

#define kpRadiansPerDegree	0.017453292519943295769		/* pi / 180 */
#define RTHF				0.70710678118654752440f		/* sqrt(1/2) */

#define noMemErr			-1
#define notCubicQTVRErr		-2
#define notObjectQTVRErr	-3

#define QW	0
#define QX	1
#define QY	2
#define QZ	3


/********************************************************************************
 * PanoFaces
 ********************************************************************************/

typedef struct PanoFaces {
	long				numFaces;
	QTMovieReadProc		readProc;
	QTMovieAllocProc	allocProc;
	QTMovieFreeProc		freeProc;
	void				*refCon;
	KPImage3D			faces[1];
} PanoFaces;



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 *****								Utilities								*****
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * BtoNEndianFloat32
 ********************************************************************************/

#if TARGET_RT_BIG_ENDIAN
	#define BtoNEndianFloat32(p, n)		/* nothing */
#else /* TARGET_RT_LITTLE_ENDIAN */
	static void
	BtoNEndianFloat32(float *f, long n)
	{
		register unsigned long *p, t;
		for (p = (unsigned long*)f; n--; p++) {
			t = *p;
			*p	= (((t >>  0) & 0xFF) << 24)
				| (((t >>  8) & 0xFF) << 16)
				| (((t >> 16) & 0xFF) <<  8)
				| (((t >> 24) & 0xFF) <<  0);
		}
	}
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * BtoNEndianInt32
 ********************************************************************************/

#if TARGET_RT_BIG_ENDIAN
	#define BtoNEndianInt32(p, n)		/* nothing */
#else /* TARGET_RT_LITTLE_ENDIAN */
	static void
	BtoNEndianInt32(unsigned long *p, unsigned long n)
	{
		register unsigned long t;
		for ( ; n--; p++) {
			t = *p;
			*p	= (((t >>  0) & 0xFF) << 24)
				| (((t >>  8) & 0xFF) << 16)
				| (((t >> 16) & 0xFF) <<  8)
				| (((t >> 24) & 0xFF) <<  0);
		}
	}
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * BtoNEndianInt32
 ********************************************************************************/

#ifdef UNUSED
#if TARGET_RT_BIG_ENDIAN
	#define BtoNEndianInt16(p, n)		/* nothing */
#else /* TARGET_RT_LITTLE_ENDIAN */
	static void
	BtoNEndianInt16(unsigned short *p, unsigned long n)
	{
		register unsigned short t;
		for ( ; n--; p++) {
			t = *p;
			*p	= ((t & 0xFF) << 8) | ((t >> 8) & 0xFF);
		}
	}
#endif /* TARGET_RT_LITTLE_ENDIAN */
#endif /* UNUSED */

/********************************************************************************
 * KPQTVRCubicFaceDataToMatrix
 *		Compute the immersion matrix from the QTVR cubic face data.
 ********************************************************************************/

void
KPQTVRCubicFaceDataToMatrix(const QTVRCubicFaceData *cufa, long width, long height, float M[3][3])
{
	float	normalizer, focalLength, quat[4], center[2], aspectSkew[2];

	normalizer  = (height - 1) * 0.5f;								/* The normalizer converts between resolution-dependent and resolution-independent parameters */
	focalLength = KPSNormalizeVector(cufa->orientation, quat, 4);	/* Actually, the square root of the normalized focal length */
	focalLength = focalLength * focalLength * normalizer;			/* Compute the resolution-dependent focal length */
	KPSScaleVector(normalizer, cufa->center,  center,     2);		/* Compute the resolution-dependent center */
	aspectSkew[0] = cufa->aspect;									/* Aspect ratio is resolution-independent */
	aspectSkew[1] = cufa->skew;										/* Skew is resolution-independent */
	quat[QW] = -quat[QW];
	KPSImmerseUsingQuaternion3x3(width, height, quat, focalLength, center, aspectSkew, M[0]);
}


#ifdef UNUSED
/********************************************************************************
 * ConvertArrayFromDegreesToRadians
 ********************************************************************************/

static void
ConvertArrayFromDegreesToRadians(register float *a, register long n)
{
	for ( ; n--; a++)
		*a *= (float)(kpRadiansPerDegree);
}
#endif /* UNUSED */


/********************************************************************************
 * defaultFaceImmersion
 ********************************************************************************/

static const QTVRCubicFaceData defaultFaceImmersion[6] = {
	/*		QW		QX		QY		QZ				CenterX	CenterY		Aspect	Skew	*/
	{	{	1.0f,	0.0f,	0.0f,	0.0f	},	{	0.0f,	0.0f	},	1.0f,	0.0f	},	/* Front */
	{	{	RTHF,	0.0f,	-RTHF,	0.0f	},	{	0.0f,	0.0f	},	1.0f,	0.0f	},	/* Right */
	{	{	0.0f,	0.0f,	1.0f,	0.0f	},	{	0.0f,	0.0f	},	1.0f,	0.0f	},	/* Back */
	{	{	-RTHF,	0.0f,	-RTHF,	0.0f	},	{	0.0f,	0.0f	},	1.0f,	0.0f	},	/* Left */
	{	{	RTHF,	RTHF,	0.0f,	0.0f	},	{	0.0f,	0.0f	},	1.0f,	0.0f	},	/* Top */
	{	{	RTHF,	-RTHF,	0.0f,	0.0f	},	{	0.0f,	0.0f	},	1.0f,	0.0f	}	/* Bottom */
};


#if USE_QUICKTIME

/********************************************************************************
 * GetGWorldFromVideoTrack
 ********************************************************************************/

static OSStatus
GetGWorldFromVideoTrack(QTTrack videoTrack, UInt32 sampleNumber, OSType pixelFormat, QTMovieFreeProc freeProc, GWorldPtr *gwp)
{
	char					*videoSample		= NULL;
	ImageDescriptionHandle	idh					= NULL;
	GWorldPtr				gw					= NULL;
	OSStatus				err;
	UInt32					videoSampleSize;
	void					*sampleDescription;
	Rect					r;
	PixMapHandle			pm;
	CGrafPtr				savePort;
	GDHandle				saveGD;

	GetGWorld(&savePort, &saveGD);																	/* Save port */

	BAIL_IF_ERR(err = QTTrackLoadSample(videoTrack, sampleNumber, (void**)(&videoSample), &videoSampleSize));	/* Get the sample ... */
	sampleDescription = QTTrackGetIndSampleDescription(videoTrack, 1);								/* ... and its description ... */
	PtrToHand(sampleDescription, (Handle *)&idh, *(UInt32 *)sampleDescription);						/* ... in a nice Handle format to please QuickTime */
	r.left   = 0;
	r.top    = 0;
	r.right  = (**idh).width;
	r.bottom = (**idh).height;
	BAIL_IF_ERR(err = QTNewGWorld(&gw, pixelFormat, &r, NULL, NULL, kICMTempThenAppMemory));
	SetGWorld(gw, NULL);
	pm = GetGWorldPixMap(gw);
	LockPixels(pm);
	err = DecompressImage(videoSample, idh, pm, NULL, &r, ditherCopy, NULL);
	UnlockPixels(pm);
	*gwp = gw;

bail:
	if (videoSample != NULL)	(*freeProc)(NULL, videoSample);
	if (idh != NULL)			DisposeHandle((Handle)idh);
	SetGWorld(savePort, saveGD);																	/* Restore port */
	if (err) {					if (gw != NULL)	DisposeGWorld(gw);
								*gwp = NULL;
	}

	return err;
}

#elif USE_KINOMA_DECODE

/********************************************************************************
 * GetKinomaBitmapFromVideoTrack
 ********************************************************************************/

static Err
GetKinomaBitmapFromVideoTrack(QTTrack videoTrack, UInt32 sampleNumber, OSType pixelFormat, UInt32 scaleShift, QTMovieFreeProc freeProc, KinomaBitmapPtr *kbpOut)
{
	Err						err;
	char					*videoSample		= NULL;
	UInt32					videoSampleSize;
	gmVideoState			state				= NULL;
	UInt16					refNum				= sysInvalidRefNum;
	KinomaBitmapPtr			kb					= NULL;
	QTImageDescription		idp					= NULL;
	UInt32					videoFormat;
	UInt16					width, height;

	BAIL_IF_ERR(err = QTTrackLoadSample(videoTrack, sampleNumber, (void**)(&videoSample), &videoSampleSize));	/* Get the sample ... */

	idp = (QTImageDescription)QTTrackGetIndSampleDescription(videoTrack, 1);
	width = idp->width;
	height = idp->height;
	if ('cvid' == idp->cType)
		videoFormat = kColorCinepakFrameType;
	else
	if ('jpeg' == idp->cType) {
		videoFormat = kColorJPEGFrameType;
		width = width >> scaleShift;
		height = height >> scaleShift;
	}
	else {
		err = -1;		// unsupported video codec
		goto bail;
	}

	err = loadVideoLibrary(videoFormat, &refNum, &state);
	BAIL_IF_ERR(err);

	kb = CreateOffscreen(width, height, 16, 16, NULL, 2);
	if (!kb) {
		err = -1;
		goto bail;
	}

	if (kb->ftrNum) MemSemaphoreReserve(true);
	err = gmVideoDecompress(refNum, state, videoSample, videoSampleSize, width, height, kb->baseAddr, kb->rowBytes, 16);
	if (kb->ftrNum) MemSemaphoreRelease(true);
	BAIL_IF_ERR(err);

bail:
	if (err && kb) {
		DisposeOffscreen(kb);
		kb = NULL;
	}

	if (NULL != state)
		gmVideoClose(refNum, state);

	if (sysInvalidRefNum != refNum)
		sysLibRemove(refNum);

	if (videoSample != NULL)
		(*freeProc)(NULL, videoSample);

	*kbpOut = kb;

	return err;
}

#elif USE_FSK_DECODE

FskErr GetFskBitmapFromVideoTrack(QTTrack videoTrack, UInt32 sampleNumber, QTMovieFreeProc freeProc, FskBitmap *bits)
{
	FskErr err									= kFskErrNone;
	char					*videoSample		= NULL;
	UInt32					videoSampleSize;
	QTImageDescription		idp					= NULL;

	idp = (QTImageDescription)QTTrackGetIndSampleDescription(videoTrack, 1);
	if ('jpeg' != idp->cType) {
		err = kFskErrBadData;
		goto bail;
	}

	BAIL_IF_ERR(err = QTTrackLoadSample(videoTrack, sampleNumber, (void**)(void*)(&videoSample), &videoSampleSize));	/* Get the sample ... */

	BAIL_IF_ERR(err = FskImageDecompressData(videoSample, videoSampleSize, "image/jpeg", NULL, 0, 0, NULL, NULL, bits));

bail:
	if (videoSample != NULL)
		(*freeProc)(NULL, videoSample);

	return err;
}



#endif /* USE_KINOMA_DECODE */

#if __FSK_LAYER__
	#if USE_QUICKTIME
		static UInt32 qtToFskFormat(UInt32 qtFmt) {
			UInt32 fskFmt;
			switch (qtFmt) {
				case k16BE565PixelFormat:		fskFmt = kFskBitmapFormat16RGB565BE;	break;
				case k16LE565PixelFormat:		fskFmt = kFskBitmapFormat16RGB565LE;	break;
				case k24BGRPixelFormat:			fskFmt = kFskBitmapFormat24BGR;			break;
				case k24RGBPixelFormat:			fskFmt = kFskBitmapFormat24RGB;			break;
				case k32ABGRPixelFormat:		fskFmt = kFskBitmapFormat32ABGR;		break;
				case k32ARGBPixelFormat:		fskFmt = kFskBitmapFormat32ARGB;		break;
				case k32BGRAPixelFormat:		fskFmt = kFskBitmapFormat32BGRA;		break;
				case k32RGBAPixelFormat:		fskFmt = kFskBitmapFormat32RGBA;		break;
				case k8IndexedGrayPixelFormat:	fskFmt = kFskBitmapFormat8G;			break;
				default:						fskFmt = kFskBitmapFormatUnknown;		break;
			}
			return fskFmt;
		}
		static UInt32 fskToQTFormat(UInt32 fskFmt) {
			UInt32 qtFmt;
			switch (fskFmt) {
				case kFskBitmapFormat16RGB565BE:	qtFmt = k16BE565PixelFormat;		break;
				case kFskBitmapFormat16RGB565LE:	qtFmt = k16LE565PixelFormat;		break;
				case kFskBitmapFormat24BGR:			qtFmt = k24BGRPixelFormat;			break;
				case kFskBitmapFormat24RGB:			qtFmt = k24RGBPixelFormat;			break;
				case kFskBitmapFormat32ABGR:		qtFmt = k32ABGRPixelFormat;			break;
				case kFskBitmapFormat32ARGB:		qtFmt = k32ARGBPixelFormat;			break;
				case kFskBitmapFormat32BGRA:		qtFmt = k32BGRAPixelFormat;			break;
				case kFskBitmapFormat32RGBA:		qtFmt = k32RGBAPixelFormat;			break;
				case kFskBitmapFormat8G:			qtFmt = k8IndexedGrayPixelFormat;	break;
				default:							qtFmt = 0;							break;
			}
			return qtFmt;
		}
	#endif /* USE_QUICKTIME */
#else /* !__FSK_LAYER__ */
	#if USE_QUICKTIME
		static UInt32 qtToFskFormat(UInt32 qtFmt) {
			UInt32 fskFmt;
			switch (qtFmt) {
				case k16BE565PixelFormat:		fskFmt = kFskBitmapFormat16RGB565BE;	break;
				case k16LE565PixelFormat:		fskFmt = kFskBitmapFormat16RGB565LE;	break;
				case k24BGRPixelFormat:			fskFmt = kFskBitmapFormat24BGR;			break;
				case k24RGBPixelFormat:			fskFmt = kFskBitmapFormat24RGB;			break;
				case k32ABGRPixelFormat:		fskFmt = kFskBitmapFormat32ABGR;		break;
				case k32ARGBPixelFormat:		fskFmt = kFskBitmapFormat32ARGB;		break;
				case k32BGRAPixelFormat:		fskFmt = kFskBitmapFormat32BGRA;		break;
				case k32RGBAPixelFormat:		fskFmt = kFskBitmapFormat32RGBA;		break;
				case k8IndexedGrayPixelFormat:	fskFmt = kFskBitmapFormat8G;			break;
				default:						fskFmt = kFskBitmapFormatUnknown;		break;
			}
			return fskFmt;
		}
		static UInt32 fskToQTFormat(UInt32 fskFmt) {
			UInt32 qtFmt;
			switch (fskFmt) {
				case kFskBitmapFormat16RGB565BE:	qtFmt = k16BE565PixelFormat;		break;
				case kFskBitmapFormat16RGB565LE:	qtFmt = k16LE565PixelFormat;		break;
				case kFskBitmapFormat24BGR:			qtFmt = k24BGRPixelFormat;			break;
				case kFskBitmapFormat24RGB:			qtFmt = k24RGBPixelFormat;			break;
				case kFskBitmapFormat32ABGR:		qtFmt = k32ABGRPixelFormat;			break;
				case kFskBitmapFormat32ARGB:		qtFmt = k32ARGBPixelFormat;			break;
				case kFskBitmapFormat32BGRA:		qtFmt = k32BGRAPixelFormat;			break;
				case kFskBitmapFormat32RGBA:		qtFmt = k32RGBAPixelFormat;			break;
				case kFskBitmapFormat8G:			qtFmt = k8IndexedGrayPixelFormat;	break;
				default:							qtFmt = 0;							break;
			}
			return qtFmt;
		}
	#endif /* USE_QUICKTIME */
	static UInt32 kpTofskFormat(UInt32 kpFmt) {
		UInt32 fskFmt;
		switch (kpFmt) {
			case kpFormat32BGRA:		fskFmt = kFskBitmapFormat32BGRA;		break;
			case kpFormat32RGBA:		fskFmt = kFskBitmapFormat32RGBA;		break;
			case kpFormat32ARGB:		fskFmt = kFskBitmapFormat32ARGB;		break;
			case kpFormat32A16RGB565LE:	fskFmt = kFskBitmapFormat32A16RGB565LE;	break;
			case kpFormat24BGR:			fskFmt = kFskBitmapFormat24BGR;			break;
			case kpFormat16RGB565LE:	fskFmt = kFskBitmapFormat16RGB565LE;	break;
			default:					fskFmt = kFskBitmapFormatUnknown;		break;
		}
		return fskFmt;
	}
	static UInt32 fskToKpFormat(UInt32 fskFmt) {
		UInt32 kpFmt;
		switch (fskFmt) {
			case kFskBitmapFormat32BGRA:		kpFmt = kpFormat32BGRA;			break;
			case kFskBitmapFormat32RGBA:		kpFmt = kpFormat32RGBA;			break;
			case kFskBitmapFormat32ARGB:		kpFmt = kpFormat32ARGB;			break;
			case kFskBitmapFormat32A16RGB565LE:	kpFmt = kpFormat32A16RGB565LE;	break;
			case kFskBitmapFormat24BGR:			kpFmt = kpFormat24BGR;			break;
			case kFskBitmapFormat16RGB565LE:	kpFmt = kpFormat16RGB565LE;		break;
			default:							kpFmt = kpFormat32BGRA;			break;
		}
		return kpFmt;
	}
#endif /* !__FSK_LAYER__ */


/********************************************************************************
 * InitImage3DFromVideoTrack
 ********************************************************************************/

static long
InitImage3DFromVideoTrack(QTTrack videoTrack, UInt32 sampleNumber, UInt32 pixelFormat, UInt32 scaleShift, QTMovieFreeProc freeProc, KPImage3D *im)
{
	long		err;

	/* Initialize to zeros in case of an exception */
	im->width		= 0;
	im->height		= 0;
	im->baseAddr	= NULL;
	im->rowBytes	= 0;
	im->userData	= NULL;
	im->pixelFormat	= 0;
	im->numPts		= 0;
	im->pts			= NULL;

	#if USE_QUICKTIME
	{
		GWorldPtr		gw		= NULL;
		PixMapHandle	pm;
		Rect			r;
		OSType			qtPixelFormat;

		#if __FSK_LAYER__
			qtPixelFormat = fskToQTFormat(pixelFormat);		/* Desired QuickTime pixel format */
		#else /* !__FSK_LAYER__ */
			qtPixelFormat = kpToQTFormat(pixelFormat);
		#endif /* __FSK_LAYER__ */

		BAIL_IF_ERR(err = GetGWorldFromVideoTrack(videoTrack, sampleNumber, qtPixelFormat, freeProc, &gw));
		CopyPortBounds(gw, &r);
		pm				= GetGWorldPixMap(gw);
		im->width		= r.right  - r.left;
		im->height		= r.bottom - r.top;
		im->baseAddr	= GetPixBaseAddr(pm);
		im->rowBytes	= QTGetPixMapHandleRowBytes(pm);
		im->userData	= gw;
		qtPixelFormat	= GETPIXMAPPIXELFORMAT(*pm);		/* Actual Quicktime pixel format */
		#if __FSK_LAYER__
			im->pixelFormat = qtToFskFormat(qtPixelFormat);
		#else /* !__FSK_LAYER__ */
			im->pixelFormat = qtToKpFormat(qtPixelFormat);
		#endif /* __FSK_LAYER__ */
	}
	#elif USE_KINOMA_DECODE
	{
		KinomaBitmapPtr kb;

		BAIL_IF_ERR(err = GetKinomaBitmapFromVideoTrack(videoTrack, sampleNumber, 16, scaleShift, freeProc, &kb));
		im->width = kb->width;
		im->height = kb->height;
		im->baseAddr = kb->baseAddr;
		im->rowBytes = kb->rowBytes;
		im->numPts = 0;
		im->pts = NULL;
		im->userData = kb;
		im->pixelFormat = kpFormat16RGB565LE;
	}
	#elif USE_FSK_DECODE
	{
		FskBitmap bits;
		FskRectangleRecord bounds;
		FskBitmapFormatEnum fskPixelFormat;

		BAIL_IF_ERR(err = GetFskBitmapFromVideoTrack(videoTrack, sampleNumber, freeProc, &bits));

		FskBitmapGetBounds(bits, &bounds);
		im->width = bounds.width;
		im->height = bounds.height;
//@@	FskBitmapWriteBegin(bits, &im->baseAddr, &im->rowBytes, NULL);
		im->userData = bits;
		FskBitmapGetPixelFormat(bits, &fskPixelFormat);
		#if __FSK_LAYER__
			im->pixelFormat = fskPixelFormat;
		#else /* !__FSK_LAYER__ */
			im->pixelFormat = fskToKpFormat(fskPixelFormat);
		#endif /* !__FSK_LAYER__ */
	}
	#else /* !USE_QUICKTIME && !KINOMA */
		#error Unsupported OS
		err = -2;
	#endif

bail:
	return err;
}


/********************************************************************************
 * KPNewCubicPanoramaFromFile
 *	returns:
 *		0  if all went well
 *		-1 if there was insufficient memory
 *		-2 if there were QuickTime parsing problems
 ********************************************************************************/

long
KPNewCubicPanoramaFromFile(
	QTMovie				movie,		/* If NULL, it is opened and closed herein */
	void				*fRef,		/* File reference */
	QTMovieReadProc		readProc,	/* Read proc */
	QTMovieAllocProc	allocProc,	/* Alloc proc */
	QTMovieFreeProc		freeProc,	/* Free proc */
	KPProgressProc		progressProc,

	long				nodeIndex,	/* The desired node */

	long				*pNumFaces,	/* The number of faces */
	KPImage3D			**faces,	/* The faces - allocated here */

	KPPanoController	*ctlr		/* If not NULL, sets the port width & height, pan, tilt, & fov */
)
{
	long				err				= 0;
	QTTrack				videoTrack		= NULL;
	//QTTrack			qtvrTrack		= NULL;
	QTTrack				panoTrack		= NULL;
	unsigned char		*panoSample		= NULL;
	long				disposeMovie	= 0;
	UInt32				size;
	UInt32				refIndex;
	void				*panoAtom, *pdatAtom, *cuvwAtom, *cufaAtom;
	QTVRPanoSampleAtom	*pdat = NULL;
	QTVRCubicViewAtom	*cuvw;
	const QTVRCubicFaceData	*cufa;
	PanoFaces			*panoFaces;
	KPImage3D			*f;
	long				portWidthHeight[2], numFaces, i;
	FskBitmapFormatEnum	pixelFormat;
	UInt32				scaleShift = 0;
	void				*cufaScratch = NULL;

	if (nodeIndex <= 0)
		nodeIndex = 1;
	*faces = NULL;

	/* Open a movie if there is none */
	if (movie == NULL) {
		BAIL_IF_ERR(err = QTMovieNewFromReader(&movie, false, readProc, fRef, allocProc, freeProc, fRef));	/* Initialize a QuiockTime reader */
		disposeMovie = 1;
	}

	/* Get the pano sample atom */
//	BAIL_IF_NULL((qtvrTrack = QTMovieGetIndTrackType(movie, FOUR_CHAR_CODE('qtvr'), 1)), err, notCubicQTVRErr);	/* Get qtvr track -- not really necessary */
	BAIL_IF_NULL((panoTrack = QTMovieGetIndTrackType(movie, FOUR_CHAR_CODE('pano'), 1)), err, notCubicQTVRErr);	/* Get pano track */
	portWidthHeight[0] = panoTrack->width;				//	GetTrackDimensions(panoTrack, &portWidth, &portHeight);
	portWidthHeight[1] = panoTrack->height;
	BAIL_IF_ERR(err = QTTrackLoadSample(panoTrack, nodeIndex, (void**)(void*)(&panoSample), &size));		/* Get pano sample */
	panoAtom = QTAtomGetRootAtom(panoSample);																/* Get pano root atom */

	/* Make sure that the panorama track is one for a cubic */
	pdatAtom = QTAtomGetAtomByID(panoAtom, FOUR_CHAR_CODE('pdat'), 1);										/* Get the panorama data atom */
	pdat     = (QTVRPanoSampleAtom*)QTAtomGetAtomDataPtr(pdatAtom, &size);									/* Get the panorama data */
	FskMemPtrNewFromData(size, pdat, (FskMemPtr*)(void*)(&pdat));											/* make copy to ensure alignment */
	BAIL_IF_NULL(pdat, err, -1);
	BtoNEndianInt32((unsigned long *)&pdat->panoType , 1);													/* Get to the proper endian representation */
	BAIL_IF_FALSE((pdat->panoType == FOUR_CHAR_CODE('cube')), err, notCubicQTVRErr);						/* Check to assure that we have a cubic QTVR */

	/* Get the video track from the panorama data atom */
	refIndex = pdat->imageRefTrackIndex;																	/* Get the index to the primary video track */
	BtoNEndianInt32((unsigned long *)&refIndex, 1);															/* Get to the proper endian representation */
	BAIL_IF_NULL((videoTrack = QTTrackGetReference(panoTrack, FOUR_CHAR_CODE('imgt'), refIndex)), err, notCubicQTVRErr);	/* Get the primary video track */

	/* Get the view parameters from the cubic view atom */
	if (ctlr != NULL) {																						/* If a controller was given ... */
		QTVRCubicViewAtom *cuvwScratch;
		KPSetWindowSizeOfPanoController(ctlr, portWidthHeight);
		BAIL_IF_NULL((cuvwAtom = QTAtomGetAtomByID(panoAtom, FOUR_CHAR_CODE('cuvw'), 1)), err, notCubicQTVRErr);/* Get the cubic view atom */
		cuvw = (QTVRCubicViewAtom*)QTAtomGetAtomDataPtr(cuvwAtom, &size);										/* Get the cubic view data */
		FskMemPtrNewFromData(size, cuvw, (FskMemPtr*)(void*)(&cuvwScratch));									/* make copy to ensure alignment */
		BAIL_IF_NULL(cuvwScratch, err, -1);
		KPQTVRCubicViewAtomInitPanoController(ctlr, cuvwScratch);
		FskMemPtrDispose(cuvwScratch);
	}

	/* Get the cubic face data */
	cufaAtom = QTAtomGetAtomByID(panoAtom, FOUR_CHAR_CODE('cufa'), 1);										/* Get the cubic face atom */
	if (cufaAtom != NULL) {																					/* If it is there, use it to orient the faces */
		QTVRCubicFaceData *wCufa;
		wCufa = (QTVRCubicFaceData*)QTAtomGetAtomDataPtr(cufaAtom, &size);									/* Get the cubic face data */
		numFaces = size / sizeof(QTVRCubicFaceData);
		FskMemPtrNewFromData(size, wCufa, (FskMemPtr*)(void*)(&cufaScratch));								/* make copy to ensure alignment */
		BAIL_IF_NULL(cufaScratch, err, -1);
		wCufa = cufaScratch;
		BtoNEndianFloat32(wCufa->orientation, size / sizeof(float));										/* Make sure it is the appropriate endian */
		cufa = wCufa;
	}
	else {																									/* If not there, assign the standard orientations */
		cufa = defaultFaceImmersion;
		numFaces = 6;
	}
	*pNumFaces = numFaces;


	/* Read the pixel data of the faces */
	#if __FSK_LAYER__											/* Use kFskBitmapFormat* */
		#if TARGET_OS_ANDROID
			pixelFormat = kFskBitmapFormat16RGB565LE;
		#elif TARGET_OS_LINUX
			pixelFormat = kFskBitmapFormat32BGRA;
		#elif TARGET_OS_MAC
			pixelFormat = kFskBitmapFormat32ARGB;
		#else
			pixelFormat = kFskBitmapFormat24BGR;
		#endif
	#else /* !__FSK_LAYER__ */									/* Use kpFormat* */
		#if TARGET_OS_ANDROID
			pixelFormat = kpFormat16RGB565LE;
		#elif TARGET_OS_LINUX
			pixelFormat = kpFormat32BGRA;
		#elif TARGET_OS_MAC
			pixelFormat = kpFormat32ARGB;
		#else
			pixelFormat = kpFormat24BGR;
		#endif
	#endif /* __FSK_LAYER__ */

	/* Allocate faces */
	do {
		BAIL_IF_ERR(err = (*allocProc)(fRef, 1, sizeof(PanoFaces) + (numFaces - 1) * sizeof(KPImage3D), (void**)(void*)(&panoFaces)));	/* calloc */
		panoFaces->numFaces		= numFaces;
		panoFaces->freeProc		= freeProc;
		panoFaces->readProc		= readProc;		/* We probably don't need this */
		panoFaces->allocProc	= allocProc;	/* We probably don't need this */

		if (progressProc) (progressProc)(fRef, 0);

		/* Read the pixel data of the faces */
		for (i = 0, f = panoFaces->faces; i < numFaces; i++, f++) {
			err = InitImage3DFromVideoTrack(videoTrack, i+1, pixelFormat, scaleShift, freeProc, f);
			if (err) {
				KPDeleteCubicPanorama(panoFaces->faces);
				panoFaces = NULL;
				break;
			}
			if (progressProc) (*progressProc)(fRef, (UInt16)(((i + 1) * 100) / numFaces));
		}
		scaleShift += 1;
	} while ((scaleShift < 4) && (NULL == panoFaces));

	if (NULL == panoFaces) {
		err = -1;
		goto bail;
	}

	*faces = panoFaces->faces;

	/* Set the immersion data of the faces, using the dimenmsions of the actual faces */
	for (i = numFaces, f = panoFaces->faces; i--; f++, cufa++) {
		KPQTVRCubicFaceDataToMatrix(cufa, f->width, f->height, f->M);
	}

bail:
	if (err != 0) {		KPDeleteCubicPanorama(*faces);
						*faces     = NULL;
						*pNumFaces = 0;
	}
	if (panoSample!= NULL)	(freeProc)(fRef, panoSample);
	if (disposeMovie)		QTMovieDispose(movie);
	FskMemPtrDispose(cufaScratch);
	FskMemPtrDispose(pdat);

	return err;
}


/********************************************************************************
 * KPDeleteCubicPanorama
 ********************************************************************************/

void
KPDeleteCubicPanorama(KPImage3D *faces)
{
	PanoFaces		*panoFaces;
	long			numFaces;

	if (faces != NULL) {
		panoFaces	= (PanoFaces*)(((char*)faces) - (intptr_t)(((PanoFaces*)NULL)->faces));

		/* Dispose of the GWorlds */
		for (numFaces = panoFaces->numFaces; numFaces--; faces++) {
			if (faces->userData != NULL) {
				#if USE_QUICKTIME
					DisposeGWorld((GWorldPtr)(faces->userData));
				#elif USE_KINOMA_DECODE
					DisposeOffscreen((KinomaBitmapPtr)(faces->userData));
				#elif USE_FSK_DECODE
					FskBitmap bm = (FskBitmap)faces->userData;
					if (NULL != faces->baseAddr)
						FskBitmapReadEnd(bm);
					FskBitmapDispose(bm);
				#endif /* USE_FSK_DECODE*/
			}
		}

		/* Dispose the faces */
		(*panoFaces->freeProc)(panoFaces->refCon, panoFaces);
	}
}


/********************************************************************************
 * KPUpdateBaseAddrOfCubicPanorama
 ********************************************************************************/

void
KPUpdateBaseAddrOfCubicPanorama(KPImage3D *faces)
{
	#if USE_QUICKTIME
		PanoFaces	*panoFaces	= (PanoFaces*)(((char*)faces) - (intptr_t)(((PanoFaces*)NULL)->faces));
		long		numFaces	= panoFaces->numFaces;
		for ( ; numFaces--; faces++)
			faces->baseAddr = GetPixBaseAddr(GetGWorldPixMap((GWorldPtr)(faces->userData)));
	#elif USE_KINOMA_DECODE
	#elif USE_FSK_DECODE
	#else
		#error Unsupported OS
	#endif
}
