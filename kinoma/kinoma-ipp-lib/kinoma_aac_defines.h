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

#ifndef __KINOMA_DEFINE_H__
#define __KINOMA_DEFINE_H__
#include "ipps.h"
#include "ippdc.h"
#include "ippac.h"

#define K_PI     3.14159265358979323846   
#define K_2PI    6.28318530717958647692  /* 2*pi                         */

typedef struct _K_IppsVLCEncodeSpec_32s
{
	unsigned int *startAddr;
	int minValue;
	int order;
	int alloc;
	int flag;  //xSML
	int dummy0, dummy1, dummy2;
}K_IppsVLCEncodeSpec_32s;


// All these Two structure guess form memory ONLY, maybe not correct
/* struct for FFT about functions*/
typedef struct
{
	int sig;			//cSML or dSML
	IppHintAlgorithm hint;
	int sizeWorkBuf;	//16, 32, 64,128,528
	int alloc;			//if initAlloc alloc == 1, if init alloc = 0;

	// Above is correct, we cahnged the follwoing: confired
	// not confirmed
	int len;			// sample number 2^(order-1)

	int *startAddr;


	double *sin;		//need sizeof(double)* (len/2))
	double *cos;		//need sizeof(double)* (len/2))
	int	*ReverseTbl;

	double *pbuff;

	int flagInv;
	int flagFwd;
	double normInv;
	double normFwd;

	int order;	
	int dummy0, dummy1;
}k_IppsFFTSpec_C_32sc, k_IppsFFTSpec_R_32s,k_IppsFFTSpec_C_32sc;


// Different above type
typedef struct
{
	int sig;			//0x03
	IppHintAlgorithm hint;
	int sizeWorkBuf;	//16, 32, 64,128,528
	int alloc;			//if initAlloc alloc == 1, if init alloc = 0;

	int len;			// sample number 2^(order-1)

	int *start_Addr;


	double *sin;		//need sizeof(double)* (len/2))
	double *cos;		//need sizeof(double)* (len/2))
	int	*ReverseTbl;

	double *pbuff;

	int flagInv;
	int flagFwd;
	double normInv;
	double normFwd;

	int order;
	int order1, order2, order3;

}k_IppsFFTSpec_R_16s, k_IppsFFTSpec_C_16s, k_IppsFFTSpec_C_16sc;

#endif
