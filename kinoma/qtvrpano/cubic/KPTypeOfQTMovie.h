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

#ifndef __KPTYPEOFQTMOIVIE__
#define __KPTYPEOFQTMOIVIE__

#ifndef __QTREADER__
# include "QTReader.h"
#endif /* __QTREADER__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#define	kpNotMovType		('!mov' | 0x80000000)	/* Negative indicates error */
#define kpUnknownQTVRType	('!qvr' | 0x80000000)	/* Negative indicates error */
#define kpVCylPanoType		('vcyl')
#define kpHCylPanoType		('hcyl')
#define kpCubicPanoType		('cube')
#define kpObjectType		('obje')
#define kpLinMovType		('LMov')
#define kpQTVROldPanoType	('STpn')				/* Used in QTVR 1.0 release*/
#define kpQTVROldObjectType	('stna')				/* Used in QTVR 1.0 release*/



long	KPTypeOfQTMovie(void *fRef, QTMovieReadProc readProc, QTMovieAllocProc allocProc, QTMovieFreeProc freeProc);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPTYPEOFQTMOIVIE__ */
