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
#ifndef __FSKCLIPLINE2D__
#define __FSKCLIPLINE2D__

#ifndef __FSKFIXEDMATH__
# include "FskFixedMath.h"
#endif /* __FSKFIXEDMATH__ */

#ifndef __FSKRECTANGLE__
	#include "FskRectangle.h"
#endif /* __FSKRECTANGLE__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* This returns true if the line is visible, and clips the endpoints as appropriate */
long	FskClipLine2D(FskConstRectangle clipRect, FskFixedPoint2D *p0, FskFixedPoint2D *p1);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKCLIPLINE2D__ */


