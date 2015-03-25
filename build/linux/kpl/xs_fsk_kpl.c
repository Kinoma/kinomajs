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
#include "xs_fsk_kpl.h"

//unsigned long nan[2]={0xffffffff, 0x7fffffff};
//unsigned long infinity[2]={0x00000000, 0x7ff00000};

#if 0
int fpclassify(double x)
{
	int result = FP_NORMAL;
	switch (_fpclass(x)) {
	case _FPCLASS_SNAN:  
	case _FPCLASS_QNAN:  
		result = FP_NAN;
		break;
	case _FPCLASS_NINF:  
	case _FPCLASS_PINF:  
		result = FP_INFINITE;
		break;
	case _FPCLASS_NZ: 
	case _FPCLASS_PZ:  
		result = FP_ZERO;
		break;
	case _FPCLASS_ND:  
	case _FPCLASS_PD: 
		result = FP_SUBNORMAL;
		break;
	}
	return result;
}
#endif

