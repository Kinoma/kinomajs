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
#ifndef __RATECONVERTER_H__
#define __RATECONVERTER_H__

#ifdef APP_ONLY
#define DEBUG_SOUND						1
#else
#define DEBUG_SOUND						0
#endif

#if DEBUG_SOUND
#include "stdio.h"

#define DUMP_INPUT						0
#define DUMP_OUTPUT						1
#define USE_INPUT_REF_FILE				0

#endif


#ifdef __cplusplus
extern "C" {
#endif

enum
{
	kAttenuation80		= 0,
	kAttenuation60		= 1,
	kAttenuation50		= 2,
	kAttenuation40		= 3,
	kAttenuation30		= 4,
	kAttenuation23		= 5,
	kAttenuationDefault = 6
};

enum
{
	kMaxRatio					= 24,					// max interpolation filter ratio
	kMaxTaps					= 100,					// 56 for 44100 - 8000
	kDefaultDstBufSizeInSample	= 1024
};

#ifndef NULL
#define NULL 0
#endif

#define kMin1				1E-6		// for bessel function
#define kMin2				1E-8		// for time position
#define kMin3				1638		// 0.1 in 2.14
#define kMax3				14746		// 0.9 = 1 - 0.1 in 2.14
#define	kMaxLong			0x7fffffff
#define MyAbs(x)			(((x) < 0) ? -(x) : (x))
#define MyMax(a, b)			(((a) > (b)) ? (a) : (b))
#define MyMin(a, b)			(((a) < (b)) ? (a) : (b))
#define FloatToFix14(a)		((short) ((a) * 16384.0 + 0.5))	// Format -- 2.14
#define ClipToShort(a)		{	if( a > 32767)			\
									a = 32767;			\
								else if (a < -32768)	\
									a = -32768;			\
							}

// rate converter parameter block
typedef struct
{
	long			samplesLeft;					//from last batch
	long			srcBufSizeInSample;
	long			dstBufSizeInSample;
	short			*srcBuf0Ptr;
	short			*srcBuf1Ptr;

	double			interpolateRatio;				//index in consumed source data
	double			numerator;
	double			denominator;
	double			curDividend;					//current time value specifier
	double			fracCurDividend;				//fractional value of current time specifier
	double			intgTimeStart;
	double			fracTimeStart;

	long			sfTotal;
	long			sfCoeffMax;						//maximum number of coefficients in any subfilter
	long			sfCoeffStride;
	long			*sfOffsetPtr;					//offset of first coefficient in each subfilter
	long			*sfCoeffTotalPtr;				//number of coefficients in each subfilter
	long			*sfCoeffLenPtr;
	short			*sfCoeffsPtr;					//coefficients in subfilter order

} KaiserRateConverter;


// rate converter parameter block
typedef struct
{
	long			inputRate;
	long			outputRate;
	long			channelCount;

	long			sampleRateStep;
	short			lastSampleLeft;
	short			lastSampleRight;

} LinearRateConverter;


typedef struct
{
	KaiserRateConverter		*kRCPtr;					//coefficients in subfilter order
	LinearRateConverter		*lRCPtr;

#if DEBUG_SOUND
	FILE			*inputDumpFile;
	FILE			*outputDumpFile;
	FILE			*inputRefFile;
#endif

} RateConverter;

typedef struct
{
	long			useLinearRC;
	long			rollOffSlope;
	long			srcChannelsTotal;
	long			dstChannelsTotal;
	long			srcSampleRate;
	long			dstSampleRate;
	float			rateMultiplier;				// multiplier to apply to rate conversion
	long			dstBufSizeInSample;			// no. samples in output buffer
} RateConverterSetting;

int	NewRateConverter
(
	RateConverterSetting	*settingPtr,
	RateConverter			**rcPtr
);

void	DisposeRateConverter
(
	RateConverter	*rcPtr
);

int RateConvert16
(
	RateConverter	*rcPtr,
	long			srcChannelsTotal,
	long			dstChannelsTotal,
	short			*srcBufPtr,
	long			srcCount,
	short			*dstBufPtr,
	long			*dstCountOut
);

#if DEBUG_SOUND
void Debug_Read_Sound( FILE *soundFile, long numChannels, long sampleCount, short *soundDataPtr );
void Debug_Dump_Sound( short *soundDataPtr, long numChannels, long sampleCount, FILE *soundFile );
int Debug_OpenSoundFileForRead( char *namePtr, long *sampleRate, long *numChannels, long *sampleTotal, FILE **inputFile );
void Debug_CreateSoundFileForWrite( long isSource, long sampleRate, long numChannels, FILE **inputFile );
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RATECONVERTER_H__ */
