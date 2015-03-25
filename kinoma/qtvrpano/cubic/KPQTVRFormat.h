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

#ifndef __KPQTVRFORMAT__
#define __KPQTVRFORMAT__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



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



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPQTVRFORMAT__ */
