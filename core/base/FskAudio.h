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
#ifndef __FSKAUDIO__
#define __FSKAUDIO__

#include "FskExtensions.h"
#include "FskMedia.h"
#include "FskTime.h"
#include "FskUtilities.h"
#include "FskList.h"

#ifdef __FSKAUDIO_PRIV__
	#ifndef __FSKAUDIOCODEC_PRIV__
		#define __FSKAUDIOCODEC_PRIV__
	#endif

		#include "FskThread.h"
	#include "FskAudioFilter.h"

	#if TARGET_OS_WIN32
		#ifndef SUPPORT_WIN32_WAVEOUT
			#ifndef SUPPORT_WIN32_DIRECTSOUND
				#define SUPPORT_WIN32_DIRECTSOUND 1
			#endif
		#endif

		#include "windows.h"
		#include "mmsystem.h"
		#include "Mmsystem.h"
		#include "mmreg.h"
		#include "Msacm.h"
		#include "dsound.h"
	#endif
#elif SUPPORT_INSTRUMENTATION
	#include "FskAudioFilter.h"
	#include "FskThread.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef FskInt64 FskSampleTime;

/* Forward declarations */
struct FskAudioOutRecord;
struct FskSndChannelRecord;


typedef void (*FskAudioOutDoneCallback)(struct FskAudioOutRecord *audioOut, void *refCon, void *dataRefCon, Boolean played);
typedef FskErr (*FskAudioOutMoreCallback)(struct FskAudioOutRecord *audioOut, void *refCon, UInt32 requestedSamples);

#ifdef __FSKAUDIO_PRIV__

// Please note:
// On Palm, the FskAudioOutBlockRecord is read from an armlet.
// So please keep the structure members 32-bit aligned to keep Brian happy.
struct FskAudioOutBlockRecord {
	struct FskAudioOutBlockRecord  *next;

	unsigned char		*data;
	UInt32				dataSize;
	UInt32				sampleCount;

	void				*refCon;
	Boolean				done;
	Boolean				silence;
	Boolean				unused0;
	Boolean				unused1;

	void				*audioOut;

#if SUPPORT_WIN32_WAVEOUT
	WAVEHDR				waveHdr;
	Boolean				prepared;
#elif SUPPORT_WIN32_DIRECTSOUND
	UInt32				samplesUsed;
#elif TARGET_OS_LINUX
	UInt32				playingFrame;
	UInt32				format;		// data might be compressed if HW can deal
	char				*loc;
#elif TARGET_OS_MAC && USE_AUDIO_QUEUE
	Boolean				onUsing;
#endif

	UInt32				frameCount;
	UInt32				*frameSizes;
	UInt32				frameSizesArray[1];		// must be last
};

typedef struct FskAudioOutBlockRecord FskAudioOutBlockRecord;
typedef FskAudioOutBlockRecord *FskAudioOutBlock;
#endif

FskDeclarePrivateType(FskAudioOut)
FskDeclarePrivateType(FskAudioDecompress)
FskDeclarePrivateType(FskAudioCompress)

