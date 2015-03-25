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

#if __FSK_LAYER__
	#include "Fsk.h"
#endif

#include "KPMatrix.h"
#include <math.h>


/* Produce procedure names with an "S" for single precision */

#define FLOAT		float
#define FLTPREC		S

#if !defined(USE_SINGLE_PRECISION_TRANSCENDENTALS)
	#if defined(_WIN32)
		#define USE_SINGLE_PRECISION_TRANSCENDENTALS 0
	#elif defined(__ARMCC_VERSION)
		#define USE_SINGLE_PRECISION_TRANSCENDENTALS 0
	#else /* !_WIN32 */
		#define USE_SINGLE_PRECISION_TRANSCENDENTALS 1
	#endif /* _WIN32 */
#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */


/*********************************************************************************
 * Common methods for single and double precision
 *********************************************************************************/

#include "KPMatrix.c"


/*********************************************************************************
 *********************************************************************************
 **							Precision converters								**
 *********************************************************************************
 *********************************************************************************/


/********************************************************************************
 *  KPCopySingleToDoubleVector
 ********************************************************************************/

void
KPCopySingleToDoubleVector(register const float *from, register double *to, register long n)
{
	while (n--)
		*to++ = *from++;
}


/********************************************************************************
 *  KPCopyDoubleToSingleVector
 ********************************************************************************/

void
KPCopyDoubleToSingleVector(register const double *from, register float *to, register long n)
{
	while (n--)
		*to++ = (float)(*from++);
}
