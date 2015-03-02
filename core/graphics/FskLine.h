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
#ifndef __FSKLINE__
#define __FSKLINE__

#ifndef __FSKFIXEDMATH__
# include "FskFixedMath.h"
#endif /* __FSKFIXEDMATH__ */

#ifndef __FSKGRAPHICS_H__
# include "FskGraphics.h"
#endif /* __FSKGRAPHICS_H__ */

#ifndef __FSKBITMAP__
# include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifndef __FSKRECTANGLE__
	#include "FskRectangle.h"
#endif /* __FSKRECTANGLE__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void FskDrawClippedJaggedLine(const FskFixedPoint2D *p0, const FskFixedPoint2D *p1, FskConstColorRGB color, FskBitmap dstBM);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKLINE__ */


