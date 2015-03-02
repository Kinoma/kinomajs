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
#ifndef __FSKIMAGE__
#define __FSKIMAGE__

#define SUPPORT_ASYNC_IMAGE_DECOMPRESS 1
#define SUPPORT_ASYNC_IMAGE_COMPRESS 1

#include "FskBitmap.h"
#include "FskExtensions.h"
#include "FskHeaders.h"
#include "FskMedia.h"
#include "FskThread.h"
#include "FskUtilities.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FskImageDecompressorInstall(a) (FskExtensionInstall(kFskExtensionImageDecompressor, a))
#define FskImageDecompressorUninstall(a) (FskExtensionUninstall(kFskExtensionImageDecompressor, a))

#define FskImageCompressorInstall(a) (FskExtensionInstall(kFskExtensionImageCompressor, a))
#define FskImageCompressorUninstall(a) (FskExtensionUninstall(kFskExtensionImageCompressor, a))

/*
	Decompress
*/

typedef struct FskImageDecompressRecord FskImageDecompressRecord;
typedef FskImageDecompressRecord *FskImageDecompress;

typedef FskErr(*FskImageDecompressorCanHandle)(UInt32 format, const char *mime, const char *extension, Boolean *canHandle);
typedef FskErr (*FskImageDecompressorNew)(FskImageDecompress deco, UInt32 format, const char *mime, const char *extension);
typedef FskErr (*FskImageDecompressorDispose)(void *state, FskImageDecompress deco);
typedef FskErr (*FskImageDecompressorDecompressFrame)(void *state, FskImageDecompress deco, const void *data, UInt32 dataSize, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType);

typedef FskErr (*FskImageDecompressorSetData)(void *state, FskImageDecompress deco, void *data, UInt32 dataSize);
typedef FskErr (*FskImageDecompressorGetMetaData)(void *state, FskImageDecompress deco, UInt32 metadata, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);

typedef FskErr (*FskImageDecompressorSniff)(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);
typedef FskErr (*FskImageDecompressorFlush)(void *state, FskImageDecompress deco);

typedef struct {
	FskImageDecompressorCanHandle			doCanHandle;
	FskImageDecompressorNew					doNew;
	FskImageDecompressorDispose				doDispose;
	FskImageDecompressorDecompressFrame		doDecompressFrame;
	FskImageDecompressorSetData				doSetData;
	FskImageDecompressorGetMetaData			doGetMetaData;

	FskMediaPropertyEntry					properties;

	FskImageDecompressorSniff				doSniff;
	FskImageDecompressorFlush				doFlush;
} FskImageDecompressorRecord, *FskImageDecompressor;

typedef void (*FskImageDecompressComplete)(FskImageDecompress deco, void *refcon, FskErr result, FskBitmap bits);

#if defined(__FSKIMAGE_PRIV__) || SUPPORT_INSTRUMENTATION
	struct FskImageDecompressRecord {
		UInt32					requestedWidth;
		UInt32					requestedHeight;
		FskBitmapFormatEnum		requestedPixelFormat;

		UInt32					frameNumber;
		Boolean					initializeComplete;

		FskBitmap				bits;
		Boolean					privateBits;

		FskImageDecompressComplete	completionFunction;
		void						*completionRefcon;

		void					*state;
		FskImageDecompressor	decoder;

		void					*data;
		UInt32					dataSize;

	#if SUPPORT_ASYNC_IMAGE_DECOMPRESS
		SInt16					flushing;
		SInt16					decompressing;
	#endif

		FskMediaSpooler			spooler;
		void					*spooled;

		FskInstrumentedItemDeclaration
	};
#endif

FskErr FskImageCodecInitialize(void);
FskErr FskImageCodecTerminate(void);

