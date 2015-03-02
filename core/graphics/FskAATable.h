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
#ifndef __FSKAATABLE__
#define __FSKAATABLE__

#ifndef __FSK__
# include "Fsk.h"
#endif /* __FSK__ */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/********************************************************************************
 * There's a lot of tables here, because there are tradeoffs.
 * Blend width:			1.00 is more true, but can yield ropiness.
 *						1.41 has virtually no ropiness, but is a but soft and/or smudgy.
 * Sampling density:	1/32 of a pixel is probably a good compromise.
 * Scale:				256 is best for edges, because we do nothing at 256, but blend at 255.
 *						255 has maximum precision for lines, but requires an alpha blend.
 *						128 uses a simple multiplication, but has 1 bit less precision.
 *
 * Current preferences:
 *			edges:		1.00 blend width, 1/32 pixel sampling, 127 scale
 *			lines:		1.00 blend width, 1/32 pixel sampling, 255 scale
 ********************************************************************************/



/* These are normalized for 255 for precision, but require more effort to multiply */
extern const UInt8 gFskGaussianNarrowLineFilter255_32[48];		/* Width = 1,       inc = 1/32 */
extern const UInt8 gFskGaussRectConvLineFilter255_32[57];		/* Width = 1,       inc = 1/32 */
extern const UInt8 gFskGaussianNarrowPolygonFilter255_32[43];	/* Width = 1,       inc = 1/32 */
extern const UInt8 gFskGaussianWideLineFilter255_32[68];		/* Width = 1.41421, inc = 1/32 */
extern const UInt8 gFskGaussianWidePolygonFilter255_32[60];		/* Width = 1.41421, inc = 1/32 */

/* These are normalized for 127 for precision (also 255 - F), but require more effort to multiply */
extern const UInt8 gFskGaussianNarrowPolygonFilter127_32[40];	/* Width = 1, inc = 1/32 */
extern const UInt8 gFskGaussianWidePolygonFilter127_32[56];	/* Width = 1.41421, inc = 1/32 */

/* These are normalized to 128 for ease of multiplication, but have lower precision */
extern const UInt8 gFskGaussianNarrowLineFilter128_32[46];		/* Width = 1,       inc = 1/32 */
extern const UInt8 gFskGaussianNarrowPolygonFilter128_32[40];	/* Width = 1,       inc = 1/32 */
extern const UInt8 gFskGaussianWideLineFilter128_32[65];		/* Width = 1.41421, inc = 1/32 */
extern const UInt8 gFskGaussianWidePolygonFilter128_32[56];		/* Width = 1.41421, inc = 1/32 */

/* These are normalized to 256 for ease of multiplication and higher precision, but are available only for polygons, not lines */
extern const UInt8 gFskGaussianNarrowPolygonFilter256_32[43];	/* Width = 1,       inc = 1/32 */
extern const UInt8 gFskGaussianWidePolygonFilter256_32[60];		/* Width = 1.41421, inc = 1/32 */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKAATABLE__ */