#if defined(__FSKAUDIO_PRIV__)
	struct FskAudioOutRecord {
		FskAudioOut					next;

		UInt32						sampleRate;
		UInt32						format;
		UInt16						numChannels;

		UInt16						leftVolume;
		UInt16						rightVolume;

		Boolean						playing;

		FskAudioOutMoreCallback		moreCB;
		void						*moreRefCon;

		FskAudioOutDoneCallback		doneCB;
		void						*doneRefCon;

		FskThread					thread;
#if TARGET_OS_LINUX
		FskListMutex				blocks;
#else
		FskAudioOutBlock			blocks;
#endif

		UInt32						chunkRequestSize;
		UInt32						bufferedSamplesTarget;

		FskSampleTime				zeroTime;

		FskMutex					mutex;

#if SUPPORT_WIN32_WAVEOUT
		HWAVEOUT					waveOut;
		Boolean						flushPending;
		Boolean						dontRefill;
#elif SUPPORT_WIN32_DIRECTSOUND
		LPDIRECTSOUND8				dS;
		LPDIRECTSOUNDBUFFER8		dSBuffer;
		HANDLE						hNotify;
		HANDLE						hEndThread;
		HANDLE						hNotifyThread;
		UInt32						bufferSize;
		FskInt64					bytesWritten;
		UInt32						bufferCount;
		UInt32						samplesNeeded;
#elif TARGET_OS_LINUX
		void						*ext;
#elif TARGET_OS_MAC && USE_AUDIO_QUEUE
		void						*audioQueue;
//#define QUEUE_GET_CURRENT_TIME
#if defined(QUEUE_GET_CURRENT_TIME)
        Float64                     currentSampleTime;
#else
		FskTimeRecord				startHostTime;
		FskTimeRecord				currentHostTime;
#endif
#elif TARGET_OS_MAC
    void						*audioConverter;
    void	 					*outputAudioUnit;
    UInt64						startHostTime;
    UInt64						currentHostTime;
#endif
		FskInstrumentedItemDeclaration
	};
#endif


enum {
	kFskAudioFormatUndefined = 0,

	kFskAudioFormatPCM16BitBigEndian = 1,		// x-audio-codec/pcm-16-be
	kFskAudioFormatPCM16BitLittleEndian,		// x-audio-codec/pcm-16-le
	kFskAudioFormatPCM8BitTwosComplement,		// x-audio-codec/pcm-8-twos
	kFskAudioFormatPCM8BitOffsetBinary,			// x-audio-codec/pcm-8-offset 

	kFskAudioFormatMP3 = 0x1000,				// x-audio-codec/mp3 - layer 3
	kFskAudioFormatMP2 = 0x1001,				// x-audio-codec/mp3 - layer 2
	kFskAudioFormatMP1 = 0x1002,				// x-audio-codec/mp3 - layer 1
	kFskAudioFormatAAC = 0x1010,				// x-audio-codec/aac
	kFskAudioFormatAACADTS = 0x1011,
	kFskAudioFormatATRAC3 = 0x1020,				// x-audio-codec/atrac3
	kFskAudioFormatATRAC3Plus,					// x-audio-codec/atrac3-plus
	kFskAudioFormatATRACAdvancedLossless,		// x-audio-codec/atrac3-lossless
	kFskAudioFormatMP1A = 0x1030,				// x-audio-codec/mpeg-1
	kFskAudioFormatAC3 = 0x1040,				// x-audio-codec/ac3
	kFskAudioFormatQCELP = 0x1050,				// x-audio-codec/qcelp
	kFskAudioFormatEVRC = 0x1060,				// x-audio-codec/evrc
	kFskAudioFormatAMRNB = 0x1070,				// x-audio-codec/amr-nb
	kFskAudioFormatAMRWB = 0x1080,				// x-audio-codec/amr-wb
	kFskAudioFormatWMA = 0x1090,				// x-audio-codec/wma
	kFskAudioFormatWMAVoice = 0x10A0,			// x-audio-codec/wma-voice
	kFskAudioFormatSPEEXNB = 0x10B0,			// x-audio-codec/speex-nb
	kFskAudioFormatSBC = 0x10C0,                // x-audio-codec/sbc
	kFskAudioFormatAppleLossless = 0x10D0       // x-audio-codec/apple-lossless
};

#if TARGET_RT_LITTLE_ENDIAN
	#define kFskAudioFormatPCM16BitNativeEndian kFskAudioFormatPCM16BitLittleEndian
#else
	#define kFskAudioFormatPCM16BitNativeEndian kFskAudioFormatPCM16BitBigEndian
#endif

#define kFskAudioOutDefaultOutputID (0)

enum FskAudioOutCategory {
	kFskAudioOutCategoryPlayback,
	kFskAudioOutCategoryAmbient,
};

enum {
	kFskAudioOutPropertyUndefined = 0,
	kFskAudioOutPropertyEQ,							// string
	kFskAudioOutPropertyCategory,						// enum FskAudioOutCategory
};

