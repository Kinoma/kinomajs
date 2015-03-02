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
#ifndef __FSKCLIPPOLYGON2D__
#define __FSKCLIPPOLYGON2D__

#ifndef __FSKFIXEDMATH__
	#include "FskFixedMath.h"
#endif /* __FSKFIXEDMATH__ */

#ifndef __FSKRECTANGLE__
	#include "FskRectangle.h"
#endif /* __FSKRECTANGLE__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* The point vector should be big enough so that the first vertex can be duplicated beyond the last */
void FskClipPolygon2D(UInt32 nIn, FskFixedPoint2D *vIn, FskConstRectangle clip, UInt32 *nOut, FskFixedPoint2D *vOut);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKCLIPPOLYGON2D__ */

