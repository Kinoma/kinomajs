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

#include "KPTypeOfQTMovie.h"


/* Get QuickTime VR data structures, either from QuickTimeVRFormat.h or inline */
#include <QuickTimeVRFormat.h>


#ifndef FOUR_CHAR_CODE
# if TARGET_RT_BIG_ENDIAN
#  define FOUR_CHAR_CODE(x)	(x)
# else /* TARGET_RT_LITTLE_ENDIAN */
#  define FOUR_CHAR_CODE(x)       (((unsigned long) ((x) & 0x000000FF)) << 24) | (((unsigned long) ((x) & 0x0000FF00)) << 8) | (((unsigned long) ((x) & 0x00FF0000)) >> 8) | (((unsigned long) ((x) & 0xFF000000)) >> 24)
# endif /* TARGET_RT_LITTLE_ENDIAN */
#endif /* FOUR_CHAR_CODE */



/********************************************************************************
 * KPTypeOfQTMovie
 ********************************************************************************/

long
KPTypeOfQTMovie(
	void				*fRef,		/* File reference */
	QTMovieReadProc		readProc,	/* Read proc */
	QTMovieAllocProc	allocProc,	/* Alloc proc */
	QTMovieFreeProc		freeProc	/* Free proc */
)
{
	QTMovie	movie			= NULL;
	long	nodeIndex		= 1;
	long	type;
	
	if ((type = QTMovieNewFromReader(&movie, false, readProc, fRef, allocProc, freeProc, fRef)) == 0) {		/* A movie */
		QTTrack	qtvrTrack;
		if ((qtvrTrack = QTMovieGetIndTrackType(movie, FOUR_CHAR_CODE('qtvr'), 1)) != NULL) {				/* A quickTime VR movie */
			QTTrack	panoTrack = NULL;
			QTTrack	objeTrack = NULL;
			if ((panoTrack = QTMovieGetIndTrackType(movie, FOUR_CHAR_CODE('pano'), 1)) != NULL) {			/* A panorama movie */
				unsigned char	*panoSample = NULL;
				UInt32			size;
				if ((type = QTTrackLoadSample(panoTrack, nodeIndex, (void**)(&panoSample), &size)) == 0) {
					void				*panoAtom, *pdatAtom;
					QTVRPanoSampleAtom	*pdat;
					panoAtom = QTAtomGetRootAtom(panoSample);
					pdatAtom = QTAtomGetAtomByID(panoAtom, FOUR_CHAR_CODE('pdat'), 1);
					pdat     = (QTVRPanoSampleAtom*)QTAtomGetAtomDataPtr(pdatAtom, NULL);
					switch (pdat->panoType) {
						case 0:
						case FOUR_CHAR_CODE('vcyl'):														/* A rotated cylindrical panorama movie */
							type = kpVCylPanoType;
							break;
						case FOUR_CHAR_CODE('hcyl'):														/* A non-rotated cylindrcal panorama movie */
							type = kpHCylPanoType;
							break;
						case FOUR_CHAR_CODE('cube'):														/* A cubic panorama movie */
							type = kpCubicPanoType;
							break;
						default:																			/* An unknown type of QTVR panorama movie */
							type = kpUnknownQTVRType;
							break;
					}
					(freeProc)(fRef, panoSample);
				}
			}
			else if ((objeTrack = QTMovieGetIndTrackType(movie, FOUR_CHAR_CODE('obje'), 1)) != NULL) {		/* An object movie */
				type = kpObjectType;
			}
			else {
				type = kpUnknownQTVRType;																	/* An unknown QTVR movie */
			}
		}
		else if ((qtvrTrack = QTMovieGetIndTrackType(movie, FOUR_CHAR_CODE('STpn'), 1)) != NULL) {			/* An old-style QTVR panorama movie */
			type = kpQTVROldPanoType;
		}
		else if ((qtvrTrack = QTMovieGetIndTrackType(movie, FOUR_CHAR_CODE('stna'), 1)) != NULL) {			/* An old-style QTVR object movie */
			type = kpQTVROldObjectType;
		}
		else {																								/* Probably a linear movie */
			/* Look at the controller */
			UInt32 controllerTypeTag, controllerType, controllerTypeSize;
			unsigned char *controllerTypeStart;
			
			controllerTypeTag = FOUR_CHAR_CODE('ctyp');
			if (QTUserDataGet(movie->userData, controllerTypeTag, 1, (void**)(&controllerTypeStart), &controllerTypeSize) == 0) {	/* Found it */
				((unsigned char*)(&controllerType))[0] = controllerTypeStart[0];
				((unsigned char*)(&controllerType))[1] = controllerTypeStart[1];
				((unsigned char*)(&controllerType))[2] = controllerTypeStart[2];
				((unsigned char*)(&controllerType))[3] = controllerTypeStart[3];
				switch (controllerType) {
					case 'STpn':	/* Old panorama type */
						type = kpQTVROldPanoType;
						break;
					case 'stna':	/* Old object type */
						type = kpQTVROldObjectType;
						break;
					case 'qtvr':	/* QTVR */
					case 'pano':	/* Panorama */
					case 'obje':	/* Object */
						type = kpUnknownQTVRType;	/* These should have been detected by looking for 'pano' and 'obje' tracks */
						break;
					default:
						type = kpLinMovType;		/* No recognized controllers -- it could be a standard linear movie */
						break;
				}
			}
			else {
				type = kpLinMovType;				/* No recognized tracks, no recognized controller: it must be a linear movie */
			}
		}
		QTMovieDispose(movie);
	}
	else {																									/* Not a movie at all */
		type = kpNotMovType;
	}

	return type;
}