FskAPI(FskErr) FskImageDecompressSniffForMIME(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

// instance management
FskAPI(FskErr) FskImageDecompressNew(FskImageDecompress *deco, UInt32 format, const char *mimeType, const char *extension);
FskAPI(FskErr) FskImageDecompressDispose(FskImageDecompress deco);

// decompressing
FskAPI(FskErr) FskImageDecompressRequestSize(FskImageDecompress deco, UInt32 width, UInt32 height);
FskAPI(FskErr) FskImageDecompressRequestedOutputFormat(FskImageDecompress deco, FskBitmapFormatEnum pixelFormat);

FskAPI(FskErr) FskImageDecompressFrame(FskImageDecompress deco, const void *data, UInt32 dataSize, FskBitmap *bits, Boolean ownIt, FskImageDecompressComplete completion, void *completionRefcon, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, void *asyncRef, UInt32 frameType);

FskAPI(FskErr) FskImageDecompressFlush(FskImageDecompress deco);

// properties
FskAPI(FskErr) FskImageDecompressHasProperty(FskImageDecompress deco, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskImageDecompressSetProperty(FskImageDecompress deco, UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskImageDecompressGetProperty(FskImageDecompress deco, UInt32 propertyID, FskMediaPropertyValue property);

// parsing routines
FskAPI(FskErr) FskImageDecompressSetData(FskImageDecompress deco, void *data, UInt32 dataSize);
FskAPI(FskErr) FskImageDecompressGetMetaData(FskImageDecompress deco, UInt32 metaData, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);

enum {
	kFskImageDecompressMetaDataDimensions = 1,
	kFskImageDecompressMetaDataBitDepth,
	kFskImageDecompressMetaDataThumbnail,
	kFskImageDecompressMetaDataOrientation,
	kFskImageDecompressMetaDataHeadersParsed,
	kFskImageDecompressMetaDataFrameType
};

enum {
	kFskImageMetdataExifMainImage = 0x40000000,
	kFskImageMetdataExifThumbnail = 0x41000000,

	kFskImageMetdataExifTIFF = 0x00000000,
	kFskImageMetdataExifExifPrivate = 0x00010000,
	kFskImageMetdataExifGPS = 0x00020000
};

// helper function
FskAPI(FskErr) FskImageDecompressData(const void *data, UInt32 dataSize, const char *mimeType, const char *extension, UInt32 targetWidth, UInt32 targetHeight, FskImageDecompressComplete completion, void *completionRefcon, FskBitmap *bits);

#if SUPPORT_INSTRUMENTATION

enum {
	kFskImageDecompressInstrMsgDecompressQueue,
	kFskImageDecompressInstrMsgDecompressFrameBegin,
	kFskImageDecompressInstrMsgDecompressFrameComplete,
	kFskImageDecompressInstrMsgDecompressFlush
};

#endif

/*
	Compress
*/

enum {
	kFskImageFrameTypeSync			= 0,
	kFskImageFrameTypeDifference 	= 1 << 0,
	kFskImageFrameTypePartialSync 	= 1 << 1,
	kFskImageFrameTypeDroppable 	= 1 << 2,

	kFskImageFrameEndOfMedia		= 1L << 28,		// end of track (media readers only)
	kFskImageFrameImmediate			= 1L << 29,
	kFskImageFrameGap				= 1L << 30,		// reserved for media readers
	kFskImageFrameDrop			 	= 1L << 31,

	kFskImageTypeMask				= 0x0000ffff
};

typedef struct FskImageCompressRecord FskImageCompressRecord;
typedef FskImageCompressRecord *FskImageCompress;

typedef FskErr(*FskImageCompressorCanHandle)(UInt32 format, const char *mime, Boolean *canHandle);
typedef FskErr (*FskImageCompressorNew)(FskImageCompress comp);
typedef FskErr (*FskImageCompressorDispose)(void *state, FskImageCompress comp);
typedef FskErr (*FskImageCompressorCompressFrame)(void *state, FskImageCompress comp, FskBitmap bits, const void **data, UInt32 *dataSize, UInt32 *frameDuration, UInt32 *compositionTimeOffset, UInt32 *frameType);

typedef struct {
	FskImageCompressorCanHandle			doCanHandle;
	FskImageCompressorNew				doNew;
	FskImageCompressorDispose			doDispose;
	FskImageCompressorCompressFrame		doCompressFrame;
	FskMediaPropertyEntry				properties;			// module sets - point to array of FskMediaPropertyEntry - terminated with kFskMediaPropertyUndefined
} FskImageCompressorRecord, *FskImageCompressor;

#if defined(__FSKIMAGE_PRIV__) || SUPPORT_INSTRUMENTATION
	struct FskImageCompressRecord {
		FskBitmapFormatEnum		format;
		const char				*mimeType;

		UInt32					width;
		UInt32					height;

		UInt32					bitrate;

		void					*esds;
		UInt32					esdsSize;

		UInt32					timeScale;

		UInt32					frameNumber;
		UInt32					frameDuration;			// microseconds

		void					*state;
		FskImageCompressor		encoder;

#if SUPPORT_ASYNC_IMAGE_COMPRESS
		SInt16					flushing;
		SInt16					compressing;
#endif

		FskInstrumentedItemDeclaration
	};
#endif

typedef void (*FskImageCompressComplete)(FskImageCompress comp, void *refcon, FskErr result, void *data, UInt32 dataSize);

FskAPI(FskErr) FskImageCompressNew(FskImageCompress *comp, UInt32 format, const char *mimeType, UInt32 width, UInt32 height);
FskAPI(FskErr) FskImageCompressDispose(FskImageCompress comp);

FskAPI(FskErr) FskImageCompressSetBitrate(FskImageCompress comp, UInt32 bitrate);
FskAPI(FskErr) FskImageCompressSetFrameDuration(FskImageCompress comp, UInt32 microseconds);		//@@ go away??!
FskAPI(FskErr) FskImageCompressSetTimeScale(FskImageCompress comp, UInt32 scale);

FskAPI(FskErr) FskImageCompressFrame(FskImageCompress comp, FskBitmap bits, void **data, UInt32 *dataSize, UInt32 *frameDuration, UInt32 *compositionTimeOffset, UInt32 *frameType, FskImageCompressComplete completion, void *completionRefcon);

FskAPI(FskErr) FskImageCompressFlush(FskImageCompress comp);

FskAPI(FskErr) FskImageCompressGetElementaryStreamDescriptor(FskImageCompress comp, void **esds, UInt32 *esdsSize);

// properties
FskAPI(FskErr) FskImageCompressHasProperty(FskImageCompress comp, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskImageCompressSetProperty(FskImageCompress comp, UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskImageCompressGetProperty(FskImageCompress comp, UInt32 propertyID, FskMediaPropertyValue property);

#ifdef __cplusplus
}
#endif

#endif
