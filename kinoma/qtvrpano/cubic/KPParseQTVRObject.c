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

#ifndef QT_READER_QTATOM
	#define QT_READER_QTATOM 1
#endif /* QT_READER_QTATOM */

#ifndef QT_READER_TRACKREF
	#define QT_READER_TRACKREF 1
#endif /* QT_READER_TRACKREF */

#include "QTReader.h"
#include "KPParseQTVRObject.h"
#include "KPObjectController.h"

#if _WIN32
# include <stddef.h>
#else /* !_WIN32 */
# include <stdint.h>
#endif /* !WIN32 */


/********************************************************************************
 ********************************************************************************
 *****							System Dependencies							*****
 ********************************************************************************
 ********************************************************************************/


/* Get QuickTime VR data structures, either from QuickTimeVRFormat.h or inline */
#if !__FSK_LAYER__
	#include <QuickTimeVRFormat.h>
#else /* __FSK_LAYER__ */
	typedef unsigned long OSType;
	typedef float Float32;


	/********************************************************************************
	 * Panorama File Format 2.0
	 ********************************************************************************/

	struct QTVRCubicFaceData {
	    Float32                         orientation[4];             /* WXYZ quaternion of absolute orientation*/
	    Float32                         center[2];                  /* Center of image relative to center of projection (default = (0,0)) in normalized units*/
	    Float32                         aspect;                     /* aspect>1 => tall pixels; aspect <1 => squat pixels (default = 1)*/
	    Float32                         skew;                       /* skew x by y (default = 0)*/
	};
	typedef struct QTVRCubicFaceData        QTVRCubicFaceData;
	typedef QTVRCubicFaceData *             QTVRCubicFaceDataPtr;

	struct QTVRPanoSampleAtom {
	    UInt16                          majorVersion;
	    UInt16                          minorVersion;

	    UInt32                          imageRefTrackIndex;         /* track reference index of the full res image track*/
	    UInt32                          hotSpotRefTrackIndex;       /* track reference index of the full res hot spot track*/

	    Float32                         minPan;
	    Float32                         maxPan;
	    Float32                         minTilt;
	    Float32                         maxTilt;
	    Float32                         minFieldOfView;
	    Float32                         maxFieldOfView;

	    Float32                         defaultPan;
	    Float32                         defaultTilt;
	    Float32                         defaultFieldOfView;

	                                                                /* Info for highest res version of image track*/
	    UInt32                          imageSizeX;                 /* pixel width of the panorama (e.g. 768)*/
	    UInt32                          imageSizeY;                 /* pixel height of the panorama (e.g. 2496)*/
	    UInt16                          imageNumFramesX;            /* diced frames wide (e.g. 1)*/
	    UInt16                          imageNumFramesY;            /* diced frames high (e.g. 24)*/

	                                                                /* Info for highest res version of hotSpot track*/
	    UInt32                          hotSpotSizeX;               /* pixel width of the hot spot panorama (e.g. 768)*/
	    UInt32                          hotSpotSizeY;               /* pixel height of the hot spot panorama (e.g. 2496)*/
	    UInt16                          hotSpotNumFramesX;          /* diced frames wide (e.g. 1)*/
	    UInt16                          hotSpotNumFramesY;          /* diced frames high (e.g. 24)*/

	    UInt32                          flags;
	    OSType                          panoType;
	    UInt32                          reserved2;
	};
	typedef struct QTVRPanoSampleAtom       QTVRPanoSampleAtom;
	typedef QTVRPanoSampleAtom *            QTVRPanoSampleAtomPtr;

	struct QTVRCubicViewAtom {
	    Float32                         minPan;
	    Float32                         maxPan;
	    Float32                         minTilt;
	    Float32                         maxTilt;
	    Float32                         minFieldOfView;
	    Float32                         maxFieldOfView;

	    Float32                         defaultPan;
	    Float32                         defaultTilt;
	    Float32                         defaultFieldOfView;
	};
	typedef struct QTVRCubicViewAtom        QTVRCubicViewAtom;
	typedef QTVRCubicViewAtom *             QTVRCubicViewAtomPtr;
	typedef struct QTVRTrackRefEntry        QTVRTrackRefEntry;


	/********************************************************************************
	 * Object File Format 2.0
	 ********************************************************************************/

	enum {
		kQTVRObjectAnimateViewFramesOn		= (1L << 0),
		kQTVRObjectPalindromeViewFramesOn	= (1L << 1),
		kQTVRObjectStartFirstViewFrameOn	= (1L << 2),
		kQTVRObjectAnimateViewsOn			= (1L << 3),
		kQTVRObjectPalindromeViewsOn		= (1L << 4),
		kQTVRObjectSyncViewToFrameRate		= (1L << 5),
		kQTVRObjectDontLoopViewFramesOn		= (1L << 6),
		kQTVRObjectPlayEveryViewFrameOn		= (1L << 7),
		kQTVRObjectStreamingViewsOn			= (1L << 8)
	};

	enum {
		kQTVRObjectWrapPanOn				= (1L << 0),
		kQTVRObjectWrapTiltOn				= (1L << 1),
		kQTVRObjectCanZoomOn				= (1L << 2),
		kQTVRObjectReverseHControlOn		= (1L << 3),
		kQTVRObjectReverseVControlOn		= (1L << 4),
		kQTVRObjectSwapHVControlOn			= (1L << 5),
		kQTVRObjectTranslationOn			= (1L << 6)
	};

	enum {
		kGrabberScrollerUI					= 1,    /* "Object" */
		kOldJoyStickUI						= 2,    /*  "1.0 Object as Scene"     */
		kJoystickUI							= 3,    /* "Object In Scene"*/
		kGrabberUI							= 4,    /* "Grabber only"*/
		kAbsoluteUI							= 5     /* "Absolute pointer"*/
	};


	struct QTVRObjectSampleAtom {
		UInt16              majorVersion;           /* kQTVRMajorVersion*/
		UInt16              minorVersion;           /* kQTVRMinorVersion*/
		UInt16              movieType;              /* ObjectUITypes*/
		UInt16              viewStateCount;         /* The number of view states 1 based*/
		UInt16              defaultViewState;       /* The default view state number. The number must be 1 to viewStateCount*/
		UInt16              mouseDownViewState;     /* The mouse down view state.   The number must be 1 to viewStateCount*/
		UInt32              viewDuration;           /* The duration of each view including all animation frames in a view*/
		UInt32              columns;                /* Number of columns in movie*/
		UInt32              rows;                   /* Number rows in movie*/
		Float32             mouseMotionScale;       /* 180.0 for kStandardObject or kQTVRObjectInScene, actual degrees for kOldNavigableMovieScene.*/
		Float32             minPan;                 /* Start   horizontal pan angle in degrees*/
		Float32             maxPan;                 /* End     horizontal pan angle in degrees*/
		Float32             defaultPan;             /* Initial horizontal pan angle in degrees (poster view)*/
		Float32             minTilt;                /* Start   vertical   pan angle in degrees*/
		Float32             maxTilt;                /* End     vertical   pan angle in degrees*/
		Float32             defaultTilt;            /* Initial vertical   pan angle in degrees (poster view)  */
		Float32             minFieldOfView;         /* minimum field of view setting (appears as the maximum zoom effect) must be >= 1*/
		Float32             fieldOfView;            /* the field of view range must be >= 1*/
		Float32             defaultFieldOfView;     /* must be in minFieldOfView and maxFieldOfView range inclusive*/
		Float32             defaultViewCenterH;
		Float32             defaultViewCenterV;

		Float32             viewRate;
		Float32             frameRate;
		UInt32              animationSettings;      /* 32 reserved bit fields*/
		UInt32              controlSettings;        /* 32 reserved bit fields*/
	};
	typedef struct QTVRObjectSampleAtom     QTVRObjectSampleAtom;
	typedef QTVRObjectSampleAtom *          QTVRObjectSampleAtomPtr;

