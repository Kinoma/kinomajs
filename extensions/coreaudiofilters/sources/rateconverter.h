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
#ifndef __RATECONVERTER_H__
#define __RATECONVERTER_H__

#ifdef __cplusplus
extern "C" {
#endif

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
#define kMin3				1638		// 0.1 in 2.14
#define kMax3				14746		// 0.9 = 1 - 0.1 in 2.14
#define MyMax(a, b)			(((a) > (b)) ? (a) : (b))
#define MyMin(a, b)			(((a) < (b)) ? (a) : (b))
#define FloatToFix14(a)		((short) ((a) * 16384.0 + 0.5))	// Format -- 2.14
#define ClipToShort(a)		{	if( a > 32767)			\
									a = 32767;			\
								else if (a < -32768)	\
									a = -32768;			\
							}

typedef struct
{
	long			samplesLeft;
	long			srcBufSizeInSample;
	long			dstBufSizeInSample;
	short			*srcBuf0Ptr;
	short			*srcBuf1Ptr;
	double			interpolateRatio;
	double			numerator;
	double			denominator;
	double			intDiv;
	double			fracDiv;
	long			sfTotal;
	long			sfCoeffMax;
	long			sfCoeffStride;
	long			*sfOffsetPtr;
	long			*sfCoeffTotalPtr;
	long			*sfCoeffLenPtr;
	short			*sfCoeffsPtr;
} KaiserRateConverter;


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
	KaiserRateConverter		*kRCPtr;
	LinearRateConverter		*lRCPtr;
} RateConverter;

typedef struct
{
	long			useLinearRC;
	long			srcChannelsTotal;
	long			dstChannelsTotal;
	long			srcSampleRate;
	long			dstSampleRate;
	long			srcSampleRateIdx;
	long			dstSampleRateIdx;
	float			rateMultiplier;				// multiplier to apply to rate conversion
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

#ifdef __cplusplus
}
#endif

#endif /* __RATECONVERTER_H__ */
