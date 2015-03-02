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
#ifndef __FSKMEDIA__
#define __FSKMEDIA__

#include "Fsk.h"
#include "FskRectangle.h"
#include "xs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    UInt32			movieDuration;
    SInt32			trackStartTime;
	SInt32			rate;
} FskMediaEditRecord, *FskMediaEdit;

typedef struct {
	UInt32						count;
	FskMediaEditRecord			edit[1];					// zero or more entries
} FskMediaEditListRecord, *FskMediaEditList;

typedef struct {
	UInt32						trackIndex;
	UInt32						editIndex;
} FskMediaEditInfoRecord, *FskMediaEditInfo;

typedef struct {
	char						str[32];
	UInt32						paramCount;
	UInt32						param[1];					// zero or more entries
} FskMediaMessageRecord, *FskMediaMessage;

typedef struct {
	xsSlot						object;
	xsMachine					*the;
} FskMediaXSObjectRecord, *FskMediaXSObject;

typedef struct FskMediaDRMKeyRecord FskMediaDRMKeyRecord;
typedef struct FskMediaDRMKeyRecord *FskMediaDRMKey;

struct FskMediaDRMKeyRecord {
	FskErr				(*transform)(FskMediaDRMKey key, const void *din, void *dout, UInt32 dataSizeIn, UInt32 *dataSizeOut, UInt32 sampleNumber);
	void				*refCon;

	UInt32				numSamplesBackup;

	UInt32				maxEncryptionSlop;
};

typedef struct FskMediaSpoolerRecord FskMediaSpoolerRecord;
typedef struct FskMediaSpoolerRecord *FskMediaSpooler;

enum {
	kFskMediaSpoolerValid = 1L << 0,
	kFskMediaSpoolerDontSeekIfExpensive = 1L << 1,
	kFskMediaSpoolerDownloadPreferred = 1L << 2,
	kFskMediaSpoolerDontDownload = 1L << 3,
	kFskMediaSpoolerTransformReceivedData = 1L << 4,
	kFskMediaSpoolerDownloading = 1L << 5,
	kFskMediaSpoolerIsNetwork = 1L << 6,
	kFskMediaSpoolerCantSeek = 1L << 7,
	kFskMediaSpoolerForwardOnly = 1L << 8,
    kFskMediaSpoolerTimeSeekSupported = 1L << 9,
    kFskMediaSpoolerUseTimeSeek = 1L << 10,
    kFskMediaSpoolerByteSeekSupported = 1L << 11
};

enum {
	kFskMediaSpoolerOperationDataReady = 1,			// param is number of bytes that arrived
	kFskMediaSpoolerOperationSetHeaders,			// param is FskHeader, callback may add headers
	kFskMediaSpoolerOperationGetHeaders,			// param is FskHeader, callback may read headers
	kFskMediaSpoolerOperationGetURI,				// params is FskMediaSpoolerGetURIRecord
	kFskMediaSpoolerOperationTransformData			// params is FskMediaSpoolerTransformDataRecord

};

typedef struct {
	char				*uriIn;
	FskInt64			position;
	char				*uriOut;
	Boolean				doSetPosition;
} FskMediaSpoolerGetURIRecord, *FskMediaSpoolerGetURI;

typedef struct {
	char				*data;
	UInt32				dataSize;
} FskMediaSpoolerTransformDataRecord, *FskMediaSpoolerTransformData;

struct FskMediaSpoolerRecord {
	void						*refcon;
	UInt32						flags;

	FskErr						(*doOpen)(FskMediaSpooler spooler, UInt32 permissions);
	FskErr						(*doClose)(FskMediaSpooler spooler);
	FskErr						(*doRead)(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesRead);
	FskErr						(*doWrite)(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToWrite, const void *buffer, UInt32 *bytesWritten);
	FskErr						(*doGetSize)(FskMediaSpooler spooler, FskInt64 *size);

	// client callbacks
	void						*clientRefCon;
	FskErr						(*onSpoolerCallback)(void *clientRefCon, UInt32 operation, void *param);
};

#define FskMediaSpoolerRead(spooler, position, bytesToRead, buffer, bytesRead) ((spooler->doRead)(spooler, position, bytesToRead, (void **)(void *)(buffer), bytesRead))

typedef struct FskRatioRecord FskRatioRecord;
typedef struct FskRatioRecord *FskRatio;

struct FskRatioRecord {
	SInt32		numer;
	SInt32		denom;
};