#endif /* __FSK_LAYER__ */


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
# if TARGET_OS_MAC || TARGET_OS_WIN32
#  define USE_QUICKTIME	1
# else /* !(TARGET_OS_MAC || TARGET_OS_WIN32) */
#  define USE_QUICKTIME	0
# endif /* !(TARGET_OS_MAC || TARGET_OS_WIN32) */
#endif /* USE_QUICKTIME */

#ifndef USE_KINOMA_DECODE
# define USE_KINOMA_DECODE	0
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

#define noMemErr			-1
#define notCubicQTVRErr		-2
#define notObjectQTVRErr	-3



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


/********************************************************************************
 * ConvertArrayFromDegreesToRadians
 ********************************************************************************/

static void
ConvertArrayFromDegreesToRadians(register float *a, register long n)
{
	for ( ; n--; a++)
		*a *= (float)(kpRadiansPerDegree);
}


/********************************************************************************
 * KPNewInteractiveObjectMovieFromFile
 ********************************************************************************/

long
KPNewInteractiveObjectMovieFromFile(
	QTMovie				movie,				/* If NULL, it is opened and closed herein */
	void				*fRef,				/* File reference */
	QTMovieReadProc		readProc,			/* Read proc */
	QTMovieAllocProc	allocProc,			/* Alloc proc */
	QTMovieFreeProc		freeProc,			/* Free proc */
	KPProgressProc		progressProc,		/* Progress proc */

	SInt32				nodeIndex,			/* The desired node */

	UInt32				*numCols,			/* The number of columns */
	UInt32				*numRows,			/* The number of rows */
	QTTrack				*videoTrack,		/* The track that holds the video */
	UInt32				*startTime,			/* The start time of the video track samples */
	UInt32				*sampleDuration,	/* The duration of each frame */

	KPObjectController	*ctlr				/* If not NULL, sets the port width & height, pan, tilt, & fov */
)
{
	long					err				= 0;
	long					disposeMovie	= 0;
	QTTrack					objeTrack		= NULL;
	QTTrack					videTrack		= NULL;
	unsigned char			*objeSample		= NULL;
	void					*objeAtom, *objiAtom;
	QTVRObjectSampleAtom	*objiData;
	long					portWidthHeight[2];
	UInt32					size;
	UInt32					nodeTime, nodeDuration;

	if (nodeIndex <= 0)
		nodeIndex = 1;

	/* Open a movie if there is none */
	if (movie == NULL) {
		BAIL_IF_ERR(err = QTMovieNewFromReader(&movie, false, readProc, fRef, allocProc, freeProc, fRef));	/* Initialize a QuiockTime reader */
		disposeMovie = 1;
	}

//	BAIL_IF_NULL((qtvrTrack = QTMovieGetIndTrackType(movie, FOUR_CHAR_CODE('qtvr'), 1)), err, notObjectQTVRErr);	/* Get qtvr track -- not really necessary */
	BAIL_IF_NULL((objeTrack = QTMovieGetIndTrackType(movie, FOUR_CHAR_CODE('obje'), 1)), err, notObjectQTVRErr);	/* Get obje track */
	portWidthHeight[0] = objeTrack->width;				//	GetTrackDimensions(panoTrack, &portWidth, &portHeight);
	portWidthHeight[1] = objeTrack->height;
	BAIL_IF_ERR(err = QTTrackLoadSample(objeTrack, nodeIndex, (void**)(void *)(&objeSample), &size));				/* Get obje sample */
	objeAtom = QTAtomGetRootAtom(objeSample);																		/* Get obje root atom */
	objiAtom = QTAtomGetAtomByID(objeAtom, FOUR_CHAR_CODE('obji'), 1);												/* Get the object info atom */
	objiData = (QTVRObjectSampleAtom*)QTAtomGetAtomDataPtr(objiAtom, NULL);											/* Get the object data */
	BtoNEndianInt16(&objiData->majorVersion, 6);																	/* Perform endian swapping if necessary */
	BtoNEndianInt32(&objiData->viewDuration, 3);
	BtoNEndianFloat32(&objiData->mouseMotionScale, 14);
	BtoNEndianInt32(&objiData->animationSettings, 2);
	ConvertArrayFromDegreesToRadians(&objiData->minPan, 9);
	BAIL_IF_NULL((videTrack = QTTrackGetReference(objeTrack, FOUR_CHAR_CODE('imgt'), 1)), err, notObjectQTVRErr);	/* Get video track */
	BAIL_IF_ERR(err = QTTrackGetSampleTemporalInfo(objeTrack, nodeIndex, &nodeTime, NULL, &nodeDuration));


	if (numRows)		*numRows		= objiData->rows;
	if (numCols)		*numCols		= objiData->columns;
	if (videoTrack)		*videoTrack		= videTrack;
	if (startTime)		*startTime		= nodeTime;
	if (sampleDuration)	*sampleDuration	= nodeDuration / (objiData->rows * objiData->columns);

	if (ctlr != NULL) {
		float			params[6];
		unsigned long	numRowsCols[2];
		long			objWidthHeight[2];

		/* Set window size */
		KPSetWindowSizeOfObjectController(ctlr, portWidthHeight);

		/* Set the number of rows and columns */
		numRowsCols[0] = objiData->rows;
		numRowsCols[1] = objiData->columns;
		KPSetRowsColumnsOfObjectController(ctlr, numRowsCols);

		objWidthHeight[0] = videTrack->width;
		objWidthHeight[1] = videTrack->height;
		KPSetObjectSizeOfObjectController(ctlr, objWidthHeight);


		/* Set media limits */
		params[0]		= objiData->minPan;
		params[1]		= objiData->maxPan;
		if (objiData->minTilt <= objiData->maxTilt) {	/* There seems to be two definitions of what min and max mean */
			params[2]	= objiData->minTilt;
			params[3]	= objiData->maxTilt;
		}
		else {
			params[2]	= objiData->maxTilt;
			params[3]	= objiData->minTilt;
		}
		params[4]		= objiData->minFieldOfView;
		params[5]		= objiData->fieldOfView;
		KPSetViewConstraintsOfObjectController(ctlr, params);

		/* Set pan, tilt, field of view */
		params[0] = objiData->defaultPan;
		params[1] = objiData->defaultTilt;
		params[2] = objiData->defaultFieldOfView;
		KPSetPanTiltFOVOfObjectController(ctlr, params);

		/* Set center */
		params[0] = objiData->defaultViewCenterH;
		params[1] = objiData->defaultViewCenterV;
		KPSetCenterOfObjectController(ctlr, params);

		/* Set pan speed (radians per min(objectWidth, objectHeight)) */
		KPSetPanSpeedOfObjectController(ctlr, objiData->mouseMotionScale);

		/* Set animation properties */
		KPSetAnimationSettingsOfObjectController(ctlr, objiData->animationSettings);

		/* Set control porperties */
		KPSetControlSettingsOfObjectController(ctlr, objiData->controlSettings);
	}

	if (progressProc) (progressProc)(fRef, 100);

bail:
	if (objeSample != NULL)	(freeProc)(fRef, objeSample);
	if (disposeMovie)		QTMovieDispose(movie);

	return err;
}

