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

#ifndef __KPQTVRCONTROLLER__
#define __KPQTVRCONTROLLER__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/********************************************************************************
 * Forward declarations
 ********************************************************************************/

struct QTVRCubicViewAtom;	/* Get it from <QuickTimeVRFormat.h>, if you need it */
struct KPPanoController;	/* Opaque */
typedef struct KPPanoController KPPanoController;



/********************************************************************************
 * Panorama controller
 ********************************************************************************/

typedef long KPPanoControllerStatus;
#define kpPanoNoConstraint	0
#define kpPanoMinPan		(1 << 0)
#define kpPanoMaxPan		(1 << 1)
#define kpPanoMinTilt		(1 << 2)
#define kpPanoMaxTilt		(1 << 3)
#define kpPanoMinFOV		(1 << 4)
#define kpPanoMaxFOV		(1 << 5)


						/* Constructor & Destructor */
KPPanoController*		KPNewPanoController(void);
void					KPDeletePanoController(KPPanoController *ctlr);

						/* Initialization from QTVR cubic view atom */
void					KPQTVRCubicViewAtomInitPanoController(KPPanoController *ctlr, const struct QTVRCubicViewAtom *cuvw);

						/* View Constraints - set at initialization time */
void					KPSetViewConstraintsOfPanoController(KPPanoController *ctlr, const float *minMaxPanTiltFOV);
void					KPGetViewConstraintsOfPanoController(const KPPanoController *ctlr, float *minMaxPanTiltFOV);

						/* Window Size - set at initialization time */
KPPanoControllerStatus	KPSetWindowSizeOfPanoController(KPPanoController *ctlr, const long *widthHeight);
void					KPGetWindowSizeOfPanoController(const KPPanoController *ctlr, long *widthHeight);

						/* Orientation = {0, 1, 2, 3} for {0, 90, 180, 270} degree roll, initialized to 0 */
KPPanoControllerStatus	KPSetOrientationOfPanoController(KPPanoController *ctlr, long orientation);
long					KPGetOrientationOfPanoController(const KPPanoController *ctlr);

						/* View Matrix */
void					KPGetViewMatrixOfPanoController(const KPPanoController *ctlr, float V[3][3]);

						/* Reset to the state set from the file */
void					KPResetPanoController(KPPanoController *ctlr);

						/* Pan, Tilt, FOV */
KPPanoControllerStatus	KPSetPanTiltFOVOfPanoController(KPPanoController *ctlr, const float *panTiltFOV);
void					KPGetPanTiltFOVOfPanoController(const KPPanoController *ctlr, float *panTiltFOV);

						/* Pan, Tilt */
KPPanoControllerStatus	KPSetPanTiltOfPanoController(KPPanoController *ctlr, const float *panTilt);
void					KPGetPanTiltOfPanoController(const KPPanoController *ctlr, float *panTilt);

						/* Pan */
KPPanoControllerStatus	KPSetPanOfPanoController(KPPanoController *ctlr, float pan);
float					KPGetPanOfPanoController(const KPPanoController *ctlr);

						/* Tilt */
KPPanoControllerStatus	KPSetTiltOfPanoController(KPPanoController *ctlr, float tilt);
float					KPGetTiltOfPanoController(const KPPanoController *ctlr);

						/* FOV */
KPPanoControllerStatus	KPSetFOVOfPanoController(KPPanoController *ctlr, float tilt);
float					KPGetFOVOfPanoController(const KPPanoController *ctlr);

						/* Focal Length */
KPPanoControllerStatus	KPSetFocalLengthOfPanoController(KPPanoController *ctlr, float focalLength);
float					KPGetFocalLengthOfPanoController(const KPPanoController *ctlr);

						/* Pan/tilt speed is given in pixels/sec/pixel; default is 2 */
void					KPSetPanSpeedOfPanoController(KPPanoController *ctlr, float velocity);
float					KPGetPanSpeedOfPanoController(KPPanoController *ctlr);

						/* Zoom speed is given in zoomFactor/sec; default is sqrt(2) */
void					KPSetZoomSpeedOfPanoController(KPPanoController *ctlr, float velocity);
float					KPGetZoomSpeedOfPanoController(KPPanoController *ctlr);

						/* Drag velocity, as in QTVR: x and y are in units of pixels, z = +1 for zoom in, z = -1 for zoom out */
KPPanoControllerStatus	KPDragVelocityOfPanoController(KPPanoController *ctlr, const float *xyz, float framePeriod);

						/* Drag absolute position: x and y are in units of pixels, z = +1 for zoom in, z = -1 for zoom out */
KPPanoControllerStatus	KPDragPositionOfPanoController(KPPanoController *ctlr, const float *xyz);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPQTVRCONTROLLER__ */