typedef union {
	SInt32					integer;
	double					number;
	Boolean					b;
	char					*str;
	FskRectangleRecord		rect;
	FskPointRecord			point;
	FskDimensionRecord		dimension;
	struct {
		void				*data;
		UInt32				dataSize;
	} data;
	FskMediaDRMKey			key;
	FskMediaEditList		editList;
	FskMediaMessage			message;
	FskMediaXSObject		xsObject;
	struct {
		UInt32				count;
		UInt32				*integer;
	} integers;
	FskMediaSpoolerRecord	spooler;
	FskRatioRecord			ratio;
	struct {
		UInt32				count;
		double				*number;
	} numbers;
	void					*sprites;
	void					*bitmap;
} fskPropertyRecord;

typedef struct {
	UInt32				type;
	fskPropertyRecord	value;
} FskMediaPropertyValueRecord, *FskMediaPropertyValue;

typedef FskErr (*FskMediaPropertyGet)(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
typedef FskErr (*FskMediaPropertySet)(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);

typedef struct {
	UInt32									id;
	UInt32									dataType;
	FskMediaPropertyGet						get;
	FskMediaPropertySet						set;
} FskMediaPropertyEntryRecord, *FskMediaPropertyEntry;

enum {
	kFskMediaPropertyTypeUndefined = 0,
	kFskMediaPropertyTypeInteger = 1,
	kFskMediaPropertyTypeFloat = 2,
	kFskMediaPropertyTypeBoolean = 3,
	kFskMediaPropertyTypeString = 4,
	kFskMediaPropertyTypeRectangle = 5,
	kFskMediaPropertyTypePoint = 6,
	kFskMediaPropertyTypeData = 7,
	kFskMediaPropertyTypeKey = 8,
	kFskMediaPropertyTypeEditList = 9,
	kFskMediaPropertyTypeMessage = 10,
	kFskMediaPropertyTypeXSObject = 11,
	kFskMediaPropertyTypeStringList = 12,
	kFskMediaPropertyTypeImage = 13,
	kFskMediaPropertyTypeUInt32List = 14,
	kFskMediaPropertyTypeDimension = 15,
	kFskMediaPropertyTypeSpooler = 16,
	kFskMediaPropertyTypeRatio = 17,
	kFskMediaPropertyTypeFloatList = 18,
	kFskMediaPropertyTypeSprites = 19,
	kFskMediaPropertyTypeBitmap = 20
};

enum {
	kFskMediaPropertyUndefined = 0,
	kFskMediaPropertyEnabled = 1,					// boolean
	kFskMediaPropertyMediaType = 2,					// string
	kFskMediaPropertyDRMKey = 3,					// key
	kFskMediaPropertyDRMInfo = 4,					// data
	kFskMediaPropertyTimeScale = 5,					// integer
	kFskMediaPropertyFlags = 6,						// integer
	kFskMediaPropertyDuration = 7,					// double
	kFskMediaPropertyEditList = 8,					// FskMediaEditList
	kFskMediaPropertyDimensions = 9,				// FskDimension
	kFskMediaPropertyFormat = 10,					// string
	kFskMediaPropertyESDS = 11,						// binary
	kFskMediaPropertyBitRate = 12,					// integer
	kFskMediaPropertySampleRate = 13,				// integer
	kFskMediaPropertyChannelCount = 14,				// integer
	kFskMediaPropertySecurityType = 15,				// binary
	kFskMediaPropertySecurityData = 16,				// binary
	kFskMediaPropertyID = 17,						// integer
	kFskMediaPropertyMessage = 18,					// FskMediaPlayerMessage
	kFskMediaPropertyClip = 19,						// rectangle
	kFskMediaPropertyXSObject= 20,
	kFskMediaPropertyCrop = 21,						// rectangle
	kFskMediaPropertyDataSize = 22,					// integer
	kFskMediaPropertyRequestHeaders = 23,			// string list
	kFskMediaPropertySamplesPerChunk = 24,			// integer
	kFskMediaPropertyVolume = 25,					// double
	kFskMediaPropertyPixelFormat = 26,				// kFskMediaPropertyTypeUInt32List
	kFskMediaPropertyQuality = 27,					// float
	kFskMediaPropertyEncryptedMetaData = 28,		// string list
	kFskMediaPropertyTRInfo = 29,					// binary
	kFskMediaPropertyOMATagMode = 30,				// binary
	kFskMediaPropertyMetaDataItems = 31,			// string list
	kFskMediaPropertyProfile = 32,					// string
	kFskMediaPropertyCanChangeSampleRate = 33,		// boolean
	kFskMediaPropertyCanChangeChannelCount = 34,	// boolean
	kFskMediaPropertyPlayRate = 35,					// float
	kFskMediaPropertySpooler = 36,					// FskMediaSpoolerRecord
	kFskMediaPropertyTime = 37,						// double
	kFskMediaPropertyState = 38,					// integer
	kFskMediaPropertyFrameRate = 39,				// FskRatio
	kFskMediaPropertyKeyFrameRate = 40,				// integer
	kFskMediaPropertyCompressionSettings = 41,		// string
	kFskMediaPropertyPixelAspectRatio = 42,			// FskRatio
	kFskMediaPropertyBalance = 43,					// float
	kFskMediaPropertyBuffer = 44,					// float
	kFskMediaPropertyContrast = 45,					// float
	kFskMediaPropertyBrightness = 46,				// float
	kFskMediaPropertyBufferDuration = 47,			// integer - units of TimeScale.
	kFskMediaPropertyScrub = 48,					// boolean
	kFskMediaPropertyEQ = 49,						// string - will probably change
	kFskMediaPropertyRedirect = 50,					// string
	kFskMediaPropertyError = 51,					// integer - negative is FskError, positive is http/rtsp error
	kFskMediaPropertyMarkTimes = 52,				// number list - times in seconds
	kFskMediaPropertyAuthentication = 53,			// string list - [user name, password]
	kFskMediaPropertyAuthorization = 54,			// string list - [host, realm, user name]
	kFskMediaPropertySeekableSegment = 55,			// double - (same units as duration)
	kFskMediaPropertyDownloadPath = 56,				// string
	kFskMediaPropertyDownloadPosition = 57,			// double (for 4GB+)
	kFskMediaPropertyDownload = 58,					// integer - 0 == no preference, 1 == wants download
	kFskMediaPropertySprites = 59,					// FskVideoSpriteWorld
	kFskMediaPropertyRotation = 60,					// double - rotation in degrees
	kFskMediaPropertyLocation = 61,					// string - current URL (after redirects)
	kFskMediaPropertyHibernate = 62,				// boolean
	kFskMediaPropertyLocal = 63,                    // boolean
	kFskMediaPropertyAudioCategory = 64,			// integer
	

	kFskMediaPropertyNext = 200,					// string
    kFskMediaPropertyPosition = 201,				// integer
	kFskMediaPropertySampleCount = 202,				// integer
	kFskMediaPropertyMaxFramesToQueue = 203,			// integer
	
	kFskMediaPropertyFirstModuleSpecific = 0x4000,	// beginning of range for particular module to extend property list

	kFskMediaPropertyPerformanceInfo = 0x6000,		// private
	kFskMediaPropertyPlayMode = 0x6010,		// private

	kFskMediaPropertySampleDescription = 0x8000,
	kFskMediaPropertySampleNumber = 0x8001,

    kFskMediaPropertyLens = 0xa001,                 // string "back", "front"
    kFskMediaPropertyAutoFocusState = 0xa002,       // integer 0: not focused, not focusing, 1: focusing, 2: focused
    kFskMediaPropertyAutoFocusArea = 0xa003,        // FskRectangleRecord
    //kFskMediaPropertyDimensionIndex = 0xa004,       // integer
    kFskMediaPropertyDimensionsList = 0xa004,       //integer list
    
//	kFskMediaPropertyFirstDVD = 0x4000						// DVD player uses 0x4000-0x40FF
	kFskMediaPropertyFirstCapture = 0x4100,					// capture uses 0x4100-0x41FF
	kFskMediaPropertyFirstCaptureTrack = 0x4200, 			// capture track uses 0x4200-0x42FF
	//kFskMediaPropertyExtraDataBegins = 0x5100,			// beginning of range for extra Data
	kFskMediaPropertyExtraDataMP4DecSpecificInfo = 20737,	// binary
	kFskMediaPropertyExtraDataMP2VSequenceHeader = 20738,	// binary
	kFskMediaPropertyExtraDataATRACFormatInfo = 20739,		// binary
	kFskMediaPropertyFormatInfo = 20740, // binary

	kFskMediaPropertyDLNASources = 20750,					// string list
	kFskMediaPropertyDLNASinks = 20751						// string list
};

/*
	help dispatching array of FskMediaPropertyEntry
*/

FskAPI(FskErr) FskMediaHasProperty(FskMediaPropertyEntry properties, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskMediaSetProperty(FskMediaPropertyEntry properties, void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskMediaGetProperty(FskMediaPropertyEntry properties, void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);

/*
	convenient
*/

FskAPI(FskErr) FskMediaPropertyCopy(const FskMediaPropertyValue from, FskMediaPropertyValue to);
FskAPI(FskErr) FskMediaPropertyEmpty(FskMediaPropertyValue property);

/*
	type coercion support for xs glue
*/

typedef FskErr (*FskMediaSetPropertyXSHelperProc)(void *obj, UInt32 propertyID, FskMediaPropertyValue value);

FskAPI(FskErr) FskMediaSetPropertyXSHelper(xsMachine *the, UInt32 dataType, UInt32 property, xsIndex argIndex, FskMediaSetPropertyXSHelperProc setProc, void *obj);
FskAPI(FskErr) FskMediaGetPropertyXSHelper(xsMachine *the, FskMediaPropertyValue property);

/*
	metadata helper
*/

enum {
	kFskMediaMetaDataFlagReference = 1 << 0,
	kFskMediaMetaDataFlagOwnIt = 1 << 1,
	kFskMediaMetaDataFlagReadOnly = 1 << 2,
	kFskMediaMetaDataFlagEncrypted = 1 << 3
};

typedef struct FskMediaMetaDataIDRecord FskMediaMetaDataIDRecord;
typedef struct FskMediaMetaDataIDRecord *FskMediaMetaDataID;

typedef struct FskMediaMetaDataItemRecord FskMediaMetaDataItemRecord;
typedef struct FskMediaMetaDataItemRecord *FskMediaMetaDataItem;

struct FskMediaMetaDataIDRecord {
	struct FskMediaMetaDataIDRecord				*next;
	
	struct FskMediaMetaDataItemRecord			*items;

	char										idName[1];		// name
};

struct FskMediaMetaDataItemRecord {
	struct FskMediaMetaDataItemRecord			*next;

	FskMediaPropertyValueRecord					value;
	UInt32										flags;
};

typedef struct FskMediaMetaDataRecord FskMediaMetaDataRecord;
typedef struct FskMediaMetaDataRecord *FskMediaMetaData;

struct FskMediaMetaDataRecord {
	FskMediaMetaDataID							ids;
    FskInstrumentedItemDeclaration
};

FskAPI(FskErr) FskMediaMetaDataNew(FskMediaMetaData *meta);
FskAPI(FskErr) FskMediaMetaDataDispose(FskMediaMetaData meta);

FskAPI(FskErr) FskMediaMetaDataAdd(FskMediaMetaData meta, const char *id, UInt32 *index, const FskMediaPropertyValue value, UInt32 flags);
FskAPI(FskErr) FskMediaMetaDataRemove(FskMediaMetaData meta, const char *id, UInt32 index);

FskAPI(FskErr) FskMediaMetaDataGet(FskMediaMetaData meta, const char *id, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
FskAPI(FskErr) FskMediaMetaDataGetNext(FskMediaMetaData meta, void **metaDataIterator, const char **id, UInt32 *index, FskMediaPropertyValue value, UInt32 *flags);

// to help out media players
FskAPI(FskErr) FskMediaMetaDataGetForMediaPlayer(FskMediaMetaData meta, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);

/*
	tag mapping
*/

enum {
	kFskMetaDataFormatUnknown = 0,
	kFskMetaDataFormatQuickTime,
	kFskMetaDataFormatMP4,				// also 3GPP
	kFskMetaDataFormatiTunes,			// aka M4A
	kFskMetaDataFormatMemoryStickVideo,	// aka Playstation Portable (PSP) video files
	kFskMetaDataFormatID3v20,
	kFskMetaDataFormatID3v23,
	kFskMetaDataFormatWindowsMedia
};

FskAPI(FskErr) FskMediaMetaDataFormatTagToFskTag(UInt32 metaDataFormat, void *id, const char **fskID, UInt32 *metaDataType, UInt32 *flags);
FskAPI(FskErr) FskMediaMetaDataFskTagToFormatTag(const char *fskID, UInt32 metaDataFormat, void **id, UInt32 *metaDataType, UInt32 *flags);

FskAPI(FskErr) FskMediaParseNPT(const char *npt, double *result);
    
    
#ifdef __cplusplus
}
#endif

#endif
