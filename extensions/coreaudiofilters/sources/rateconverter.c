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
#define __FSKAUDIOFILTER_PRIV__

#include "Fsk.h"

#if TARGET_RT_FPU
	#define USE_NEW_RATE_CONVERTER 1
#endif

#include "FskAudioFilter.h"
#include "FskAudio.h"
#include "FskExtensions.h"
#include "FskUtilities.h"

Boolean	rateConverterCanHandle(const char *filterType);
FskErr rateConverterNew(FskAudioFilter filter, void **filterState);
FskErr rateConverterDispose(FskAudioFilter filter, void *stateIn);
FskErr rateConverterGetMaximumBufferSize(FskAudioFilter filter, void *stateIn, UInt32 sampleCount, UInt32 *bufferSize);
FskErr rateConverterStart(FskAudioFilter filter, void *stateIn);
FskErr rateConverterStop(FskAudioFilter filter, void *stateIn);
FskErr rateConverterProcessSamples(FskAudioFilter filter, void *stateIn, void *input, UInt32 inputSampleCount, void *output, UInt32 *outputSampleCount, UInt32 *outputSize);

FskErr rateConverterSetSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord gRateConverterProperties[] = {
	{kFskMediaPropertySampleRate,			kFskMediaPropertyTypeInteger,		NULL,						rateConverterSetSampleRate},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskAudioFilterDispatchRecord gRateConverter = {rateConverterCanHandle, rateConverterNew, rateConverterDispose,
				rateConverterGetMaximumBufferSize, rateConverterStart, rateConverterStop, rateConverterProcessSamples, gRateConverterProperties};


#if USE_NEW_RATE_CONVERTER

#include "rateconverter.h"
#include "math.h"
#include "stdlib.h"

//const static int 	avail_src_hz[13] = {8000, 11025, 12000, 16000, 22050, 24000 , 32000, 44100, 48000, 88200, 96000, 176400, 192000 };
const static double      sampleRatio_44100[13] = {5.512500000000000178e+00, 4.000000000000000000e+00, 3.674999999999999822e+00, 2.756250000000000089e+00, 2.000000000000000000e+00, 1.837499999999999911e+00, 1.378125000000000044e+00, 1.000000000000000000e+00, 9.187499999999999556e-01, 5.000000000000000000e-01, 4.593749999999999778e-01, 2.500000000000000000e-01, 2.296874999999999889e-01};
const static double	       numerator_44100[13] = {3.528000000000000000e+03, 4.000000000000000000e+00, 1.176000000000000000e+03, 3.528000000000000000e+03, 2.000000000000000000e+00, 1.176000000000000000e+03, 3.528000000000000000e+03, 1.000000000000000000e+00, 1.176000000000000000e+03, 1.000000000000000000e+00, 1.176000000000000000e+03, 1.000000000000000000e+00, 1.176000000000000000e+03};
const static double      denominator_44100[13] = {6.400000000000000000e+02, 1.000000000000000000e+00, 3.200000000000000000e+02, 1.280000000000000000e+03, 1.000000000000000000e+00, 6.400000000000000000e+02, 2.560000000000000000e+03, 1.000000000000000000e+00, 1.280000000000000000e+03, 2.000000000000000000e+00, 2.560000000000000000e+03, 4.000000000000000000e+00, 5.120000000000000000e+03};
const static double intpFilterCutoff_44100[13] = {2.083333333333333218e-02, 1.250000000000000000e-01, 2.083333333333333218e-02, 2.083333333333333218e-02, 2.500000000000000000e-01, 2.083333333333333218e-02, 2.083333333333333218e-02, 5.000000000000000000e-01, 1.914062499999999792e-02, 2.500000000000000000e-01, 9.570312499999998959e-03, 1.250000000000000000e-01, 4.785156249999999480e-03};
const static double 		    beta_44100[13] = {9.285765209999999881e-01, 9.285765209999999881e-01, 9.285765209999999881e-01, 9.285765209999999881e-01, 9.285765209999999881e-01, 9.285765209999999881e-01, 9.285765209999999881e-01, 9.285765209999999881e-01, 2.116624861000000024e+00, 2.116624861000000024e+00, 2.116624861000000024e+00, 2.116624861000000024e+00, 2.116624861000000024e+00};
const static double 		  IoBeta_44100[13] = {8.146888213860199457e-01, 8.146888213860199457e-01, 8.146888213860199457e-01, 8.146888213860199457e-01, 8.146888213860199457e-01, 8.146888213860199457e-01, 8.146888213860199457e-01, 8.146888213860199457e-01, 4.039545422103200112e-01, 4.039545422103200112e-01, 4.039545422103200112e-01, 4.039545422103200112e-01, 4.039545422103200112e-01};
const static double 		 fracDiv_44100[13] = {0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00};
const static double 		  intDiv_44100[13] = {3.381000000000000000e+03, 3.000000000000000000e+00, 1.127000000000000000e+03, 3.381000000000000000e+03, 1.000000000000000000e+00, 1.127000000000000000e+03, 3.381000000000000000e+03, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00};
const static long  filterCoeffsTotal_44100[13] = {		   145, 		  25, 		   145,  		  145, 			 13, 		 145,  		   145,  	       7, 		   241,  		  19, 		   481, 		  35, 		   913};
const static long 	     samplesLeft_44100[13] = {           3,            3,            3,            3,            3,            3,            3,            1,            6,           10,           11,           18,           20};

const static double      sampleRatio_48000[13] = {6.000000000000000000e+00, 4.353741496598639849e+00, 4.000000000000000000e+00, 3.000000000000000000e+00, 2.176870748299319924e+00, 2.000000000000000000e+00, 1.500000000000000000e+00, 1.088435374149659962e+00, 1.000000000000000000e+00, 5.442176870748299811e-01, 5.000000000000000000e-01, 2.721088435374149905e-01, 2.500000000000000000e-01};
const static double	       numerator_48000[13] = {6.000000000000000000e+00, 1.920000000000000000e+03, 4.000000000000000000e+00, 3.000000000000000000e+00, 9.600000000000000000e+02, 2.000000000000000000e+00, 3.000000000000000000e+00, 4.800000000000000000e+02, 1.000000000000000000e+00, 2.400000000000000000e+02, 1.000000000000000000e+00, 1.200000000000000000e+02, 1.000000000000000000e+00};
const static double      denominator_48000[13] = {1.000000000000000000e+00, 4.410000000000000000e+02, 1.000000000000000000e+00, 1.000000000000000000e+00, 4.410000000000000000e+02, 1.000000000000000000e+00, 2.000000000000000000e+00, 4.410000000000000000e+02, 1.000000000000000000e+00, 4.410000000000000000e+02, 2.000000000000000000e+00, 4.410000000000000000e+02, 4.000000000000000000e+00};
const static double intpFilterCutoff_48000[13] = {8.333333333333332871e-02, 2.083333333333333218e-02, 1.250000000000000000e-01, 1.666666666666666574e-01, 2.083333333333333218e-02, 2.500000000000000000e-01, 1.666666666666666574e-01, 2.083333333333333218e-02, 5.000000000000000000e-01, 1.133786848072562518e-02, 2.500000000000000000e-01, 5.668934240362812592e-03, 1.250000000000000000e-01};
const static double             beta_48000[13] = {9.285765209999999881e-01, 9.285765209999999881e-01, 9.285765209999999881e-01, 9.285765209999999881e-01, 9.285765209999999881e-01, 9.285765209999999881e-01, 9.285765209999999881e-01, 9.285765209999999881e-01, 9.285765209999999881e-01, 2.116624861000000024e+00, 2.116624861000000024e+00, 2.116624861000000024e+00, 2.116624861000000024e+00};
const static double           IoBeta_48000[13] = {8.146888213860199457e-01, 8.146888213860199457e-01, 8.146888213860199457e-01, 8.146888213860199457e-01, 8.146888213860199457e-01, 8.146888213860199457e-01, 8.146888213860199457e-01, 8.146888213860199457e-01, 8.146888213860199457e-01, 4.039545422103200112e-01, 4.039545422103200112e-01, 4.039545422103200112e-01, 4.039545422103200112e-01};
const static double 		 fracDiv_48000[13] = {0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00};
const static double 		  intDiv_48000[13] = {5.000000000000000000e+00, 1.840000000000000000e+03, 3.000000000000000000e+00, 2.000000000000000000e+00, 9.200000000000000000e+02, 1.000000000000000000e+00, 2.000000000000000000e+00, 4.600000000000000000e+02, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00, 0.000000000000000000e+00};
const static long  filterCoeffsTotal_48000[13] = {			37, 	   	 145, 			25, 		  19, 		   145, 		  13, 			 19, 		 145, 			 7, 		 385, 		    19, 		769, 			35};
const static long 	     samplesLeft_48000[13] = {           3,            3,            3,            3,            3,            3,            3,            3,            1,            9,           10,           17,           18};

static int SetSubFilter( long filterCoeffsTotal, float *filterImpulse,  KaiserRateConverter	*rcPtr )
{
	long	polyOffset;
	long	sfCoeffTotal;
 	long	sfTotal;
 	long	sfCoeffMax;
	long	i, j, m;
	int		err = 0;

	sfTotal = rcPtr->sfTotal;
	
	err = FskMemPtrNewClear( ( sfTotal + 1 ) * sizeof(long) , (FskMemPtr *)&rcPtr->sfOffsetPtr );
	BAIL_IF_ERR( err );

	err = FskMemPtrNewClear( ( sfTotal + 1 ) * sizeof(long) , (FskMemPtr *)&rcPtr->sfCoeffTotalPtr );
	BAIL_IF_ERR( err );

	err = FskMemPtrNewClear( ( sfTotal + 1 ) * sizeof(long) , (FskMemPtr *)&rcPtr->sfCoeffLenPtr );
	BAIL_IF_ERR( err );

	rcPtr->sfCoeffStride = (( filterCoeffsTotal / sfTotal + 16 )>>4)<<4;
	if( rcPtr->sfCoeffStride < 16 )
		rcPtr->sfCoeffStride = 16;

	err = FskMemPtrNewClear( rcPtr->sfCoeffStride * ( sfTotal + 2 ) * sizeof(short) , (FskMemPtr *)&rcPtr->sfCoeffsPtr );
	BAIL_IF_ERR( err );
	
	sfCoeffMax = 0;
	for (i = 0; i < sfTotal; ++i)
	{
		sfCoeffTotal = (filterCoeffsTotal - 1 - i + sfTotal) / sfTotal;
	
		for (polyOffset = 0, j = i; sfCoeffTotal > 0; ++polyOffset, --sfCoeffTotal, j += sfTotal)
		{
			if (filterImpulse[j] != 0.0)
				break;
		}
		
		rcPtr->sfOffsetPtr[i] = polyOffset;
		for (j += (sfCoeffTotal - 1) * sfTotal; sfCoeffTotal > 0; --sfCoeffTotal, j -= sfTotal)
		{
			if (filterImpulse[j] != 0)
				break;
		}

		if (sfCoeffTotal & 0x00000001)
			sfCoeffTotal++;

		rcPtr->sfCoeffTotalPtr[i] = sfCoeffTotal;
		sfCoeffMax = MyMax(sfCoeffTotal, sfCoeffMax);

		for (m = 0, j = i + polyOffset * sfTotal; m < sfCoeffTotal; ++m, j += sfTotal)
		{
			short *thisPtr = rcPtr->sfCoeffsPtr + rcPtr->sfCoeffStride * i;
			
			if( j >= filterCoeffsTotal )
				thisPtr[m] = 0;
			else
				thisPtr[m] = (FloatToFix14(filterImpulse[j] * 0.5));
		}	
 	}
 	
	m = rcPtr->sfCoeffTotalPtr[0];
	for (i = 0; i < m; i++)
	{
		short *thisPtr = rcPtr->sfCoeffsPtr + rcPtr->sfCoeffStride * sfTotal;
		short *thatPtr = rcPtr->sfCoeffsPtr;

		thisPtr[i] = thatPtr[i];
	}	

 	rcPtr->sfCoeffTotalPtr[sfTotal] = rcPtr->sfCoeffTotalPtr[0];	
 	rcPtr->sfOffsetPtr[sfTotal]	  = rcPtr->sfOffsetPtr[0] - 1; 
	rcPtr->sfCoeffMax					 = sfCoeffMax;
	
	for (i = 0; i < sfTotal; ++i)
	{
		if (rcPtr->sfCoeffTotalPtr[i] <= rcPtr->sfCoeffTotalPtr[i+1])
			rcPtr->sfCoeffLenPtr[i] = rcPtr->sfCoeffTotalPtr[i+1];
		else
			rcPtr->sfCoeffLenPtr[i] = rcPtr->sfCoeffTotalPtr[i];
	}
	
bail:
	return err;
}


void SetCoeffs( double fc, double beta, double IoBeta, KaiserRateConverter *rcPtr, long coeffsTotal, float *coeff )
{
	double	T, gain, dT;
	long	i, n, N;

	N		= coeffsTotal - 1;
	gain	= 2.0 * (double)rcPtr->sfTotal * fc;
	T		= 0.5 * N;
	dT		= 1.0 / T;

	for( i = 0, n = N; i <= n; ++i, --n )
	{
		double x = 2.0 * fc * (i - T);
		double xm, val;

		if (x == 0.0)
			val = 1.0;
		else
		{
			xm = fmod(x, 2.0);
			if (x == floor(x))
				val = 0.0;
			else
				val = sin(3.1415926 * xm) / (3.1415926 * x);
		}

		x = ( i - T ) * dT;
		if( x < -1.0 || x > 1.0 )
			break;

		{
			double	xx, xh;
			double	sum = 1.0;
			double	pow = 1.0;
			double	ds	= 1.0;
			long	k	= 0;

			xx = beta * sqrt( 1.0 - x * x ) * sqrt( 1.0 - x * x );
			xh	= 0.5 * xx;
			while ( ds > ( sum * kMin1 ) )
			{
				++k;
				pow = pow * ( xh / k );
				ds  = pow * pow;
				sum = sum + ds;
			}
			coeff[n] = coeff[i] = (float)( gain * val * sum * IoBeta );
		}
	}
}


static int LinearMono16
( 
	LinearRateConverter	*lRCPtr,				
	short				*inputPtr,			
	long				inputSampleCount,	
	short				*outPtr,			
	long				*outputSampleCount	
)
{
	long	convertedSampleCount;
	long	time = 0;
	long	count;
	long	sample;
	short	*i, *o;
	int		err = 0;

	convertedSampleCount = (long)(0.5 + ((float)inputSampleCount * (float)lRCPtr->outputRate) / (float)lRCPtr->inputRate);
	*outputSampleCount = convertedSampleCount;

	o = outPtr;
	i = inputPtr;
	count = convertedSampleCount;

	while (count-- && (time < 65536)) 
	{
		short *sampleP = &i[0];
		unsigned char denom = (unsigned char)(time >> 8);

		sample = lRCPtr->lastSampleLeft * (256 - denom);
		sample += sampleP[0] * denom;
		*o++ = (short)(sample >> 8);

		time += lRCPtr->sampleRateStep;
	}

	count++;

	while (count--) 
	{
		short *sampleP = &i[time >> 16];
		unsigned char denom = (unsigned char)(time >> 8);

		sample = sampleP[-1] * (256 - denom);
		sample += sampleP[0] * denom;
		*o++ = (short)(sample >> 8);

		time += lRCPtr->sampleRateStep;
	}

	lRCPtr->lastSampleLeft = ((short *)inputPtr)[inputSampleCount - 1]; 

	return err;
}


static int LinearStereo16
( 
	LinearRateConverter	*lRCPtr,				
	short			*inputPtr,			
	long			inputSampleCount,	
	short			*outPtr,			
	long			*outputSampleCount	
)
{
	long	convertedSampleCount;
	long	time = 0;
	long	count;
	long	sample;
	short	*i, *o;
	int		err = 0;

	convertedSampleCount = (long)(0.5 + ((float)inputSampleCount * (float)lRCPtr->outputRate) / (float)lRCPtr->inputRate);
	*outputSampleCount = convertedSampleCount;

	o = outPtr;
	i = inputPtr;
	count = convertedSampleCount;

	while (count-- && (time < 65536)) 
	{
		short *sampleP = &i[0];
		unsigned char denom = (unsigned char)(time >> 8);

		// left sample
		sample = lRCPtr->lastSampleLeft * (256 - denom);
		sample += sampleP[0] * denom;
		*o++ = (short)(sample >> 8);

		// right sample
		sample = lRCPtr->lastSampleRight * (256 - denom);
		sample += sampleP[+1] * denom;
		*o++ = (short)(sample >> 8);

		time += lRCPtr->sampleRateStep;
	}

	count++;
	
	while (count--) 
	{
		short *sampleP = &i[(time >> 16) << 1];
		unsigned char denom = (unsigned char)(time >> 8);

		// left sample
		sample = sampleP[-2] * (256 - denom);
		sample += sampleP[0] * denom;
		*o++ = (short)(sample >> 8);

		// right sample
		sample = sampleP[-1] * (256 - denom);
		sample += sampleP[+1] * denom;
		*o++ = (short)(sample >> 8);

		time += lRCPtr->sampleRateStep;
	}

	i = &((short *)inputPtr)[(inputSampleCount - 1) * 2];
	lRCPtr->lastSampleLeft = i[0];
	lRCPtr->lastSampleRight = i[1];

	return err;
}

#define	ONE_STEP_SUM( c, s, i, idx) ( c[i]*s[idx-i] + c[i+1]*s[idx-i-1 ] )

static long KaiserMono16
( 
	KaiserRateConverter	*rcPtr,				
	short			*inputPtr,			
	long			inputSampleCount,	
	short			*outPtr,			
	long			*outputSampleCount	
)
{
	long	usedSampleIn	  = 0;
	long	outputCount       = 0;
	short	*src0BufPtr       = &rcPtr->srcBuf0Ptr[rcPtr->samplesLeft + rcPtr->sfCoeffMax];
	long	inputBufferSize   = inputSampleCount + rcPtr->samplesLeft;
	long	movingIndex		  = rcPtr->sfCoeffMax - 1;
	long	*sfOffsetAry	  = rcPtr->sfOffsetPtr;
	long	*sfCoeffTotalPtr  = rcPtr->sfCoeffTotalPtr;
	double	fracTimeMove	  = rcPtr->fracDiv;
	double	intgTimeMove	  = rcPtr->intDiv;
	double	timeIncrement	  = rcPtr->denominator;
	double	timeSpecification = rcPtr->numerator;
	double	interpolateRatio  = rcPtr->interpolateRatio;
	long	i;
	long	err = 0;

	for (i = 0; i < inputSampleCount; i++)
		src0BufPtr[i] = *inputPtr++;

	src0BufPtr = rcPtr->srcBuf0Ptr + rcPtr->sfCoeffMax;

	while( ( movingIndex < inputBufferSize ) && ( outputCount < kDefaultDstBufSizeInSample ) )
	{
		long   sumL		  = 0;
		double interRatio = (fracTimeMove + intgTimeMove) * interpolateRatio;
		long   sfIdx	  = (long)interRatio;	
		short  distance   = FloatToFix14(interRatio - sfIdx);
		short  *coefPtr;											
		long   thisIdx;

		if( distance >= kMax3 )
			sfIdx++;
		
		coefPtr  = rcPtr->sfCoeffsPtr + sfIdx * rcPtr->sfCoeffStride;		
		thisIdx  = movingIndex - sfOffsetAry[ sfIdx ];
		
		if( distance > kMin3 && distance < kMax3 )
		{
			long   thatIdx		= movingIndex - sfOffsetAry[ sfIdx + 1 ];
			short  *coefNextPtr = coefPtr + rcPtr->sfCoeffStride;							
			long   sumLNext		= 0;				

			for (i = 0; i < rcPtr->sfCoeffLenPtr[sfIdx]; i += 2)
			{
				sumL 	 += ONE_STEP_SUM( coefPtr, 	  src0BufPtr, i, thisIdx );
				sumLNext += ONE_STEP_SUM( coefNextPtr,src0BufPtr, i, thatIdx );
			}
			
			sumL  += (short)(( sumLNext - sumL + 0x00002000) >> 14) * distance;
		}
		else
		{
			coefPtr = rcPtr->sfCoeffsPtr + sfIdx * rcPtr->sfCoeffStride;		
			thisIdx = movingIndex - sfOffsetAry[sfIdx];
			
			for( i = 0; i < sfCoeffTotalPtr[sfIdx]; i += 2 )
				sumL  += ONE_STEP_SUM( coefPtr,  src0BufPtr, i, thisIdx );
		}		

		sumL = (sumL + 0x00001000) >> 13;

		ClipToShort( sumL );
		*outPtr++ = (short)sumL;
		outputCount++;
		
		intgTimeMove += timeIncrement;
		while (intgTimeMove >= timeSpecification)
		{
			movingIndex++;
			intgTimeMove -= timeSpecification;
		}
	}

	usedSampleIn = movingIndex - rcPtr->sfCoeffMax + 1;
	rcPtr->intDiv = intgTimeMove;

	rcPtr->samplesLeft = inputBufferSize - usedSampleIn;
	for (i = 0; i < rcPtr->samplesLeft; i++)
		src0BufPtr[i] = src0BufPtr[i + usedSampleIn];

	*outputSampleCount = outputCount;
	
	return err;
}


static long KaiserStereo16
( 
	KaiserRateConverter	*rcPtr,				
	short			*inputPtr,			
	long			inputSampleCount,	
	short			*outPtr,			
	long			*outputSampleCount	
)
{
	long	usedSampleIn		= 0;
	long	outputCount			= 0;
	short	*src0BufPtr			= &rcPtr->srcBuf0Ptr[rcPtr->samplesLeft + rcPtr->sfCoeffMax];
	short	*src1BufPtr			= &rcPtr->srcBuf1Ptr[rcPtr->samplesLeft + rcPtr->sfCoeffMax];
	long	inputBufferSize		= inputSampleCount + rcPtr->samplesLeft;
	long	movingIndex			= rcPtr->sfCoeffMax - 1;
	long	*sfOffsetAry		= rcPtr->sfOffsetPtr;
	long	*sfCoeffTotalPtr	= rcPtr->sfCoeffTotalPtr;
	double	fracTimeMove		= rcPtr->fracDiv;
	double	intgTimeMove		= rcPtr->intDiv;
	double	timeIncrement		= rcPtr->denominator;
	double	timeSpecification	= rcPtr->numerator;
	double	interpolateRatio	= rcPtr->interpolateRatio;
	long	i;
	long	err = 0;

	for (i = 0; i < inputSampleCount; i++)
	{
		src0BufPtr[i] = *inputPtr++;
		src1BufPtr[i] = *inputPtr++;
	}

	src0BufPtr = rcPtr->srcBuf0Ptr + rcPtr->sfCoeffMax;
	src1BufPtr = rcPtr->srcBuf1Ptr + rcPtr->sfCoeffMax;

	while( ( movingIndex < inputBufferSize ) && ( outputCount < kDefaultDstBufSizeInSample ) )
	{
		long   sumL		  = 0;
		long   sumR		  = 0;
		double interRatio = (fracTimeMove + intgTimeMove) * interpolateRatio;
		long   sfIdx	  = (long)interRatio;		
		short  distance   = FloatToFix14(interRatio - sfIdx);
		short  *coefPtr;											
		long   thisIdx;
		
		if( distance >= kMax3 )
			sfIdx++;
			
		coefPtr = rcPtr->sfCoeffsPtr + sfIdx * rcPtr->sfCoeffStride;		
		thisIdx = movingIndex - sfOffsetAry[sfIdx];
		
		if( distance > kMin3 && distance < kMax3 )
		{
			long   thatIdx		= movingIndex - sfOffsetAry[sfIdx+1];
			short  *coefNextPtr = coefPtr + rcPtr->sfCoeffStride;			
			long   sumLNext		= 0;				
			long   sumRNext		= 0;				
			
			for (i = 0; i < rcPtr->sfCoeffLenPtr[sfIdx]; i += 2)
			{
				sumL      += ONE_STEP_SUM( coefPtr,     src0BufPtr, i, thisIdx );
				sumR  	  += ONE_STEP_SUM( coefPtr,     src1BufPtr, i, thisIdx );
				sumLNext  += ONE_STEP_SUM( coefNextPtr, src0BufPtr, i, thatIdx );
				sumRNext  += ONE_STEP_SUM( coefNextPtr, src1BufPtr, i, thatIdx );
			}
			
			sumL  += (short)(( sumLNext - sumL + 0x00002000) >> 14) * distance;
			sumR  += (short)(( sumRNext - sumR + 0x00002000) >> 14) * distance;
		}
		else
			for( i = 0; i < sfCoeffTotalPtr[sfIdx]; i += 2 )
			{
				sumL  += ONE_STEP_SUM( coefPtr,     src0BufPtr, i, thisIdx );
				sumR  += ONE_STEP_SUM( coefPtr,     src1BufPtr, i, thisIdx );
			}

		sumL = (sumL + 0x00001000) >> 13;
		sumR = (sumR + 0x00001000) >> 13;

		ClipToShort( sumL );
		ClipToShort( sumR );
		*outPtr++ = (short)sumL;
		*outPtr++ = (short)sumR;
		outputCount++;
		
		intgTimeMove += timeIncrement;
		while (intgTimeMove >= timeSpecification)
		{
			movingIndex++;
			intgTimeMove -= timeSpecification;
		}
	}

	usedSampleIn = movingIndex - rcPtr->sfCoeffMax+1;
	rcPtr->intDiv = intgTimeMove;

	rcPtr->samplesLeft = inputBufferSize - usedSampleIn;
	for (i = 0; i < rcPtr->samplesLeft; i++)
	{
		src0BufPtr[i] = src0BufPtr[i + usedSampleIn];
		src1BufPtr[i] = src1BufPtr[i + usedSampleIn];
	}

	*outputSampleCount = outputCount;
	
	return err;
}


int RateConvert16
( 
	RateConverter	*rcPtr,
	long			srcChannelsTotal,
	long			dstChannelsTotal,
	short			*srcBuf0Ptr,		
	long			srcCount0,
	short			*dstBuf0Ptr,
	long			*dstCountOut
)	
{
	short	*srcBufPtr   = srcBuf0Ptr;
	short	*dstBufPtr	 = dstBuf0Ptr;		
	long	srcCount	 = srcCount0;
	long	dstCount	 = 0;
	long	thisSrcCount = 0;
	long	thisDstCount = 0;
	int		err			 = 0;
	
	while( srcCount > 0 )
	{
		LinearRateConverter *lRCPtr = rcPtr->lRCPtr;
		KaiserRateConverter *kRCPtr = rcPtr->kRCPtr;
		
		if( rcPtr->lRCPtr != NULL )
		{
			thisSrcCount = srcCount;
			
			if ( srcChannelsTotal == 1 && dstChannelsTotal == 1 )
				err = LinearMono16(  lRCPtr, srcBufPtr, thisSrcCount, dstBufPtr, &thisDstCount );
			else if(  srcChannelsTotal == 2 && dstChannelsTotal == 2 )
				err = LinearStereo16( lRCPtr, srcBufPtr, thisSrcCount, dstBufPtr, &thisDstCount );
			else
				err = -1;
		}
		else if( kRCPtr != NULL )
		{
			thisSrcCount = MyMin( (rcPtr->kRCPtr->srcBufSizeInSample - rcPtr->kRCPtr->samplesLeft), srcCount );
			
			if ( srcChannelsTotal == 1 && dstChannelsTotal == 1 )
					err = KaiserMono16(   kRCPtr, srcBufPtr, thisSrcCount, dstBufPtr, &thisDstCount );
			else if(  srcChannelsTotal == 2 && dstChannelsTotal == 2 )
					err = KaiserStereo16( kRCPtr, srcBufPtr, thisSrcCount, dstBufPtr, &thisDstCount );
			else
				err = -1;
		}
		else
			err = -1;
			
		if( err != 0 ) goto bail;									
		
		srcCount  -= thisSrcCount;
		srcBufPtr += thisSrcCount * srcChannelsTotal;

		dstCount  += thisDstCount;
		dstBufPtr += thisDstCount * dstChannelsTotal;
	}
	
	*dstCountOut = dstCount;
	
bail:	
	return err;
}


static void DisposeKaiserRateConverter( KaiserRateConverter *rcPtr )
{
	if( rcPtr != NULL )
	{
		FskMemPtrDispose( rcPtr->sfOffsetPtr );
		FskMemPtrDispose( rcPtr->sfCoeffTotalPtr );
		FskMemPtrDispose( rcPtr->sfCoeffLenPtr );
		FskMemPtrDispose( rcPtr->sfCoeffsPtr );
		FskMemPtrDispose( rcPtr->srcBuf0Ptr );
		FskMemPtrDispose( rcPtr->srcBuf1Ptr );
	
		FskMemPtrDispose( rcPtr );
	}
}


static int NewKaiserRateConverter( RateConverterSetting *settingPtr, KaiserRateConverter **rcOutPtr )
{
	int		err = 0;
	KaiserRateConverter *rcPtr = NULL;
	long	i, firstNon0Pos;
	long	coeffsTotal;
	float	*coeff 		 = NULL;
	float	*filtBuffPtr = NULL;
	double  sampleRatio;
	int 	s = settingPtr->srcSampleRateIdx;
	int 	d = settingPtr->dstSampleRateIdx;
	double  fc   	    = d == 0 ? intpFilterCutoff_44100[s]  : intpFilterCutoff_48000[s];
	double  beta 	    = d == 0 ? beta_44100[s] 			  : beta_48000[s];
	double  IoBeta 	    = d == 0 ? IoBeta_44100[s] 			  : IoBeta_48000[s];

	err = FskMemPtrNewClear( sizeof( KaiserRateConverter) , (FskMemPtr *)&rcPtr );
	BAIL_IF_ERR( err );

	rcPtr->dstBufSizeInSample = kDefaultDstBufSizeInSample;
	sampleRatio 		= d == 0 ? sampleRatio_44100[s] 	  : sampleRatio_48000[s];
	coeffsTotal 		= d == 0 ? filterCoeffsTotal_44100[s] : filterCoeffsTotal_48000[s];
	rcPtr->numerator    = d == 0 ? numerator_44100[s]   	  : numerator_48000[s];
	rcPtr->denominator  = d == 0 ? denominator_44100[s] 	  : denominator_48000[s];
	rcPtr->fracDiv      = d == 0 ? fracDiv_44100[s]   	      : fracDiv_48000[s];
	rcPtr->intDiv  	    = d == 0 ? intDiv_44100[s]   	      : intDiv_48000[s];
	rcPtr->samplesLeft  = d == 0 ? samplesLeft_44100[s]   	  : samplesLeft_48000[s];
	rcPtr->sfTotal 		= rcPtr->numerator;
	if (rcPtr->sfTotal > kMaxRatio)
		rcPtr->sfTotal = kMaxRatio;

	err = FskMemPtrNewClear( coeffsTotal * sizeof( float) , (FskMemPtr *)&coeff );
	BAIL_IF_ERR( err );

	SetCoeffs( fc, beta, IoBeta, rcPtr, coeffsTotal, coeff );

	//remove leading and trailing zeros
	for( i = 0; i < coeffsTotal; ++i)
		if (coeff[i] != 0.0)
			break;
	
	firstNon0Pos = i;
	for( i = coeffsTotal - 1; i >= firstNon0Pos; --i)
		if (coeff[i] != 0.0)
			break;
	
	filtBuffPtr	= coeff + firstNon0Pos;
	coeffsTotal	= i - firstNon0Pos + 1;

	err = SetSubFilter( coeffsTotal, filtBuffPtr, rcPtr );
	BAIL_IF_ERR( err );
	
	rcPtr->srcBufSizeInSample = (long) ( ((rcPtr->denominator * (kDefaultDstBufSizeInSample - 1)) / rcPtr->numerator) + 2 + rcPtr->sfCoeffMax );
	rcPtr->interpolateRatio   = rcPtr->sfTotal / rcPtr->numerator;

	err = FskMemPtrNewClear( ( rcPtr->srcBufSizeInSample + 256 ) * sizeof( short) , (FskMemPtr *)&rcPtr->srcBuf0Ptr );
	BAIL_IF_ERR( err );

	if( settingPtr->srcChannelsTotal == 2 )
	{
		err = FskMemPtrNewClear( ( rcPtr->srcBufSizeInSample + 256 ) * sizeof( short) , (FskMemPtr *)&rcPtr->srcBuf1Ptr );
		BAIL_IF_ERR( err );
	}
	
	*rcOutPtr = rcPtr;
	
bail:
	if( coeff != 0 )
		FskMemPtrDispose( coeff );

	if( err != 0 && rcPtr != NULL )
	{
		DisposeKaiserRateConverter( rcPtr );
		*rcOutPtr = NULL;
	}	

	return err;
}


static void DisposeLinearRateConverter( LinearRateConverter *lRCPtr )
{
	FskMemPtrDispose( lRCPtr );
}


static int NewLinearRateConverter( RateConverterSetting *settingPtr, LinearRateConverter **lRCOutPtr )
{
	LinearRateConverter *rcPtr;
	long	numer, denom;
	int		err = 0;

	err = FskMemPtrNewClear( sizeof( LinearRateConverter) , (FskMemPtr *)&rcPtr );
	BAIL_IF_ERR( err );

	rcPtr->inputRate	= settingPtr->srcSampleRate;
	rcPtr->outputRate	= settingPtr->dstSampleRate;
	rcPtr->channelCount = settingPtr->srcChannelsTotal;

	numer = settingPtr->srcSampleRate / settingPtr->dstSampleRate;
	denom = settingPtr->srcSampleRate % settingPtr->dstSampleRate;
	denom = (denom << 16) / settingPtr->dstSampleRate;
	rcPtr->sampleRateStep = (numer << 16) | denom;
	
	*lRCOutPtr = rcPtr;
	
bail:	
	if( err != 0 )
	{
		DisposeLinearRateConverter( rcPtr );
		*lRCOutPtr = NULL;
	}
	
	return err;
}


void DisposeRateConverter( RateConverter *rcPtr )
{
	if( rcPtr != NULL )
	{
		if( rcPtr->lRCPtr != NULL )
			DisposeLinearRateConverter( rcPtr->lRCPtr );
		
		if( rcPtr->kRCPtr != NULL )
			DisposeKaiserRateConverter( rcPtr->kRCPtr );
	
		FskMemPtrDispose( rcPtr );
	}
}


int NewRateConverter( RateConverterSetting *settingPtr, RateConverter **rcOutPtr )
{	
	RateConverter *rcPtr;
	int err = 0;

	err = FskMemPtrNewClear( sizeof( RateConverter) , (FskMemPtr *)&rcPtr );
	BAIL_IF_ERR( err );
	
	if( settingPtr->dstSampleRate == 44100 )
		settingPtr->dstSampleRateIdx = 0;
	else if( settingPtr->dstSampleRate == 48000 )
		settingPtr->dstSampleRateIdx = 1;
	else
		settingPtr->useLinearRC = 1;
	
	if( 	 settingPtr->srcSampleRate == 8000 )
		settingPtr->srcSampleRateIdx = 0;
	else if( settingPtr->srcSampleRate == 11025)
		settingPtr->srcSampleRateIdx = 1;
	else if( settingPtr->srcSampleRate == 12000)
		settingPtr->srcSampleRateIdx = 2;
	else if( settingPtr->srcSampleRate == 16000)
		settingPtr->srcSampleRateIdx = 3;
	else if( settingPtr->srcSampleRate == 22050)
		settingPtr->srcSampleRateIdx = 4;
	else if( settingPtr->srcSampleRate == 24000)
		settingPtr->srcSampleRateIdx = 5;
	else if( settingPtr->srcSampleRate == 32000)
		settingPtr->srcSampleRateIdx = 6;
	else if( settingPtr->srcSampleRate == 44100)
		settingPtr->srcSampleRateIdx = 7;
	else if( settingPtr->srcSampleRate == 48000)
		settingPtr->srcSampleRateIdx = 8;
	else if( settingPtr->srcSampleRate == 88200)
		settingPtr->srcSampleRateIdx = 9;
	else if( settingPtr->srcSampleRate == 96000)
		settingPtr->srcSampleRateIdx = 10;
	else if( settingPtr->srcSampleRate == 176400)
		settingPtr->srcSampleRateIdx = 11;
	else if( settingPtr->srcSampleRate == 192000)
		settingPtr->srcSampleRateIdx = 12;
	else 
		settingPtr->useLinearRC = 1;
		
	if( settingPtr->useLinearRC )
		err = NewLinearRateConverter( settingPtr, &rcPtr->lRCPtr );
	else
		err = NewKaiserRateConverter( settingPtr, &rcPtr->kRCPtr );
	BAIL_IF_ERR( err );
	
	*rcOutPtr = rcPtr;

bail:	
	if( err != 0 && rcPtr != NULL )
	{
		DisposeRateConverter( rcPtr );
		*rcOutPtr = NULL;
	}	
	
	return err;
}

#endif

//#pragma mark -

typedef struct {
	UInt32			outputRate;

#if USE_NEW_RATE_CONVERTER
	RateConverter	*rcPtr;
	Boolean			initialized;
#else
	UInt32			inputRate;
	UInt32			channelCount;

	UInt32			sampleRateStep;
	SInt16			lastSampleLeft;
	SInt16			lastSampleRight;
#endif
} rateConverterStateRecord, *rateConverterState;

FskExport(FskErr) coreaudiofilters_fskLoad(FskLibrary library)
{
	FskExtensionInstall(kFskExtensionAudioFilter, &gRateConverter);
	return kFskErrNone;
}

FskExport(FskErr) coreaudiofilters_fskUnload(FskLibrary library)
{
	FskExtensionUninstall(kFskExtensionAudioFilter, &gRateConverter);
	return kFskErrNone;
}

Boolean	rateConverterCanHandle(const char *filterType)
{
	return (0 == FskStrCompareCaseInsensitive("rate converter", filterType));
}


FskErr rateConverterNew(FskAudioFilter filter, void **filterState)
{
	FskErr err;
	rateConverterState state;

	err = FskMemPtrNewClear(sizeof(rateConverterStateRecord), &state);
	BAIL_IF_ERR(err);

bail:
	if (err) {
		rateConverterDispose(filter, state);
		state = NULL;
	}
	*filterState = state;

	return err;
}

FskErr rateConverterDispose(FskAudioFilter filter, void *stateIn)
{
	rateConverterState state = stateIn;

#if USE_NEW_RATE_CONVERTER
	DisposeRateConverter(state->rcPtr);
#endif
	FskMemPtrDispose(state);

	return kFskErrNone;
}

FskErr rateConverterGetMaximumBufferSize(FskAudioFilter filter, void *stateIn, UInt32 sampleCount, UInt32 *bufferSize)
{
	rateConverterState state = stateIn;
	float outputSampleCount;
	
#if USE_NEW_RATE_CONVERTER
	if( state->rcPtr->kRCPtr != NULL )
		sampleCount += state->rcPtr->kRCPtr->sfCoeffMax + 8;
#endif
	
	outputSampleCount = ((float)sampleCount * (float)state->outputRate) / (float)filter->inputSampleRate;
	*bufferSize = ((UInt32)(1.5 + outputSampleCount)) * 2 * filter->inputChannelCount;

	return kFskErrNone;
}


FskErr rateConverterStart(FskAudioFilter filter, void *stateIn)
{
	rateConverterState		state = stateIn;
	FskErr					err = kFskErrNone;
	
	if (0 == state->outputRate)
		return kFskErrInvalidParameter;

#if TARGET_RT_LITTLE_ENDIAN
	if (kFskAudioFormatPCM16BitLittleEndian != filter->inputFormat)
		return kFskErrInvalidParameter;
#else
	if (kFskAudioFormatPCM16BitBigEndian != filter->inputFormat)
		return kFskErrInvalidParameter;
#endif
	
#if USE_NEW_RATE_CONVERTER
	if (!state->initialized) {
		RateConverterSetting	setting;
		
		setting.useLinearRC		    = 0;
		setting.srcChannelsTotal	= filter->inputChannelCount;
		setting.dstChannelsTotal	= setting.srcChannelsTotal;
		setting.srcSampleRate		= filter->inputSampleRate;
		setting.dstSampleRate		= state->outputRate;

		err = NewRateConverter( &setting, &state->rcPtr );
		state->initialized = (kFskErrNone == err);
	}
#else
	{
		UInt32 numer, denom;

		numer = filter->inputSampleRate / state->outputRate;
		denom = filter->inputSampleRate % state->outputRate;
		denom = (denom << 16) / state->outputRate;
		state->sampleRateStep = (numer << 16) | denom;
	}
#endif

	return err;
}

FskErr rateConverterStop(FskAudioFilter filter, void *stateIn)
{
#if !USE_NEW_RATE_CONVERTER
	rateConverterState state = stateIn;

	state->lastSampleLeft = 0;
	state->lastSampleRight = 0;
#endif

	return kFskErrNone;
}

#if USE_NEW_RATE_CONVERTER

FskErr rateConverterProcessSamples(FskAudioFilter filter, void *stateIn, void *input, UInt32 inputSampleCount, void *output, UInt32 *outputSampleCount, UInt32 *outputSize)
{
	long				outSampleCount = 0;
	rateConverterState	state		= stateIn;
	FskErr				err			= kFskErrNone;

	// flush, nothing to do
	if ((NULL == input) || (0 == inputSampleCount))
		 goto bail;

	// rate convert
	err = RateConvert16( state->rcPtr, filter->inputChannelCount, filter->inputChannelCount, (short *)input, inputSampleCount, (short *)output, &outSampleCount );
	if( err != kFskErrNone ) goto bail;
	
bail:
	if (err)
		outSampleCount = 0;

	*outputSampleCount	= outSampleCount;
	*outputSize			= outSampleCount * 2 * filter->inputChannelCount;

	return err;
}

#else

FskErr rateConverterProcessSamples(FskAudioFilter filter, void *stateIn, void *input, UInt32 inputSampleCount, void *output, UInt32 *outputSampleCount, UInt32 *outputSize)
{
	rateConverterState state = stateIn;
	UInt32 convertedSampleCount = 0;
	UInt32 time = 0;
	UInt32 count;
	SInt32 sample;
	SInt16 *i, *o;

	// flush, nothing to do
	if ((NULL == input) || (0 == inputSampleCount))
		 goto bail;

	convertedSampleCount = (UInt32)(0.5 + ((float)inputSampleCount * (float)state->outputRate) / (float)filter->inputSampleRate);

	o = output;
	i = (SInt16 *)input;
	count = convertedSampleCount;

	if (1 == filter->inputChannelCount) {

		while (count-- && (time < 65536)) {
			SInt16 *sampleP = &i[0];
			UInt8 denom = (UInt8)(time >> 8);

			sample = state->lastSampleLeft * (256 - denom);
			sample += sampleP[0] * denom;
			*o++ = (SInt16)(sample >> 8);

			time += state->sampleRateStep;
		}

		count++;

		while (count--) {
			SInt16 *sampleP = &i[time >> 16];
			UInt8 denom = (UInt8)(time >> 8);

			sample = sampleP[-1] * (256 - denom);
			sample += sampleP[0] * denom;
			*o++ = (SInt16)(sample >> 8);

			time += state->sampleRateStep;
		}

		state->lastSampleLeft = ((SInt16 *)input)[inputSampleCount - 1]; 
	}
	else {
		// stereo rate converter

		while (count-- && (time < 65536)) {
			SInt16 *sampleP = &i[0];
			UInt8 denom = (UInt8)(time >> 8);

			// left sample
			sample = state->lastSampleLeft * (256 - denom);
			sample += sampleP[0] * denom;
			*o++ = (SInt16)(sample >> 8);

			// right sample
			sample = state->lastSampleRight * (256 - denom);
			sample += sampleP[+1] * denom;
			*o++ = (SInt16)(sample >> 8);

			time += state->sampleRateStep;
		}

		count++;
		
		while (count--) {
			SInt16 *sampleP = &i[(time >> 16) << 1];
			UInt8 denom = (UInt8)(time >> 8);

			// left sample
			sample = sampleP[-2] * (256 - denom);
			sample += sampleP[0] * denom;
			*o++ = (SInt16)(sample >> 8);

			// right sample
			sample = sampleP[-1] * (256 - denom);
			sample += sampleP[+1] * denom;
			*o++ = (SInt16)(sample >> 8);

			time += state->sampleRateStep;
		}

		i = &((SInt16 *)input)[(inputSampleCount - 1) * 2];
		state->lastSampleLeft = i[0];
		state->lastSampleRight = i[1];
	}

bail:
	*outputSampleCount = convertedSampleCount;
	*outputSize = convertedSampleCount * 2 * filter->inputChannelCount;

	return kFskErrNone;
}


#endif

FskErr rateConverterSetSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	rateConverterState state = stateIn;

	state->outputRate = property->value.integer;

	return kFskErrNone;
}