typedef FskErr (*AudioOutNewFunction)(FskAudioOut *audioOut, UInt32 outputID, UInt32 format);
typedef FskErr (*AudioOutDisposeFunction)(FskAudioOut audioOut);
typedef FskErr (*AudioOutIsValidFunction)(FskAudioOut audioOut, Boolean *isValid);
typedef FskErr (*AudioOutGetFormatFunction)(FskAudioOut audioOut, UInt32 *format, UInt16 *numChannels, double *sampleRate);
typedef FskErr (*AudioOutSetFormatFunction)(FskAudioOut audioOut, UInt32 format, UInt16 numChannels, double sampleRate, unsigned char *formatInfo, UInt32 formatInfoSize);
typedef FskErr (*AudioOutSetOutputBufferSizeFunction)(FskAudioOut audioOut, UInt32 chunkSize, UInt32 bufferedSamplesTarget);
typedef FskErr (*AudioOutGetOutputBufferSizeFunction)(FskAudioOut audioOut, UInt32 *chunkSize, UInt32 *bufferedSamplesTarget, UInt32 *minimumBytesToStart);
typedef FskErr (*AudioOutGetVolumeFunction)(FskAudioOut audioOut, UInt16 *left, UInt16 *right);
typedef FskErr (*AudioOutSetVolumeFunction)(FskAudioOut audioOut, UInt16 left, UInt16 right);
typedef FskErr (*AudioOutStartFunction)(FskAudioOut audioOut, FskSampleTime atSample);
typedef FskErr (*AudioOutStopFunction)(FskAudioOut audioOut);
typedef FskErr (*AudioOutEnqueueFunction)(FskAudioOut audioOut, void *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, UInt32 *frameSizes);
typedef FskErr (*AudioOutGetSamplePositionFunction)(FskAudioOut audioOut, FskSampleTime *position);
typedef FskErr (*AudioOutGetSamplesQueuedFunction)(FskAudioOut audioOut, UInt32 *samplesQueued, UInt32 *targetQueueLength);
typedef FskErr (*AudioOutSingleThreadedClientFunction)(FskAudioOut audioOut, Boolean *isSingleThreaded);
typedef FskErr (*AudioOutSetDoneCallbackFunction)(FskAudioOut audioOut, FskAudioOutDoneCallback cb, void *refCon);
typedef FskErr (*AudioOutSetMoreCallbackFunction)(FskAudioOut audioOut, FskAudioOutMoreCallback cb, void *refCon);
typedef FskErr (*AudioOutHasPropertyFunction)(FskAudioOut audioOut, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
typedef FskErr (*AudioOutSetPropertyFunction)(FskAudioOut audioOut, UInt32 propertyID, FskMediaPropertyValue property);
typedef FskErr (*AudioOutGetPropertyFunction)(FskAudioOut audioOut, UInt32 propertyID, FskMediaPropertyValue property);
typedef FskErr (*AudioOutGetDeviceHandle)(FskAudioOut audioOut, void *deviceHandle);

typedef struct AudioOutVectors {
	AudioOutNewFunction						doNew;
	AudioOutDisposeFunction					doDispose;
	AudioOutIsValidFunction					doIsValid;
	AudioOutGetFormatFunction				doGetFormat;
	AudioOutSetFormatFunction				doSetFormat;
	AudioOutSetOutputBufferSizeFunction		doSetOutputBufferSize;
	AudioOutGetVolumeFunction				doGetVolume;
	AudioOutSetVolumeFunction				doSetVolume;
	AudioOutSetDoneCallbackFunction			doSetDoneCallback;
	AudioOutSetMoreCallbackFunction			doSetMoreCallback;
	AudioOutStartFunction					doStart;
	AudioOutStopFunction					doStop;
	AudioOutEnqueueFunction					doEnqueue;
	AudioOutGetSamplePositionFunction		doGetSamplePosition;
	AudioOutGetSamplesQueuedFunction		doGetSamplesQueued;
	AudioOutSingleThreadedClientFunction	doSingleThreadedClient;
	AudioOutGetDeviceHandle					doGetDeviceHandle;
	AudioOutGetOutputBufferSizeFunction		doGetOutputBufferSize;
	AudioOutHasPropertyFunction				doHasProperty;
	AudioOutGetPropertyFunction				doGetProperty;
	AudioOutSetPropertyFunction				doSetProperty;
} AudioOutVectors, *AudioOutVectorSet;


FskAPI(void) FskAudioOutSetVectors(AudioOutVectorSet vectors);
FskAPI(void) FskAudioOutGetVectors(AudioOutVectorSet *vectors);

/*
	Audio output "driver" (i.e. platform dependent portion)
*/

FskAPI(FskErr) FskAudioOutNew(FskAudioOut *audioOut, UInt32 outputID, UInt32 format);
FskAPI(FskErr) FskAudioOutDispose(FskAudioOut audioOut);

FskAPI(FskErr) FskAudioOutIsValid(FskAudioOut audioOut, Boolean *isValid);

FskAPI(FskErr) FskAudioOutGetFormat(FskAudioOut audioOut, UInt32 *format, UInt16 *numChannels, double *sampleRate);
FskAPI(FskErr) FskAudioOutSetFormat(FskAudioOut audioOut, UInt32 format, UInt16 numChannels, double sampleRate, unsigned char *formatInfo, UInt32 formatInfoSize);
FskAPI(FskErr) FskAudioOutSetOutputBufferSize(FskAudioOut audioOut, UInt32 chunkSize, UInt32 bufferedSamplesTarget);
FskAPI(FskErr) FskAudioOutGetOutputBufferSize(FskAudioOut audioOut, UInt32 *chunkSize, UInt32 *bufferedSamplesTarget, UInt32 *minimumBytesToStart);

FskAPI(FskErr) FskAudioSetVolume(FskAudioOut audioOut, UInt16 left, UInt16 right);
FskAPI(FskErr) FskAudioGetVolume(FskAudioOut audioOut, UInt16 *left, UInt16 *right);

FskAPI(FskErr) FskAudioOutSetDoneCallback(FskAudioOut audioOut, FskAudioOutDoneCallback cb, void *refCon);
FskAPI(FskErr) FskAudioOutSetMoreCallback(FskAudioOut audioOut, FskAudioOutMoreCallback cb, void *refCon);

FskAPI(FskErr) FskAudioOutStart(FskAudioOut audioOut, FskSampleTime atSample);
FskAPI(FskErr) FskAudioOutStop(FskAudioOut audioOut);

FskAPI(FskErr) FskAudioOutEnqueue(FskAudioOut audioOut, void *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, UInt32 *frameSizes);
FskAPI(FskErr) FskAudioOutGetSamplePosition(FskAudioOut audioOut, FskSampleTime *position);
FskAPI(FskErr) FskAudioOutGetSamplesQueued(FskAudioOut audioOut, UInt32 *samplesQueued, UInt32 *targetQueueLength);

FskAPI(FskErr) FskAudioOutHasProperty(FskAudioOut audioOut, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskAudioOutSetProperty(FskAudioOut audioOut, UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskAudioOutGetProperty(FskAudioOut audioOut, UInt32 propertyID, FskMediaPropertyValue property);

FskAPI(FskErr) FskAudioOutSingleThreadedClient(FskAudioOut audioOut, Boolean *isSingleThreaded);

FskAPI(FskErr) FskAudioOutGetDeviceHandle(FskAudioOut audioOut, void *handle);

#if TARGET_OS_MAC
	void FskAudioOutRefillQueue(FskAudioOut audioOut);
	void FskAudioOutRemoveUnusedFromQueue(FskAudioOut audioOut);
#endif

enum {
	kFskAudioOutDontRestartOnReset = 1L << 0
};

/*
	Sound Channel
*/

typedef void (*FskSndChannelDoneCallback)(struct FskSndChannelRecord *sndChan, void *refCon, void *dataRefCon, Boolean played);
typedef FskErr (*FskSndChannelMoreCallback)(struct FskSndChannelRecord *sndChan, void *refCon, SInt32 requestedSamples);
typedef void (*FskSndChannelAbortCallback)(struct FskSndChannelRecord *sndChan, void *refCon, FskErr err);

struct FskSndChannelBlockRecord {
	struct FskSndChannelBlockRecord  *next;

	unsigned char		*data;
	UInt32				dataSize;
	UInt32				frameCount;
	UInt32				samplesPerFrame;
	SInt32				samplesToSkip;			// positive number is samples to skip at front of block, negative number is silence samples to insert before start of block
	Boolean				processed;

	void				*refCon;

	void				*sndChan;

	UInt32				*frameSizes;			// data is at end of block
	UInt32				dummy;
};

typedef struct FskSndChannelBlockRecord FskSndChannelBlockRecord;
typedef FskSndChannelBlockRecord *FskSndChannelBlock;

#if !defined(__FSKAUDIO_PRIV__) && !SUPPORT_INSTRUMENTATION
	FskDeclarePrivateType(FskSndChannel)
#else
	struct FskSndChannelRecord {
		struct FskSndChannelRecord	*next;

		UInt32						outputID;
		FskAudioOut					audioOut;

		FskThread					thread;

		UInt32						outSampleRate;
		UInt32						outFormat;
		UInt16						outNumChannels;

		UInt32						inSampleRate;
		UInt32						inFormat;
		char						*inMIME;
		UInt16						inNumChannels;
		unsigned char				*inFormatInfo;
		UInt32						inFormatInfoSize;

		float						volume;
		UInt16						effectiveVolume;
		UInt16						volL;
		UInt16						volR;

		FskErr						err;

		SInt32						pan;

		Boolean						playing;
		Boolean						threadSafeClient;
		FskTimeCallBack				refillCallback;
		FskListMutex				playBlocks;
		FskMutex					mutex;

		FskMutex					mutexBlocks;		// to protect access to blocks
		FskSndChannelBlock			blocks;

		FskSndChannelMoreCallback	moreCB;
		void						*moreRefCon;

		FskSndChannelDoneCallback	doneCB;
		void						*doneRefCon;

		FskSndChannelAbortCallback	abortCB;
		void						*abortRefCon;

		FskAudioDecompress			deco;
		UInt32						decompressedSampleRate;
		FskAudioFilter				rateConverter;
		FskAudioFilter				timeScaler;
		FskSampleTime				startSample;
		double						playRate;
		Boolean						timeScalerDoesRateConversion;
		FskAudioFilter				toneController;
		char						*toneSettings;

		FskMediaPropertyValueRecord	trInfo;

		FskInstrumentedItemDeclaration
	};

	typedef struct FskSndChannelRecord FskSndChannelRecord;
	typedef FskSndChannelRecord *FskSndChannel;
#endif

enum {
	kFskSndChannelThreadSafeClient = 1L << 0
};

FskAPI(FskErr) FskSndChannelNew(FskSndChannel *sndChan, UInt32 outputID, UInt32 format, UInt32 flags);
FskAPI(FskErr) FskSndChannelDispose(FskSndChannel sndChan);

FskAPI(FskErr) FskSndChannelSetDoneCallback(FskSndChannel sndChan, FskSndChannelDoneCallback cb, void *refCon);
FskAPI(FskErr) FskSndChannelSetMoreCallback(FskSndChannel sndChan, FskSndChannelMoreCallback cb, void *refCon);
FskAPI(FskErr) FskSndChannelSetAbortCallback(FskSndChannel sndChan, FskSndChannelAbortCallback cb, void *refCon);

FskAPI(FskErr) FskSndChannelSetFormat(FskSndChannel sndChan, UInt32 format, const char *mime, UInt16 numChannels, double sampleRate,
					unsigned char *formatInfo, UInt32 formatInfoSize);
FskAPI(FskErr) FskSndChannelGetFormat(FskSndChannel sndChan, UInt32 *format, char **mime, UInt16 *numChannels, double *sampleRate,
					unsigned char **formatInfo, UInt32 *formatInfoSize);

FskAPI(FskErr) FskSndChannelSetVolume(FskSndChannel sndChan, float volume);
FskAPI(FskErr) FskSndChannelGetVolume(FskSndChannel sndChan, float *volume);

FskAPI(FskErr) FskSndChannelSetPan(FskSndChannel sndChan, SInt32 pan);
FskAPI(FskErr) FskSndChannelGetPan(FskSndChannel sndChan, SInt32 *pan);

FskAPI(FskErr) FskSndChannelStart(FskSndChannel sndChan, FskSampleTime atSample);
FskAPI(FskErr) FskSndChannelStop(FskSndChannel sndChan);

FskAPI(FskErr) FskSndChannelGetSamplePosition(FskSndChannel sndChan, FskSampleTime *atSample);

FskAPI(FskErr) FskSndChannelEnqueue(FskSndChannel sndChan, void *data, UInt32 dataSize, UInt32 frameCount, UInt32 samplesPerFrame, void *dataRefCon, UInt32 *frameSizes);
FskAPI(FskErr) FskSndChannelEnqueueWithSkip(FskSndChannel sndChan, void *data, UInt32 dataSize, UInt32 frameCount, UInt32 samplesPerFrame, void *dataRefCon, UInt32 *frameSizes, SInt32 samplesToSkip);

FskAPI(FskErr) FskSndChannelHasProperty(FskSndChannel sndChan, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskSndChannelSetProperty(FskSndChannel sndChan, UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskSndChannelGetProperty(FskSndChannel sndChan, UInt32 propertyID, FskMediaPropertyValue property);

/*
	Audio codecs
*/

#define FskAudioDecompressorInstall(a) (FskExtensionInstall(kFskExtensionAudioDecompressor, a))
#define FskAudioDecompressorUninstall(a) (FskExtensionUninstall(kFskExtensionAudioDecompressor, a))

#define FskAudioCompressorInstall(a) (FskExtensionInstall(kFskExtensionAudioCompressor, a))
#define FskAudioCompressorUninstall(a) (FskExtensionUninstall(kFskExtensionAudioCompressor, a))

FskErr FskAudioCodecInitialize(void);

/*
	Audio decompressor
*/


typedef FskErr(*FskAudioDecompressorCanHandle)(UInt32 format, const char *mime, Boolean *canHandle);
typedef FskErr (*FskAudioDecompressorNew)(FskAudioDecompress deco, UInt32 format, const char *mime);
typedef FskErr (*FskAudioDecompressorDispose)(void *state, FskAudioDecompress deco);
typedef FskErr (*FskAudioDecompressorDecompressFrames)(void *state, FskAudioDecompress deco, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, void **samples, UInt32 *samplesSize);
typedef FskErr (*FskAudioDecompressorDiscontinuity)(void *state, FskAudioDecompress deco);

typedef struct {
	FskAudioDecompressorCanHandle			doCanHandle;
	FskAudioDecompressorNew					doNew;
	FskAudioDecompressorDispose				doDispose;
	FskAudioDecompressorDecompressFrames	doDecompressFrames;
	FskAudioDecompressorDiscontinuity		doDiscontinuity;

	FskMediaPropertyEntry					properties;
} FskAudioDecompressorRecord, *FskAudioDecompressor;

#if defined(__FSKAUDIOCODEC_PRIV__)
	struct FskAudioDecompressRecord {
		UInt32					inputSampleRate;
		UInt32					inputChannelCount;

		UInt32					frameNumber;

		UInt32					requestedOutputFormat;

		void					*formatInfo;
		UInt32					formatInfoSize;

		UInt32					outputFormat;
		UInt32					outputChannelCount;

		void					*state;
		FskAudioDecompressor	decoder;

		FskInstrumentedItemDeclaration
	};
#endif

FskAPI(FskErr) FskAudioDecompressNew(FskAudioDecompress *deco, UInt32 audioFormat, const char *mimeType, UInt32 sampleRate, UInt32 channelCount, void *formatInfo, UInt32 formatInfoSize);
FskAPI(FskErr) FskAudioDecompressDispose(FskAudioDecompress deco);

FskAPI(FskErr) FskAudioDecompressRequestedOutputFormat(FskAudioDecompress deco, UInt32 audioFormat);

FskAPI(FskErr) FskAudioDecompressFrames(FskAudioDecompress deco, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, void **samples, UInt32 *audioFormat, UInt32 *sampleCount, UInt32 *channelCount);
FskAPI(FskErr) FskAudioDecompressDiscontinuity(FskAudioDecompress deco);

// properties
FskAPI(FskErr) FskAudioDecompressHasProperty(FskAudioDecompress deco, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskAudioDecompressSetProperty(FskAudioDecompress deco, UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskAudioDecompressGetProperty(FskAudioDecompress deco, UInt32 propertyID, FskMediaPropertyValue property);

// helper
FskAPI(FskErr) FskAudioMIMEToFormat(const char *mime, UInt32 *audioFormat);

#if SUPPORT_INSTRUMENTATION

enum {
	kFskAudioDecompressInstrDiscontinuity = kFskInstrumentedItemFirstCustomMessage,
	kFskAudioDecompressInstrDecompress,
	kFskAudioDecompressInstrDecompressFailed
};

#endif

/*
	Audio compressor
*/

typedef FskErr(*FskAudioCompressorCanHandle)(const char *format, Boolean *canHandle);
typedef FskErr (*FskAudioCompressorNew)(FskAudioCompress comp);
typedef FskErr (*FskAudioCompressorDispose)(void *state, FskAudioCompress comp);
typedef FskErr (*FskAudioCompressorCompressFrames)(void *state, FskAudioCompress comp, const void *data, UInt32 dataSize, UInt32 inSampleCount, void **outSamples, UInt32 *outDataSize, UInt32 **outFrameSizes, UInt32 *frameCount);

typedef struct {
	FskAudioCompressorCanHandle				doCanHandle;
	FskAudioCompressorNew					doNew;
	FskAudioCompressorDispose				doDispose;
	FskAudioCompressorCompressFrames		doCompressFrames;
	FskMediaPropertyEntry					properties;			// module sets - point to array of FskMediaPropertyEntry - terminated with kFskMediaPropertyUndefined
} FskAudioCompressorRecord, *FskAudioCompressor;

#if defined(__FSKAUDIOCODEC_PRIV__)
	struct FskAudioCompressRecord {
		char					*inputFormat;
		UInt32					inputSampleRate;
		UInt32					inputChannelCount;

		UInt32					requestedBitRate;

		UInt32					frameNumber;

		void					*desc;
		UInt32					descSize;

		char					*outputFormat;
		UInt32					outputSampleRate;
		UInt32					outputChannelCount;
		UInt32					outputSamplesPerFrame;

		void					*state;
		FskAudioCompressor		encoder;
	};
#endif

FskAPI(FskErr) FskAudioCompressNew(FskAudioCompress *comp, const char *outputFormat, const char *inputFormat, UInt32 inputSampleRate, UInt32 inputChannelCount);
FskAPI(FskErr) FskAudioCompressDispose(FskAudioCompress comp);

FskAPI(FskErr) FskAudioCompressGetDescription(FskAudioCompress comp, void **desc, UInt32 *descSize);
FskAPI(FskErr) FskAudioCompressSetBitRate(FskAudioCompress comp, UInt32 bitsPerSecond);

FskAPI(FskErr) FskAudioCompressFrames(FskAudioCompress comp, const void *data, UInt32 dataSize, UInt32 inSampleCount, void **outSamples, UInt32 *outDataSize, UInt32 **outFrameSizes, UInt32 *frameCount, UInt32 *sampleCount, UInt32 *outSampleRate, UInt32 *outChannelCount);

FskAPI(FskErr) FskAudioCompressHasProperty(FskAudioCompress comp, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskAudioCompressGetProperty(FskAudioCompress comp, UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskAudioCompressSetProperty(FskAudioCompress comp, UInt32 propertyID, FskMediaPropertyValue property);

#ifdef __cplusplus
}
#endif

#endif
