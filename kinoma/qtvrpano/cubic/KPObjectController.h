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

#ifndef __KPOBJECTCONTROLLER__
#define __KPOBJECTCONTROLLER__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/********************************************************************************
 * Forward declarations
 ********************************************************************************/

struct KPObjectController;	/* Opaque */
typedef struct KPObjectController KPObjectController;


/********************************************************************************
 * Object controller
 ********************************************************************************/

typedef long KPObjectControllerStatus;
#define kpObjectNoConstraint	0
#define kpObjectMinPan		(1 << 0)
#define kpObjectMaxPan		(1 << 1)
#define kpObjectMinTilt		(1 << 2)
#define kpObjectMaxTilt		(1 << 3)
#define kpObjectMinFOV		(1 << 4)
#define kpObjectMaxFOV		(1 << 5)
#define kpObjectMinX		(1 << 6)
#define kpObjectMaxX		(1 << 7)
#define kpObjectMinY		(1 << 8)
#define kpObjectMaxY		(1 << 9)


							/* Constructor & Destructor */
KPObjectController*			KPNewObjectController(void);
void						KPDeleteObjectController(KPObjectController *ctlr);

							/* View Matrix and Frame */
void						KPGetScaleOffsetFrameOfObjectController(const KPObjectController *ctlr, float scale[2], float offset[2], long *frame);

							/* View constraints - set at initialization time */
void						KPSetViewConstraintsOfObjectController(KPObjectController *ctlr, const float *minMaxPanTiltFOV);
void						KPGetViewConstraintsOfObjectController(const KPObjectController *ctlr, float *minMaxPanTiltFOV);

							/* Object Size - set at initialization time */
void						KPSetObjectSizeOfObjectController(KPObjectController *ctlr, const long *widthHeight);
void						KPGetObjectSizeOfObjectController(const KPObjectController *ctlr, long *widthHeight);

							/* Number of rows and columns - set at initialization time */
void						KPSetRowsColumnsOfObjectController(KPObjectController *ctlr, const unsigned long *numRowsColumns);
void						KPGetRowsColumnsOfObjectController(const KPObjectController *ctlr, unsigned long *numRowsColumns);

							/* Window Size - set at initialization time, and with window size changes */
KPObjectControllerStatus	KPSetWindowSizeOfObjectController(KPObjectController *ctlr, const long *widthHeight);
void						KPGetWindowSizeOfObjectController(const KPObjectController *ctlr, long *widthHeight);

							/* Animation settings - set at initialization time, if at all; default is no animation */
void						KPSetAnimationSettingsOfObjectController(KPObjectController *ctlr, long settings);	/* As specified in QuickTimeVRFormat.h */
long						KPGetAnimationSettingsOfObjectController(KPObjectController *ctlr);

							/* Control settings - set at initialization time, if at all; default is kQTVRObjectWrapPanOn|kQTVRObjectCanZoomOn|kQTVRObjectTranslationOn */
void						KPSetControlSettingsOfObjectController(KPObjectController *ctlr, long settings);	/* As specified in QuickTimeVRFormat.h */
long						KPGetControlSettingsOfObjectController(KPObjectController *ctlr);

							/* Pan/tilt speed is given in radians/min(width,height) - set at initialization time if at all; default is pi */
void						KPSetPanSpeedOfObjectController(KPObjectController *ctlr, float velocity);
float						KPGetPanSpeedOfObjectController(KPObjectController *ctlr);

							/* Zoom speed is given in zoomFactor/frame - set at initialization time if at all; default is sqrt(2) */
void						KPSetZoomSpeedOfObjectController(KPObjectController *ctlr, float velocity);
float						KPGetZoomSpeedOfObjectController(KPObjectController *ctlr);

							/* Pan, Tilt, FOV */
KPObjectControllerStatus	KPSetPanTiltFOVOfObjectController(KPObjectController *ctlr, const float *panTiltFOV);
void						KPGetPanTiltFOVOfObjectController(const KPObjectController *ctlr, float *panTiltFOV);

							/* Pan, Tilt */
KPObjectControllerStatus	KPSetPanTiltOfObjectController(KPObjectController *ctlr, const float *panTilt);
void						KPGetPanTiltOfObjectController(const KPObjectController *ctlr, float *panTilt);

							/* Pan */
KPObjectControllerStatus	KPSetPanOfObjectController(KPObjectController *ctlr, float pan);
float						KPGetPanOfObjectController(const KPObjectController *ctlr);

							/* Tilt */
KPObjectControllerStatus	KPSetTiltOfObjectController(KPObjectController *ctlr, float tilt);
float						KPGetTiltOfObjectController(const KPObjectController *ctlr);

							/* FOV */
KPObjectControllerStatus	KPSetFOVOfObjectController(KPObjectController *ctlr, float tilt);
float						KPGetFOVOfObjectController(const KPObjectController *ctlr);

							/* Center */
KPObjectControllerStatus	KPSetCenterOfObjectController(KPObjectController *ctlr, const float *center);
void						KPGetCenterOfObjectController(const KPObjectController *ctlr, float *center);

							/* Drag absolute position: x and y are in units of pixels, z = +1 for zoom in, z = -1 for zoom out, translateMode = {0,1} */
KPObjectControllerStatus	KPDragPositionOfObjectController(KPObjectController *ctlr, const float *xyz, long translateMode);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPOBJECTCONTROLLER__ */


