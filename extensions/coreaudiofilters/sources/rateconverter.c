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
#ifndef APP_ONLY
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
#else
#include "stdlib.h"
#include "stdio.h"

#define memNewChunkFlagAllowLarge			0x1000	// allow >64K allocations
typedef unsigned char * *FskMemPtr;

int FskMemPtrNew( long size, FskMemPtr *newMemory)
{
	*newMemory = (FskMemPtr)malloc(size);

	return *newMemory ? 0 : -1;
}

void FskMemSet(void *dst, char fill, long count)
{
	char *d = (char *)dst;

	while (count--)
		*d++ = fill;
}

int FskMemPtrNewClear(long size, FskMemPtr *newMemory)
{
	int err;

	err = FskMemPtrNew(size, newMemory);
	if (0 == err)
		FskMemSet(*newMemory, 0, size);

	return err;
}

int FskMemPtrDispose(void *ptr)
{
	if (NULL != ptr) {
		free(ptr);
	}
	
	return 0;
}


#endif //APP_ONLY

#if USE_NEW_RATE_CONVERTER

#include "rateconverter.h"
#include "math.h"
#include "stdlib.h"

#if DEBUG_SOUND

#define ShortSwapEndian( s )  ( ( s&0x00ff )<<8 | ( ( s&0xff00 )>> 8 ) )
#define LongSwapEndian( s )  (  ( s&0x000000ff )<<24 | ( s&0x0000ff00 )<<8 | ( s&0x00ff0000 )>>8 | ( s&0xff000000 )>>24 )

#ifdef macintosh 
	#define ShortN2L(s)  ShortSwapEndian(s)
	#define LongN2L(s)   LongSwapEndian(s)
	#define LongL2N(s)   LongSwapEndian(s)
	#define ShortL2N(s)  ShortSwapEndian(s)
	#define LongN2B(s)   (s)
#else
	#define ShortN2L(s)  (s)
	#define LongN2L(s)   (s)
	#define LongL2N(s)   (s)
	#define ShortL2N(s)  (s)
	#define LongN2B(s)   LongSwapEndian(s)
#endif	


void Debug_Read_Sound( FILE *soundFile, long numChannels, long sampleCount, short *soundDataPtr )
{
	int i;
	
	if( soundFile == NULL )
		return;
	
	for( i = 0; i < sampleCount; i++ )
	{
		short twoBytes;
		
		fread( &twoBytes, 2, 1, soundFile);
		twoBytes = ShortN2L( twoBytes );
		*soundDataPtr = twoBytes;
		soundDataPtr++;
		
		if( numChannels == 2 )
		{
			fread( &twoBytes, 2, 1, soundFile);
			twoBytes = ShortN2L( twoBytes );
			*soundDataPtr = twoBytes;
			soundDataPtr++;
		}
	}
}


void Debug_Dump_Sound( short *soundDataPtr, long numChannels, long sampleCount, FILE *soundFile )
{
	int i;
	
	if( soundFile == NULL )
		return;
	
	for( i = 0; i < sampleCount; i++ )
	{
		short twoBytes;
		
		twoBytes = *soundDataPtr;
		twoBytes = ShortN2L( twoBytes );
		fwrite( &twoBytes, 2, 1, soundFile);
		soundDataPtr++;
		
		if( numChannels == 2 )
		{
			twoBytes = *soundDataPtr;
			twoBytes = ShortN2L( twoBytes );
			fwrite( &twoBytes, 2, 1, soundFile);
			soundDataPtr++;
		}
	}
	
	fflush( soundFile );
}


void Debug_CreateSoundFileForWrite( long isSource, long sampleRate, long numChannels, FILE **inputFile )
{
	FILE	*soundFile = *inputFile;
	char	string[512];
	char	filename[512];
	FILE	*stream;
	char	*ptr;
	
	if( inputFile == NULL )
		return;

#ifdef macintosh
	stream = popen("date", "r");
	fscanf(stream,"%[^\n]\n", string);
	pclose(stream);
	
	if( isSource )
		sprintf(filename, "/tmp/SoundInput_%s.wav", string );
	else
		sprintf(filename, "/tmp/SoundOutput_%s.wav", string );

	ptr = filename;

	while (*ptr)
	{
		if (' ' == *ptr)
			*ptr = '_';
		if (':' == *ptr)
			*ptr = '_';

		*ptr++;
	}
#else
	if( isSource )
		sprintf(filename, "C:\\SoundInput.wav" );
	else
		sprintf(filename, "C:\\SoundOutput.wav" );

	ptr = filename;

#endif

	soundFile = fopen(filename,"wb");	
	*inputFile = soundFile;
	if( soundFile == NULL )
	{
		printf("cannot open dump file for write (%s)\n", filename );
		return;
	}	
	{
		long	fourBytes;
		short	twoBytes;
		
		fourBytes = LongN2B('RIFF');
		fwrite( &fourBytes,  4, 1, soundFile);

		fourBytes = 1024*1024*10;							//total size not known yet
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);

		fourBytes = LongN2B('WAVE');		
		fwrite( &fourBytes, 4, 1, soundFile);

		fourBytes = LongN2B('fmt ');		
		fwrite( &fourBytes, 4, 1, soundFile);

		fourBytes = 16;		
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);

		twoBytes = 1;										//compression mode, 1 PCM/uncompressed
		twoBytes = ShortN2L( twoBytes );		
		fwrite( &twoBytes, 2, 1, soundFile);

		twoBytes = numChannels;								//	number of channels
		twoBytes = ShortN2L( twoBytes );		
		fwrite( &twoBytes, 2, 1, soundFile);

		fourBytes = sampleRate;								//sample rate
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);
		
		fourBytes = sampleRate * 2 * numChannels;			//average bytes per second
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);
		
		twoBytes = numChannels * 2;							//	block align
		twoBytes = ShortN2L( twoBytes );		
		fwrite( &twoBytes, 2, 1, soundFile);

		twoBytes = 16;										//	significant bits per sample
		twoBytes = ShortN2L( twoBytes );		
		fwrite( &twoBytes, 2, 1, soundFile);

		fourBytes = LongN2B('data');
		fwrite( &fourBytes,  4, 1, soundFile);

		fourBytes = 1024*1024*10;							//total size not known yet
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);
	}
	
	fflush( soundFile );
}


int Debug_OpenSoundFileForRead( char *namePtr, long *sampleRate, long *numChannels, long *sampleTotla, FILE **inputFile )
{
	FILE	*soundFile = *inputFile;
	int		err = 0;
	int		bytesTotal;
	
	if( namePtr == NULL )
		return -1;

	soundFile = fopen( namePtr, "rb" );	
	*inputFile = soundFile;
	if( soundFile == NULL )
	{
		printf("cannot open dump file for read (%s)\n", namePtr );
		return -1;
	}
	
	*sampleTotla = 0;
	fseek( soundFile, 0, SEEK_END );
	bytesTotal = ftell( soundFile );
	fseek( soundFile, 0, SEEK_SET );

	{
		long	fourBytes;
		short	twoBytes;
		
		fread( &fourBytes,  4, 1, soundFile);
		if( fourBytes != LongN2B('RIFF'))
			return -1;

		fread( &fourBytes, 4, 1, soundFile);
		fourBytes = LongL2N(fourBytes);//size

		fread( &fourBytes, 4, 1, soundFile);
		if( fourBytes != LongN2B('WAVE'))
			return -1;

		fread( &fourBytes, 4, 1, soundFile);
		if( fourBytes != LongN2B('fmt ') )
			return -1;

		fread( &fourBytes, 4, 1, soundFile);			//size should be 16
		fourBytes = LongL2N(fourBytes);
		if( fourBytes != 16 )
			return -1;

		fread( &twoBytes, 2, 1, soundFile);				//compression mode, 1 PCM/uncompressed
		twoBytes = ShortL2N(twoBytes);
		if( twoBytes != 1 )
			return -1;
		
		fread( &twoBytes, 2, 1, soundFile);				//	number of channels
		twoBytes = ShortL2N(twoBytes);
		*numChannels = twoBytes;

		fread( &fourBytes, 4, 1, soundFile);			//size should be 16
		fourBytes = LongL2N(fourBytes);
		*sampleRate = fourBytes;

		fread( &fourBytes, 4, 1, soundFile);			//average bytes per second
		fourBytes = LongL2N(fourBytes);

		fread( &twoBytes, 2, 1, soundFile);				//	block alig
		twoBytes = ShortL2N(twoBytes);

		fread( &twoBytes, 2, 1, soundFile);				//	block alig
		twoBytes = ShortL2N(twoBytes);

		fread( &fourBytes, 4, 1, soundFile);
		if( fourBytes != LongN2B('data') )
			return -1;

		fread( &fourBytes, 4, 1, soundFile);			//chunck size
		fourBytes = LongL2N(fourBytes);
	}

	*sampleTotla = ( bytesTotal - 44 ) / ( 2 * (*numChannels) );

	return err;
}


static int Debug_InitSoundFiles( RateConverterSetting *settingPtr, RateConverter *rcPtr )
{
	int err = 0;
	
	rcPtr->inputDumpFile	= NULL;
	rcPtr->outputDumpFile	= NULL;
	rcPtr->inputRefFile		= NULL;
	
	if( DUMP_INPUT )
		Debug_CreateSoundFileForWrite( 1, settingPtr->srcSampleRate, settingPtr->srcChannelsTotal, &rcPtr->inputDumpFile );

	if( DUMP_OUTPUT )
		Debug_CreateSoundFileForWrite( 0, settingPtr->dstSampleRate, settingPtr->dstChannelsTotal, &rcPtr->outputDumpFile );

	if( USE_INPUT_REF_FILE )
	{
		long total;
		
		err = Debug_OpenSoundFileForRead( "/tmp/SoundInputRef.wav", &settingPtr->srcSampleRate, &settingPtr->srcChannelsTotal, &total, &rcPtr->inputRefFile );
		BAIL_IF_ERR( err );
	}	

bail:
	return err;
}


void Debug_PrintFilterCoeff( long filterCoeffsTotal, float *filterCoeffsPtr, KaiserRateConverter *rcPtr )
{
	long i, j;
	
	printf( "Coeff Total: %d \n", filterCoeffsTotal);
	
	for( i = 0; i < filterCoeffsTotal; i++ )
		printf("%8.5f ", filterCoeffsPtr[i] );
	
	printf( "\n");
	
	printf( "Subfilter Total: %d \n", rcPtr->sfTotal );
	printf( "Subfilter sfCoeffMax: %d \n", rcPtr->sfCoeffMax );
	printf( "Subfilter sfCoeffStride: %d \n", rcPtr->sfCoeffStride );
	
	for( i = 0; i < rcPtr->sfTotal; i++ )
	{
		short *thisPtr = rcPtr->sfCoeffsPtr + i * rcPtr->sfCoeffStride;
		
		printf( "offset : %d \n", rcPtr->sfOffsetPtr[i] );
		printf( "total  : %d \n", rcPtr->sfCoeffTotalPtr[i] );
		printf( "len    : %d \n", rcPtr->sfCoeffLenPtr[i] );
		
		for( j = 0; j < rcPtr->sfCoeffTotalPtr[i]; j++ )
			printf( "%d, ", thisPtr[j] );

		printf( "\n" );
	}
}


#endif

//#pragma mark -

static void KaiseImpulse
( 
	double			filterPBandGain,
	double			fc,					// interpolation filter cutoff,
	double			beta,				// Kaiser window parameter
	double			IoBeta,
	long			filterCoeffsTotal,
	float			*filterImpulse
)
{
	long	i, n, N;
	double	T, gain, dT;

	// multiply the Kaiser window by the sin(x)/x response
	// use symmetry to reduce computations
	N		= filterCoeffsTotal - 1;
	gain	= 2.0 * filterPBandGain * fc;
	T		= 0.5 * N;
	dT		= 1.0 / T;

	for( i = 0, n = N; i <= n; ++i, --n )
	{
		double x = 2.0 * fc * (i - T);
		double xm, val;

		// calculate sin(x)/x, taking care at multiples of pi
		//  sin(pi * x) / (pi * x)
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
		if( x < -1.0 || x > 1.0 ) // avoid recalculating the denominator Bessel function
			break;
		
		{
			double	xx, xh;
			double	sum = 1.0;
			double	pow = 1.0;
			double	ds	= 1.0;
			long	k	= 0;

			xx = beta * sqrt( 1.0 - x * x ) * sqrt( 1.0 - x * x );
			// the zeroth-order modified Bessel function of first kind
			//	using a series expansion, returning Bassel value
			//				inf	  (xx/2)^k
			//	Io(xx) = 1 + SUM [ ------- ]^2
			//				k=1		k!
			xh	= 0.5 * xx;
			while ( ds > ( sum * kMin1 ) )
			{
				++k;
				pow = pow * ( xh / k );
				ds  = pow * pow;
				sum = sum + ds;
			}
			
			filterImpulse[n] = filterImpulse[i] = (float)( gain * val * sum * IoBeta );
		}
	}
}


static int SetSubFilter
( 
	long			filterCoeffsTotal,
	float			*filterImpulse, 
	KaiserRateConverter	*rcPtr 
)
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
	
	// for each sub-filter, find the first and last non-zero component
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
 	
	// set up the extra subfilter
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


long FindIntegerRatio( double f, long *divisor, long *dividend, long maxDividend )
{
	double	ratioInt;			
	double	targetF;	
	double	savedF;		
	double	numerator0    = 1.0;
	double	denominator0  = 0.0;
	double	numerator1    = 0.0;
	double	denominator1  = 1.0;
	double	numerator2;		
	double	denominator2;	
	long	isExactMatch = 1;

	savedF  = MyAbs(f);
	targetF = savedF;
	
	while (1)	//main loop for the continued fraction expansion
	{
		ratioInt = floor(targetF);

		//recursive calculation of the numerator and denominator
		numerator2		= numerator1;
		numerator1		= numerator0;
		numerator0		= ratioInt * numerator1 + numerator2;
		denominator2	= denominator1;
		denominator1	= denominator0;
		denominator0	= ratioInt * denominator1 + denominator2;

		if( ( numerator0 > kMaxLong ) || ( denominator0 > maxDividend ) )
		{ // try a secondary convergent
			double  predRatioInt  = floor((kMaxLong - numerator2) / numerator1);
			double  predRatioInt2 = floor((maxDividend - denominator2) / denominator1);

			numerator0   = numerator1;	 // back off to the previous convergent
			denominator0 = denominator1;

			if( predRatioInt > predRatioInt2 )
				predRatioInt = predRatioInt2;
			
			if( predRatioInt > 0 )
			{
				double sndNumerator   = (predRatioInt * numerator1)   + numerator2;
				double secondaryDenominator = (predRatioInt * denominator1) + denominator2;
				
				if( MyAbs(savedF - (sndNumerator / secondaryDenominator))  < MyAbs(savedF - (numerator0 / denominator0)) )
				{
					numerator0   = sndNumerator;
					denominator0 = secondaryDenominator;
				}
			}
			
			isExactMatch = 0;
			break;
		}

		// check the approximation error
		if( MyAbs(savedF - numerator0 / denominator0) <= 0.0 )
			break;

		// next term
		targetF = 1.0 / (targetF - ratioInt);
	}

	if( f < 0.0 )
		numerator0 = -numerator0;

	*divisor  = (long)numerator0;
	*dividend = (long)denominator0;

	return isExactMatch;
}

const static double betaAry[6]				= {    7.85726,         5.65326,         4.55126,      3.395321052,       2.116624861,      0.928576521 };
const static long	subhFiltLengthAry[6]	= {         14,              11,               9,                7,                 5,                3 };
const static double IoBetaAry[6]			= { 0.00267262,     0.020387997, 0.05467829396648, 0.1479653534153,  0.40395454221032, 0.81468882138602 };
const static double attenParamAry[6]		= {          0, 3.6246518105850, 2.92824216539813,      2.23189415,        1.53551532,                0 };

static int GetFilterCoeffs( RateConverterSetting *settingPtr, KaiserRateConverter *rcPtr, long *filterCoeffsTotalOut, float **filterCoeffsPtrOut, double *sampleRatioOut )
{
	double	filterPBandGain;						
	double	intpFilterCutoff;
	double	beta;									
	double	IoBeta;
	double 	sampleRatio;
	long	subhFiltLength;
	double	filterCutoff;
	long	upAttenuation;							
	long	downAttenuation;						
	float	*filterCoeffsPtr = NULL;
	long	filterCoeffsTotal  = 0;
	int		err = 0;
	
	upAttenuation	  = settingPtr->rollOffSlope;
	downAttenuation	  = settingPtr->rollOffSlope;
	sampleRatio		  = (double)settingPtr->dstSampleRate / (double)settingPtr->srcSampleRate;
	sampleRatio		 /= settingPtr->rateMultiplier;
	rcPtr->dstBufSizeInSample = settingPtr->dstBufSizeInSample;//bounded by the total number of output sample for each conversion
	*sampleRatioOut   = sampleRatio;
	
	{
		long	divisor;						
		long	dividend;							
		
		FindIntegerRatio( sampleRatio, &divisor, &dividend, kMaxLong );
		rcPtr->numerator   = divisor;
		rcPtr->denominator = dividend;

		rcPtr->sfTotal = divisor;
		if (rcPtr->sfTotal > kMaxRatio)
			rcPtr->sfTotal = kMaxRatio;

		filterPBandGain = (double)rcPtr->sfTotal;
	}
	
	if (sampleRatio >= 1.0)
	{
		filterCutoff		= 0.5;
		intpFilterCutoff = filterCutoff / rcPtr->sfTotal;
		
		if( upAttenuation == kAttenuationDefault )
			upAttenuation =  kAttenuation23;
			
		beta			= betaAry[ upAttenuation ];
		IoBeta			= IoBetaAry[ upAttenuation ];
		subhFiltLength	= subhFiltLengthAry[ upAttenuation ];
	}
	else
	{
		#define	kFilterTransition	0.362465
		float	attenParam;	
		float	numCoeff;					
		long	numCoeffLong;
		
		filterCutoff = 0.5 * sampleRatio;
		intpFilterCutoff = filterCutoff / rcPtr->sfTotal;
		
		if( downAttenuation == kAttenuationDefault )
			downAttenuation = kAttenuation30;
			
		beta		 = betaAry[ downAttenuation ];
		IoBeta		 = IoBetaAry[ downAttenuation ];
		attenParam	 = (float)attenParamAry[ downAttenuation ];
		numCoeff	 = (float)( ( rcPtr->sfTotal * attenParam ) / ( kFilterTransition * filterCutoff ) );
		numCoeffLong = (long)numCoeff;

		if ( numCoeff - numCoeffLong > 0.0)
			 numCoeffLong++;

		subhFiltLength = (long)( numCoeffLong * 0.5 / rcPtr->sfTotal );
		if( numCoeffLong > 2*rcPtr->sfTotal *subhFiltLength )
			subhFiltLength++;	

		if( subhFiltLength > kMaxTaps )
			subhFiltLength = kMaxTaps;
	}

	filterCoeffsTotal	= 2 * subhFiltLength * rcPtr->sfTotal + 1;

	err = FskMemPtrNewClear( filterCoeffsTotal * sizeof( float) , (FskMemPtr *)&filterCoeffsPtr );
	BAIL_IF_ERR( err );

	KaiseImpulse( filterPBandGain, intpFilterCutoff, beta, IoBeta, filterCoeffsTotal, filterCoeffsPtr );
	
	*filterCoeffsPtrOut   = filterCoeffsPtr;
	*filterCoeffsTotalOut = filterCoeffsTotal;

bail:
	if( err != 0 && filterCoeffsPtr != NULL )
		FskMemPtrDispose( filterCoeffsPtr );
	
	return err;
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
	double	fracTimeMove	  = rcPtr->fracCurDividend;	
	double	intgTimeMove	  = rcPtr->curDividend;
	double	timeIncrement	  = rcPtr->denominator;
	double	timeSpecification = rcPtr->numerator;
	double	interpolateRatio  = rcPtr->interpolateRatio;
	long	i;
	long	err = 0;

	for (i = 0; i < inputSampleCount; i++)
		src0BufPtr[i] = *inputPtr++;

	src0BufPtr = rcPtr->srcBuf0Ptr + rcPtr->sfCoeffMax;

	while( ( movingIndex < inputBufferSize ) && ( outputCount < rcPtr->dstBufSizeInSample ) )
	{
		long   sumL		  = 0;
		double interRatio = (fracTimeMove + intgTimeMove) * interpolateRatio;
		long   sfIdx	  = (long)interRatio;	
		short  distance   = FloatToFix14(interRatio - sfIdx);		// distance between the actual point and the integral part
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
				sumL     += coefPtr[i]     * src0BufPtr[ thisIdx - i ] + coefPtr[i+1]     * src0BufPtr[ thisIdx - i - 1 ];
				sumLNext += coefNextPtr[i] * src0BufPtr[ thatIdx - i ] + coefNextPtr[i+1] * src0BufPtr[thatIdx - i - 1];
			}
			
			sumL  += (short)(( sumLNext - sumL + 0x00002000) >> 14) * distance;
		}
		else
		{
			coefPtr = rcPtr->sfCoeffsPtr + sfIdx * rcPtr->sfCoeffStride;		
			thisIdx = movingIndex - sfOffsetAry[sfIdx];
			
			for( i = 0; i < sfCoeffTotalPtr[sfIdx]; i += 2 )
				sumL += coefPtr[i] * src0BufPtr[ thisIdx - i ] + coefPtr[i+1] * src0BufPtr[ thisIdx - i - 1 ];
		}		

		sumL = (sumL + 0x00001000) >> 13;

		ClipToShort( sumL );
		*outPtr++ = (short)sumL;
		outputCount++;
		
		// update the sample pointer (n, intgTimeMove, fracTimeMove) - fracTimeMove remains unchanged
		intgTimeMove += timeIncrement;
		while (intgTimeMove >= timeSpecification)
		{
			movingIndex++;
			intgTimeMove -= timeSpecification;
		}
	}

	usedSampleIn = movingIndex - rcPtr->sfCoeffMax + 1;
	rcPtr->curDividend = intgTimeMove;

	// shift useful data that is already in the buffer
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
	double	fracTimeMove		= rcPtr->fracCurDividend;
	double	intgTimeMove		= rcPtr->curDividend;
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

	while( ( movingIndex < inputBufferSize ) && ( outputCount < rcPtr->dstBufSizeInSample ) )
	{
		long   sumL		  = 0;
		long   sumR		  = 0;
		double interRatio = (fracTimeMove + intgTimeMove) * interpolateRatio;
		long   sfIdx	  = (long)interRatio;		
		short  distance   = FloatToFix14(interRatio - sfIdx);		// distance between the actual point and the integral part
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
				sumL     += coefPtr[i]     * src0BufPtr[ thisIdx - i ] + coefPtr[i+1]     * src0BufPtr[ thisIdx - i - 1 ];
				sumR     += coefPtr[i]     * src1BufPtr[ thisIdx - i ] + coefPtr[i+1]     * src1BufPtr[ thisIdx - i - 1 ];
				sumLNext += coefNextPtr[i] * src0BufPtr[thatIdx - i]   + coefNextPtr[i+1] * src0BufPtr[thatIdx - i - 1];
				sumRNext += coefNextPtr[i] * src1BufPtr[thatIdx - i]   + coefNextPtr[i+1] * src1BufPtr[thatIdx - i - 1];
			}
			
			sumL  += (short)(( sumLNext - sumL + 0x00002000) >> 14) * distance;
			sumR  += (short)(( sumRNext - sumR + 0x00002000) >> 14) * distance;
		}
		else
			for( i = 0; i < sfCoeffTotalPtr[sfIdx]; i += 2 )
			{
				sumL += coefPtr[i] * src0BufPtr[ thisIdx - i ] + coefPtr[i+1] * src0BufPtr[ thisIdx - i - 1 ];
				sumR += coefPtr[i] * src1BufPtr[ thisIdx - i ] + coefPtr[i+1] * src1BufPtr[ thisIdx - i - 1 ];
			}

		sumL = (sumL + 0x00001000) >> 13;
		sumR = (sumR + 0x00001000) >> 13;

		ClipToShort( sumL );
		ClipToShort( sumR );
		*outPtr++ = (short)sumL;
		*outPtr++ = (short)sumR;
		outputCount++;
		
		// update the sample pointer (n, intgTimeMove, fracTimeMove) - fracTimeMove remains unchanged
		intgTimeMove += timeIncrement;
		while (intgTimeMove >= timeSpecification)
		{
			movingIndex++;
			intgTimeMove -= timeSpecification;
		}
	}

	usedSampleIn = movingIndex - rcPtr->sfCoeffMax+1;
	rcPtr->curDividend = intgTimeMove;

	// shift useful data that is already in the buffer
	rcPtr->samplesLeft = inputBufferSize - usedSampleIn;
	for (i = 0; i < rcPtr->samplesLeft; i++)
	{
		src0BufPtr[i] = src0BufPtr[i + usedSampleIn];
		src1BufPtr[i] = src1BufPtr[i + usedSampleIn];
	}

	*outputSampleCount = outputCount;
	
	return err;
}


// 16 bit mono or stereo rate convertion
//	assuming src and dst channel numbers are the same
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
	
#if DEBUG_SOUND
	#if USE_INPUT_REF_FILE
		Debug_Read_Sound( rcPtr->inputRefFile, srcChannelsTotal, srcCount, srcBufPtr );
	#endif
#endif

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
	
#if DEBUG_SOUND

	#if DUMP_INPUT
		if( srcCount < 0 )
			srcCount = 0;
		
		srcCount0 -= srcCount;
		Debug_Dump_Sound( srcBuf0Ptr, srcChannelsTotal, srcCount0, rcPtr->inputDumpFile );
	#endif
	
	#if DUMP_OUTPUT
		Debug_Dump_Sound( dstBuf0Ptr, dstChannelsTotal, dstCount, rcPtr->outputDumpFile );
	#endif
	
#endif	

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
	KaiserRateConverter *rcPtr = NULL;
	long	i, firstNon0Pos;
	long	filterCoeffsTotal;
	float	*filterCoeffsPtr = NULL;
	float	*filtBuffPtr	 = NULL;
	double  sampleRatio;
	double	filterDelay;
	int		err = 0;

	err = FskMemPtrNewClear( sizeof( KaiserRateConverter) , (FskMemPtr *)&rcPtr );
	BAIL_IF_ERR( err );

	err = GetFilterCoeffs( settingPtr, rcPtr, &filterCoeffsTotal, &filterCoeffsPtr, &sampleRatio );
	BAIL_IF_ERR( err );

	//remove leading and trailing zeros
	for( i = 0; i < filterCoeffsTotal; ++i)
		if (filterCoeffsPtr[i] != 0.0)
			break;
	
	firstNon0Pos = i;
	for( i = filterCoeffsTotal - 1; i >= firstNon0Pos; --i)
		if (filterCoeffsPtr[i] != 0.0)
			break;
	
	filterDelay		= ( 0.5 * ( filterCoeffsTotal - 1) - firstNon0Pos ) / rcPtr->sfTotal;
	filtBuffPtr		= filterCoeffsPtr + firstNon0Pos;
	filterCoeffsTotal	= i - firstNon0Pos + 1;

	err = SetSubFilter( filterCoeffsTotal, filtBuffPtr, rcPtr );
	BAIL_IF_ERR( err );
	
#if DEBUG_SOUND
	Debug_PrintFilterCoeff( filterCoeffsTotal, filtBuffPtr, rcPtr );
#endif	
	{
		long newDenominator;
		long newNumerator;
		long denominatorMax;
		long isExactMatch;

		denominatorMax	= (long)( kMaxLong / (sampleRatio + rcPtr->dstBufSizeInSample ) );
		isExactMatch	= FindIntegerRatio( sampleRatio, &newNumerator, &newDenominator, denominatorMax);
		if( !isExactMatch )
		{
			rcPtr->denominator = 1.0;
			rcPtr->numerator   = sampleRatio;
		}
		else
		{
			long multiplyFactor;
			long r;
			long u = MyAbs(newNumerator);
			long v = MyAbs((long) rcPtr->sfTotal);
			long gcd;

			while (v != 0)
			{
				r = u % v;
				u = v;
				v = r;
			}
			gcd = u;

			multiplyFactor =  rcPtr->sfTotal / gcd;
			if ((newDenominator <= (denominatorMax / multiplyFactor)) && (newNumerator <= (kMaxLong / multiplyFactor)))
			{
				rcPtr->denominator = multiplyFactor * newDenominator;
				rcPtr->numerator   = multiplyFactor * newNumerator;
			}
		}
	}
	
	//derive the total number of input sample it caches for each conversion
	rcPtr->srcBufSizeInSample = (long) ( ((rcPtr->denominator * (settingPtr->dstBufSizeInSample - 1)) / rcPtr->numerator) + 2 + rcPtr->sfCoeffMax );
	rcPtr->interpolateRatio   = rcPtr->sfTotal / rcPtr->numerator;

	{
		long 	timeOffsetAtSampleRate = (long)(filterDelay);		// time offset at current sampling rate

		if (rcPtr->numerator == floor(rcPtr->numerator))
		{ //integer Numerator
			rcPtr->intgTimeStart = floor(rcPtr->numerator * (filterDelay - timeOffsetAtSampleRate));
			rcPtr->fracTimeStart = (rcPtr->numerator * (filterDelay - timeOffsetAtSampleRate)) - rcPtr->intgTimeStart;

			// if it is nearly an integer
			if (rcPtr->fracTimeStart < kMin2)
				rcPtr->fracTimeStart = 0.0;
			else if (rcPtr->fracTimeStart > (1.0 - kMin2))
			{
				rcPtr->fracTimeStart = 0.0;
				rcPtr->intgTimeStart = rcPtr->intgTimeStart + 1.0;
				if (rcPtr->intgTimeStart >= rcPtr->numerator)
				{
					rcPtr->intgTimeStart = 0.0;
					++timeOffsetAtSampleRate;
				}
			}
		}
		else 
		{ //non-integer numerator
			rcPtr->intgTimeStart = rcPtr->numerator * (filterDelay - timeOffsetAtSampleRate);
			rcPtr->fracTimeStart = 0.0;
		}
		
		rcPtr->samplesLeft = rcPtr->sfCoeffMax - timeOffsetAtSampleRate - 1;
	}
	
	// initialize rate converter state
	// need to be set whenever new filter and new source
	rcPtr->fracCurDividend = rcPtr->fracTimeStart;
	rcPtr->curDividend	   = rcPtr->intgTimeStart;
	
	err = FskMemPtrNewClear( ( rcPtr->srcBufSizeInSample + 256 ) * sizeof( short) , (FskMemPtr *)&rcPtr->srcBuf0Ptr );
	BAIL_IF_ERR( err );

	if( settingPtr->srcChannelsTotal == 2 )
	{
		err = FskMemPtrNewClear( ( rcPtr->srcBufSizeInSample + 256 ) * sizeof( short) , (FskMemPtr *)&rcPtr->srcBuf1Ptr );
		BAIL_IF_ERR( err );
	}
	
	*rcOutPtr = rcPtr;
	
bail:
	if( filterCoeffsPtr != 0 )
		FskMemPtrDispose( filterCoeffsPtr );

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
	
	if( settingPtr->useLinearRC )
		err = NewLinearRateConverter( settingPtr, &rcPtr->lRCPtr );
	else
		err = NewKaiserRateConverter( settingPtr, &rcPtr->kRCPtr );
	BAIL_IF_ERR( err );
	
#if DEBUG_SOUND
	Debug_InitSoundFiles( settingPtr, rcPtr );
#endif

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

#ifndef APP_ONLY

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
		setting.rollOffSlope		= kAttenuationDefault;
		setting.srcChannelsTotal	= filter->inputChannelCount;
		setting.dstChannelsTotal	= setting.srcChannelsTotal;
		setting.srcSampleRate		= filter->inputSampleRate;
		setting.dstSampleRate		= state->outputRate;
		setting.rateMultiplier		= 1.0;
		setting.dstBufSizeInSample	= kDefaultDstBufSizeInSample;
		
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

// sample rate conversion (cheap sample rate converter from Kinoma Player)
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

// sample rate conversion (cheap sample rate converter from Kinoma Player)
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

	//o = tempBuf + headRoom * 2 * filter->inputChannelCount;//¥¥¥output;
	o = output;
	i = (SInt16 *)input;
	count = convertedSampleCount;

	if (1 == filter->inputChannelCount) {
		// mono rate converter

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

#endif //APP_ONLY
