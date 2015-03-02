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
#define __FSKMEDIAREADER_PRIV__

#include "QTReader.h"
#include "FskMediaReader.h"

#include "camera_jni.h"
#include "jsmn.h"

#include "FskPlatformImplementation.h"
FskInstrumentedSimpleType(CameraAndroid, CameraAndroid);
#define mlog  FskCameraAndroidPrintfMinimal
#define nlog  FskCameraAndroidPrintfNormal
#define vlog  FskCameraAndroidPrintfVerbose
#define dlog  FskCameraAndroidPrintfDebug


#include "kinoma_utilities.h"

#define OUTPUT_BITMAP               0
#define REALTIME_MODE               0

#define USE_PREVIEW_DISPLAY			0
#define kDefaultCurrentCameraID		0

#define kDefaultPreviewFPS			30
#define kDefaultPreviewWidth		640
#define kDefaultPreviewHeight		480
#define kDefaultPhotoWidth			1600
#define kDefaultPhotoHeight			1200

#define kDefaultPreviewPixelCount	kDefaultPreviewWidth*kDefaultPreviewHeight			//VGA
#define kDefaultPhotoPixelCount		kDefaultPhotoWidth*kDefaultPhotoHeight				//about 2MP

#define MAX_FRAME_TO_BUFFER			30
#define TIMESCALE_in_Micro_Sec		1000000
#define TIMESCALE_in_MiliSec		1000
#define CAM_DEFAULT_TIME_SCALE		TIMESCALE_in_MiliSec//TIMESCALE_in_Micro_Sec

#define DUMP_YUV                0
#define DUMP_YUV_PATH           "/sdcard/tmp/dump.yuv"

typedef struct SizeRecord SizeRecord;		//output callback
typedef struct SizeRecord *SizePtr;
struct SizeRecord
{
	SizePtr				next;
    FskDimensionRecord  d;
};

typedef struct CameraGenericRecord CameraGenericRecord;
typedef struct CameraGenericRecord *CameraGeneric;

typedef struct CameraGenericTrackRecord CameraGenericTrackRecord;
typedef struct CameraGenericTrackRecord *CameraGenericTrack;

struct CameraGenericTrackRecord
{
	//CameraGenericTrack			next;
	FskMediaReaderTrackRecord	reader_track;
	CameraGeneric				state;
    char                        *media_type;
    int                         pixel_format;
    int                         dimension_index;
    FskDimensionRecord          current_dimension;
    FskListMutex                dimension_list;
    int                         data_size;
    unsigned char               *data;
};

struct CameraGenericRecord
{
	int						initialized;
	int						time_scale;
	
    int                     output_bitmap;
    int                     realtime_mode;

	FskThread               mainUIThread;
    
	FskInt64				camera_start_time;
	FskInt64				media_start_time;
	
	FskMediaReader			reader;
	
    unsigned char           *camera_params_json;
    int                     camera_count;
	int						current_camera_id;
	CameraGenericTrack		video_preview_track;    //index 0
	CameraGenericTrack		photo_track;            //index 1
	CameraGenericTrack		video_track;            //index 2

	FskCamera               *mFskCamera;

	int						debug_captured_frame_count;
	int						debug_received_frame_count;
    FILE                    *debug_yuv_dump_file;

	SInt32					id;
};


static FskErr cameraGenericGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericSetLens(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericSetAutoFocusState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericGetAutoFocusArea(void *state_in, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericSetAutoFocusArea(void *state_in, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericGetJSON(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericSetJSON(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord cameraGenericProperties[] =
{
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		cameraGenericGetTimeScale,	NULL},
	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	cameraGenericGetDLNASinks,	NULL},
	{kFskMediaPropertyLens,                 kFskMediaPropertyTypeString,   		NULL,                       cameraGenericSetLens},
	{kFskMediaPropertyAutoFocusState,       kFskMediaPropertyTypeInteger,   	NULL,                       cameraGenericSetAutoFocusState},
	//{kFskMediaPropertyJSON,                 kFskMediaPropertyTypeString,        cameraGenericGetJSON,       cameraGenericSetJSON},
	{kFskMediaPropertyAutoFocusArea,        kFskMediaPropertyTypeRectangle,   	cameraGenericGetAutoFocusArea,  cameraGenericSetAutoFocusArea},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

static FskErr cameraGenericTrackGetMediaType(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericTrackGetFormat(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericTrackGetFormatInfo(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericTrackGetDimensionIndex(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericTrackSetDimensionIndex(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericTrackGetDimensionsList(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericTrackGetDimensions(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericTrackSetDimensions(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr cameraGenericTrackSetEnabled(void *state_in, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord cameraGenericVideoTrackProperties[] =
{
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		cameraGenericTrackGetMediaType,		NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		cameraGenericTrackGetFormat,		NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			cameraGenericTrackGetFormatInfo,	NULL},
	{kFskMediaPropertyDimensionsList,		kFskMediaPropertyTypeUInt32List,	cameraGenericTrackGetDimensionsList, NULL},
	//{kFskMediaPropertyDimensionIndex,		kFskMediaPropertyTypeInteger,		cameraGenericTrackGetDimensionIndex,cameraGenericTrackSetDimensionIndex},
	{kFskMediaPropertyDimensions,			kFskMediaPropertyTypeDimension,		cameraGenericTrackGetDimensions,	cameraGenericTrackSetDimensions},
	{kFskMediaPropertyEnabled,              kFskMediaPropertyTypeBoolean,   	NULL,                       cameraGenericTrackSetEnabled},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,								NULL}
};

FskMediaReaderTrackDispatchRecord gCameraGenericVideoTrack = {cameraGenericVideoTrackProperties};

static SInt32 gDisposedId = 0;
static SInt32 gCurrentId = 0;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
Boolean cameraGenericCanHandle(const char *mimeType);
FskErr cameraGenericNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
FskErr cameraGenericDispose(FskMediaReader reader, void *readerState);
FskErr cameraGenericGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
FskErr cameraGenericStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
FskErr cameraGenericStop(FskMediaReader reader, void *readerState);
FskErr cameraGenericExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
FskErr cameraGenericGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
FskErr cameraGenericSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

FskMediaReaderDispatchRecord gCameraAndroid = {cameraGenericCanHandle, cameraGenericNew, cameraGenericDispose, cameraGenericGetTrack, cameraGenericStart, cameraGenericStop, cameraGenericExtract, cameraGenericGetMetadata, cameraGenericProperties, cameraGenericSniff};

FskMediaPropertyEntry	cameraGenericGetProperties();
#ifdef __cplusplus
}
#endif /* __cplusplus */

using namespace android;

#include "FskArch.h"
#include "FskHardware.h"
#include "android_device_whitelist.c"


FskErr size_queue_in( FskListMutex size_list, int width,  int height )
{
	SizePtr item = NULL;
	FskErr   err = kFskErrNone;

	err = FskMemPtrNewClear(sizeof(SizeRecord), (FskMemPtr *)&item);
	BAIL_IF_ERR( err );

	item->d.width  = width;
	item->d.height = height;

	//dlog("appending to list, item: %x, width/height: %d/%d\n", (int)item, width, height );
	FskListMutexAppend(size_list, item);

bail:
	return err;
}


int size_queue_total( FskListMutex size_list )
{
	SizePtr item   = NULL;
    int     total  = 0;
    
    while(1)
    {
        item = (SizePtr)FskListMutexGetNext(size_list, (void *)item);
        if( item == NULL )
            break;
        
        total++;
    }
    
    return total;
}


void size_queue_check_by_index( FskListMutex size_list, int index, int *width_out,  int *height_out )
{
	SizePtr item   = NULL;
    int     width  = 0;
    int     height = 0;
    int     i;

	FskErr   err = kFskErrNone;

    for( i = 0; i < index; i++ )
        item = (SizePtr)FskListMutexGetNext(size_list, (void *)item);

	if( item )
    {
        width = item->d.width;
        height = item->d.height;
	}

    *width_out = width;
    *height_out =height;
}


void size_queue_flush( FskListMutex size_list )
{
	if( size_list == NULL )
		return;

	while(1)
	{
		SizePtr item = (SizePtr)FskListMutexRemoveFirst(size_list);;
		if( item == NULL )
		{
			//dlog("no more func_item in queue!!!\n" );
			break;
		}
		FskMemPtrDispose(item);
	}
}


static void post_frame_arrived(void *arg0, void *arg1, void *arg2, void *arg3)
{
	CameraGeneric  state  = (CameraGeneric)arg0;
    int event = (int)arg1;

    //Sometimes this callback get called after state is disposed, Android handle it's memory itself, no need to do free here
    if (gDisposedId != (SInt32)arg2)
        (state->reader->eventHandler)(state->reader, state->reader->eventHandlerRefCon, kFskEventMediaReaderDataArrived, NULL);
}


void preview_event_callback(void *refCon, int event )
{
	CameraGeneric  state  = (CameraGeneric)refCon;
    FskMediaReader reader = state->reader;

	dlog("////////////////////////////////////////////////////");
    dlog("CAMERA FRAME ARRIVING: refCon: %x, event: %d", (int)refCon, event);
    dlog("////////////////////////////////////////////////////");
    FskThreadPostCallback(state->mainUIThread, post_frame_arrived, (void*)state, (void *)event, (void *)state->id, NULL);
}

FskErr cameraParameterInitialize(CameraGeneric state)
{
	FskErr			err = kFskErrNone;

	state->mFskCamera->getPreviewSize((int *)&state->video_preview_track->current_dimension.width, (int *)&state->video_preview_track->current_dimension.height);
	state->video_preview_track->pixel_format = state->mFskCamera->getPreviewFormat();
	state->video_preview_track->data_size = state->video_preview_track->current_dimension.width * state->video_preview_track->current_dimension.height * 3 / 2;
	state->camera_count = state->mFskCamera->getCameraCount();

	state->camera_params_json =	(unsigned char *)state->mFskCamera->getParameters();
	{
		unsigned char *js = state->camera_params_json;
		jsmn_parser p;
		jsmntok_t tokens[2000];
		int this_w, this_h;
		int r;
		int i;
		char *this_string;

		mlog("got camera params, js: %s\n", js);

		jsmn_init(&p);
		r = jsmn_parse(&p, (const char*)js, strlen((const char*)js), tokens, 2000);
		mlog("jsmn_parse() called, r: %d\n", r);

		i = 0;
		this_string = (char *)"previewSize";
		JSON_FIND_STRING( tokens, r, this_string, i );
		i++;
		if( i >= r ) goto bail;
		GET_W_H( tokens, i, this_w, this_h );
		mlog("cam param ==> defult preview size %d x %d\n", this_w, this_h);
		state->video_preview_track->current_dimension.width = this_w;
		state->video_preview_track->current_dimension.height = this_h;
		mlog("cam param ==> preview_size_list\n");
		err = size_queue_in(state->video_preview_track->dimension_list, this_w, this_h );

		i = 0;
		this_string = (char *)"pictureSize";
		JSON_FIND_STRING( tokens, r, this_string, i );
		i++;
		if( i >= r ) goto bail;
		GET_W_H( tokens, i, this_w, this_h );
		mlog("cam param ==> defult photo size %d x %d\n", this_w, this_h);
		state->photo_track->current_dimension.width = this_w;
		state->photo_track->current_dimension.height = this_h;
		mlog("cam param ==> photo_size_list\n");
		err = size_queue_in(state->photo_track->dimension_list, this_w, this_h );

		i = 0;
		this_string = (char *)"pictureSizeValues";
		JSON_FIND_STRING( tokens, r, this_string, i );
		i++;
		if( i >= r ) goto bail;
		if( tokens[i].type == JSMN_ARRAY )
		{
			int array_total = tokens[i].size;
			mlog("this_string: %s, is an array with total: %d\n", this_string, array_total);
			i++;//point to height/width object
			for( ;array_total--; )
			{
				GET_W_H( tokens, i, this_w, this_h );
				mlog("cam param ==> picture size %d x %d\n", this_w, this_h);
				if( this_w != state->photo_track->current_dimension.width && this_h != state->photo_track->current_dimension.height )
				{
					mlog("cam param ==> photo_size_list\n");
					err = size_queue_in(state->photo_track->dimension_list, this_w, this_h );
					BAIL_IF_ERR( err );
				}
			}
		}

		i = 0;
		this_string = (char *)"previewSizeValues";
		JSON_FIND_STRING( tokens, r, this_string, i );
		i++;
		if( i >= r ) goto bail;
		if( tokens[i].type == JSMN_ARRAY )
		{
			int array_total = tokens[i].size;
			mlog("this_string: %s, is an array with total: %d\n", this_string, array_total);
			i++;//point to height/width object
			for( ;array_total--; )
			{
				GET_W_H( tokens, i, this_w, this_h );
				mlog("cam param ==> preview size %d x %d\n", this_w, this_h);
				if( this_w != state->video_preview_track->current_dimension.width && this_h != state->video_preview_track->current_dimension.height )
				{
					mlog("cam param ==> preview_size_list\n");
					err = size_queue_in(state->video_preview_track->dimension_list, this_w, this_h );
					BAIL_IF_ERR( err );
				}
			}
		}
	}

bail:
	dlog("out of cameraParameterInitialize\n");
	return err;
}

FskErr camInstantiate(CameraGeneric state)
{
	FskErr			err = kFskErrNone;
	
	dlog("into camInstantiate()\n");
	
	if( state->initialized )
	{
		dlog("already initialized!!!\n");
		goto bail;
	}

	state->camera_count = 0;
	state->time_scale   = CAM_DEFAULT_TIME_SCALE;

	{
		CameraGenericTrack  track;
		
		dlog( "creating cam preview track.\n");
		err = FskMemPtrNewClear(sizeof(CameraGenericTrackRecord), &track);
		BAIL_IF_ERR( err );
		
		track->state				 = state;
		track->reader_track.dispatch = &gCameraGenericVideoTrack;
		track->reader_track.state	 = track;
		track->media_type  =  FskStrDoCopy("video-preview");
        FskListMutexNew(&track->dimension_list, "video-preview-dimension-list");
        state->video_preview_track = track;

		dlog( "creating cam photo track.\n");
		err = FskMemPtrNewClear(sizeof(CameraGenericTrackRecord), &track);
		BAIL_IF_ERR( err );

		track->state				 = state;
		track->reader_track.dispatch = &gCameraGenericVideoTrack;
		track->reader_track.state	 = track;
		track->media_type  =  FskStrDoCopy("image");
        FskListMutexNew(&track->dimension_list, "photo-dimension-list");
        state->photo_track = track;
	}
	
	dlog("initializing camera service\n");;
	state->mFskCamera = FskCamera::getFskCamera();
	err = state->mFskCamera->init();
	BAIL_IF_ERR( err );

    state->mFskCamera->setpostEventProc((void*)preview_event_callback, (void *)state);

	err = cameraParameterInitialize(state);
	BAIL_IF_ERR( err );

	state->initialized = 1;

	gCurrentId++;
	state->id = gCurrentId;
	
bail:
	dlog("out of camInstantiate, initialized: %d\n", (int)state->initialized);
	return err;
}

//
//
//demuxer interface
//
//

FskMediaPropertyEntry cameraGenericGetProperties()
{
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into androidCameraGetProperties\n" );
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	
	return cameraGenericProperties;
}


Boolean cameraGenericCanHandle(const char *mimeType)
{
	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericCanHandle(), mimeType: %s\n", mimeType);
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if
	(
	 (	0 == FskStrCompareCaseInsensitive( mimeType, "video/x-kinoma-capture" ) ) ||
	 (	0 == FskStrCompareCaseInsensitive( mimeType, "video/cama") )
	)
	{
		dlog("bingo!!!\n");		
		return true;
	}
	
	return false;
}


FskErr cameraGenericSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	FskErr err = kFskErrUnknownElement;
	int offset = 0;

	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericSniff(), dataSize: %d\n", (int)dataSize);
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if (dataSize >= 4) 
	{	
		if
		( 
		   data[0] == 'c' &&
		   data[1] == 'a' &&
		   data[2] == 'm' &&
		   data[3] == 'a' 
		)
		{
			dlog("found cama, setting video/cama\n");
			*mime = FskStrDoCopy("x-kinoma-capture");
			err = kFskErrNone;
		}
	}

bail:
	dlog("out of cameraGenericSniff(), err: %d\n", (int)err);
	
	return err;
}


FskErr cameraGenericNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	CameraGeneric state = NULL;

	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericNew()\n");;
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	
	err = FskMemPtrNewClear(sizeof(CameraGenericRecord), &state);
	BAIL_IF_ERR( err );
	
	*readerState = state;			// must be set before anything that might issue a callback
	state->reader = reader;
	state->media_start_time = 0;
	
    state->output_bitmap = OUTPUT_BITMAP;
    state->realtime_mode = REALTIME_MODE;
    
	dlog("setting state: kFskMediaPlayerStateInstantiating\n");
	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);

    state->mainUIThread = FskThreadGetCurrent();
    
	dlog("calling cameraGenericNew()\n");
	err = camInstantiate(state);
	if( err == kFskErrNeedMoreTime )
		err = kFskErrNone;

bail:
	if( (kFskErrNone != err) && (state != NULL ) )
	{
		dlog("initialization failing, dispoing!!!\n");
		cameraGenericDispose(reader, state);
		(state->reader->doSetState)(state->reader, kFskMediaPlayerStateClosed);
		*readerState = NULL;
	}
	else
	{
		*readerState = state;			// must be set before anything that might issue a callback
		dlog("setting state: kFskMediaPlayerStateStopped\n");;
		(state->reader->doSetState)(state->reader, kFskMediaPlayerStateStopped);
	}
	
	dlog("out of cameraGenericNew()\n");;

	return err;
}


FskErr cameraGenericDispose(FskMediaReader reader, void *readerState)
{
	CameraGeneric state = (CameraGeneric)readerState;

	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericDispose()\n");;
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if( state != NULL )
	{
        CameraGenericTrack track;
		cameraGenericStop(reader, state);

        track = state->video_preview_track;
		if( track )
		{
			dlog("disposing video_preview_track\n");
            if( track->dimension_list != NULL )
            {
                dlog( "calling frame_queue_flush\n" );
                size_queue_flush( track->dimension_list );
                FskListMutexDispose(track->dimension_list);
            }
			FskMemPtrDispose(track);
		}

        track = state->photo_track;
		if( track )
		{
			dlog("disposing photo_track\n");
            if( track->dimension_list != NULL )
            {
                dlog( "calling frame_queue_flush\n" );
                size_queue_flush( track->dimension_list );
                FskListMutexDispose(track->dimension_list);
            }
			FskMemPtrDispose(track);
		}

        if( state->debug_yuv_dump_file != NULL )
            fclose( state->debug_yuv_dump_file );
        
		dlog("disposing state\n");
		gDisposedId = state->id;
		FskMemPtrDispose(state);
	}

	dlog("out of cameraGenericDispose()\n");
	return kFskErrNone;
}

FskErr cameraGenericGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track_out)
{
	CameraGeneric	  state  = (CameraGeneric)readerState;
	CameraGenericTrack track  = NULL;

	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericGetTrack(): index: %d\n", index);
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if( !state->initialized )
		return kFskErrMemFull;

    if( index == 0 )
        track = state->video_preview_track;
    else if( index == 1 )
        track = state->photo_track;
    else if( index == 2 )
        track = state->video_track;

	if( track )
	{
		dlog("got a track\n");;
		*track_out = &track->reader_track;
		return kFskErrNone;
	}
	else
	{
		dlog("track not found\n");;		
		return kFskErrNotFound;
	}
}


FskErr cameraGenericStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	CameraGeneric	state = (CameraGeneric)readerState;
	FskErr			err = kFskErrNone;
	
	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericStart(), startTime/endTime: %d/%d\n", (int)startTime, (int)endTime);
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	
	if( startTime != NULL )
		state->media_start_time  = (FskInt64)*startTime;
	dlog("state->media_start_time: %d\n", (int)state->media_start_time);
	
	if( !state->initialized )
	{
		dlog("camera not initialized!!!\n");
		err =  kFskErrBadState;
		goto bail;
	}
	
	state->debug_captured_frame_count	= 0;
	state->debug_received_frame_count	= 0;
	state->camera_start_time	= 0;

	err = state->mFskCamera->startPreview();
	BAIL_IF_ERR( err );

bail:
	dlog("out of cameraGenericStart, err: %d\n", err);
	return err;
}


FskErr cameraGenericStop(FskMediaReader reader, void *readerState)
{
	CameraGeneric state = (CameraGeneric)readerState;
	status_t status;
	
	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericStop()\n");;
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if (state->mFskCamera)
	{
		state->mFskCamera->stopPreview();
		state->mFskCamera->finish();
	}

	
	dlog("out of cameraGenericStop\n");
	return kFskErrNone;		//@@
}


FskErr cameraGenericExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *trackOut, UInt32 *infoCountOut, FskMediaReaderSampleInfo *infoOut, unsigned char **data)
{
	CameraGeneric			 state = (CameraGeneric)readerState;
	CameraGenericTrack		 track	= NULL;
	int						 size = 0;
	FskInt64				 time = 0;
	int						 fps  = 0;
	unsigned char			 *frame = NULL;
	FskMediaReaderSampleInfo info = NULL;
	FskErr					 err = kFskErrNone;

	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericExtract()\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	*infoCountOut	= 0;
	*infoOut		= NULL;
	*trackOut		= NULL;
	*data			= NULL;
    
	if( !state->initialized )
		goto bail;

    track = state->photo_track;
    if( track != NULL )
    {
        state->mFskCamera->getPhotoData( &track->data, &track->data_size );
        dlog("check if there is any photo data\n");
        if( state->photo_track->data_size!= 0 )
        {
            dlog("allocating a new sample record for photo\n");
            err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
            BAIL_IF_ERR( err );

            info->flags				= kFskImageFrameImmediate;
            info->samples			= 1;
            info->decodeTime		= 0;
            info->compositionTime	= -1;

            *infoCountOut			= 1;
            *infoOut				= info;
            *trackOut				= &track->reader_track;

            info->sampleSize        = track->data_size;
            *data                   = track->data;

            mlog("done packing a new sample record for photo, photo_data_size: %d\n", track->data_size);
#if 0
            {
                char jpg_path[] = "/sdcard/kinoma_camera_ouput.jpg";
                FILE *f = NULL;
                f = fopen(jpg_path, "wb");
                if( f == NULL )
                {
                    dlog( "can't create kinoma camera output jpg file!!! \n");
                }
                else
                {
                    fwrite(photo_data, 1, photo_data_size, f);
                    fclose(f);
                }
            }
#endif
            track->data       = NULL;
            track->data_size  = 0;

            goto bail;
        }
    }


    track = state->video_preview_track;
    if( track == NULL )
        goto bail;
    {
	    FskTimeRecord now;
	    FskTimeGetNow(&now);

	    //Get the Camera start time to caculate the relative time, which matchs the frame timestamps
	   //***bnie  FskTimeSub(state->mFskCamera->getStartTime(), &now);
		FskInt64 etime = (FskInt64)FskTimeInMS(&now);
        dlog("==============================================>extracting at time: %lld\n", etime);
    }

    if( state->realtime_mode )
        state->mFskCamera->pullLatestPreviewFrame(&frame, &size, &time);
    else
        state->mFskCamera->pullPreviewFrame(&frame, &size, &time);
    
    if( frame == NULL || size == 0 )
    {
        dlog("no camera frame ready yet, setting kFskErrNeedMoreTime!!!\n");
        err = kFskErrNeedMoreTime;
    }
    BAIL_IF_ERR( err );
    dlog("==============================================>extracted time: %lld\n", time);
	
	if( state->camera_start_time == 0 )
		state->camera_start_time = time;
	
	time -= state->camera_start_time;
	state->debug_received_frame_count++;
	if( time > 0 )//use absolut time to calculate fps
		fps = state->debug_received_frame_count*state->time_scale/time;

	//get time based on start time to make player happy
	time += state->media_start_time;
	dlog("got a frame with offsetted time: %lld\n", time);
	dlog("state->debug_received_frame_count: %d\n", state->debug_received_frame_count);
	
	//buf_printf(0, frame, state->preview_width, state->preview_height, 0, 50, "player received frame: %d, fps: %d\n", state->debug_received_frame_count, fps);
	
	
	dlog("allocating a new sample record\n");
	err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
	BAIL_IF_ERR( err );

	info->flags				= kFskImageFrameTypeSync;
	info->samples			= 1;
	info->decodeTime		= (FskInt64)time;
	info->compositionTime	= -1;

	*infoCountOut			= 1;
	*infoOut				= info;
	*trackOut				= &track->reader_track;
    
    if( size !=  track->data_size )
    {
        dlog("size/preview_frame_size: %d/%d", size, track->data_size );
        // size = track->data_size;
        track->data_size = size;
    }
    
    if( !state->output_bitmap )
    {
        info->sampleSize	= size;
        *data				= frame;
    }
    else
    {
		int width   = track->current_dimension.width;
		int height  = track->current_dimension.height;
		int format  = track->pixel_format;
		int bitDepth = 8;
		int rowbytes = width * bitDepth / 8;
        FskBitmap	bits = NULL;

        err = FskBitmapNewWrapper( width, height, (FskBitmapFormatEnum)format, bitDepth, (void *)frame, rowbytes, &bits);
        *data	= (unsigned char *)bits;
        dlog("use FskBitmapNewWrapper() to create a new bitmap: err:%d, width: %d, height: %d, format: %d, bitDepth: %d, frame: %x, rowbytes: %d, bits: %x",
                err, width, height, format, bitDepth, (int)frame, rowbytes, (int)bits);

        info->sampleSize = sizeof(FskBitmap);
	
        if( DUMP_YUV )
        {
            if( state->debug_yuv_dump_file == NULL )
            {
                unsigned char *d = frame;
                state->debug_yuv_dump_file = fopen( DUMP_YUV_PATH, "wb" );

                switch(format)
                {
                    case kFskBitmapFormatYUV420:
                        *d++ = 'y';
                        *d++ = 'u';
                        *d++ = 'v';
                        *d++ = '4';
                        *d++ = '2';
                        *d++ = '0';
                        break;
                    case kFskBitmapFormatUYVY:
                        *d++ = '2';
                        *d++ = 'v';
                        *d++ = 'u';
                        *d++ = 'y';
                        *d++ = ' ';
                        *d++ = ' ';
                        break;
                    case kFskBitmapFormatYUV420spuv:
                        *d++ = 's';
                        *d++ = 'p';
                        *d++ = 'u';
                        *d++ = 'v';
                        *d++ = ' ';
                        *d++ = ' ';
                        break;
                    case kFskBitmapFormatYUV420spvu:
                        *d++ = 's';
                        *d++ = 'p';
                        *d++ = 'v';
                        *d++ = 'u';
                        *d++ = ' ';
                        *d++ = ' ';
                        break;
                }

                *d++ = 0;                   //fps
                *d++ = 30;
                *d++ = ( width>>8)&0xff;    //width
                *d++ = ( width>>0)&0xff;
                *d++ = (height>>8)&0xff;    //height
                *d++ = (height>>0)&0xff;
            }
            
            if( state->debug_yuv_dump_file != NULL )
            {
                fwrite( frame,  1, size, state->debug_yuv_dump_file );
                fflush( state->debug_yuv_dump_file );
            }
        }
    
    }
    
    dlog("outputting 1 video frame: size: %d, decode_time: %d, sample_flag: %d\n",
			(int)info->sampleSize, (int)info->decodeTime, (int)info->flags);


bail:
	if( *infoOut == NULL && err != kFskErrEndOfFile )
		err = kFskErrNeedMoreTime;

	return err;
}


FskErr cameraGenericGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericGetMetadata()\n");;
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	dlog("out of cameraGenericGetMetadata(), metaData: kFskErrUnknownElement\n");;
	return kFskErrUnknownElement;
}


static FskErr cameraGenericGetTimeScale(void *state_in, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGeneric state = (CameraGeneric)state_in;
	FskErr			err = kFskErrNone;
		 
	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericGetTimeScale()\n");;
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	
	if( !state->initialized )
	{
		dlog("camera not initialized!!!\n");;
		err = kFskErrBadState;
		goto bail;
	}
	
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->time_scale;
	
bail:	
	dlog("out of cameraGenericGetTimeScale(), state->time_scale: %d\n", (int)state->time_scale);
	return err;
}


static FskErr cameraGenericGetDLNASinks(void *state_in, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "video/yuv420\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

//
//
//track interface
//
//

static FskErr cameraGenericTrackGetMediaType(void *state_in, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGenericTrack track = (CameraGenericTrack)state_in;
	CameraGeneric		state  = track->state;
	FskErr				err = kFskErrNone;

	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericTrackGetMediaType()\n");;
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if( !state->initialized )
	{
		dlog("camera not initialized, bailing!!!\n");;
		err = kFskErrBadState;
		goto bail;
	}
	
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy(track->media_type);

bail:	
	dlog("out of cameraGenericTrackGetMediaType(), mediaType: %s\n", property->value.str);
	return err;
}

static FskErr cameraGenericTrackGetFormat(void *state_in, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGenericTrack track = (CameraGenericTrack)state_in;
	CameraGeneric		state  = track->state;
	FskErr				err = kFskErrNone;

	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericTrackGetFormat()\n");;
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if( !state->initialized )
	{
		dlog("camera not initialized, bailing!!!\n");;
		err = kFskErrBadState;
		goto bail;
	}
	
	property->type = kFskMediaPropertyTypeString;

    if(	0 == FskStrCompareCaseInsensitive("image", track->media_type) )
    {
        dlog("returning image/jpeg\n");;
        property->value.str = FskStrDoCopy("image/jpeg");
    }
    else if(	0 == FskStrCompareCaseInsensitive("video-preview", track->media_type) )
    {
        dlog("checking track->pixel_format: %d\n", track->pixel_format);

        if( state->output_bitmap )
        {
            dlog("returning x-video-codec/bitmap\n");;
            property->value.str = FskStrDoCopy("x-video-codec/bitmap");
        }
        else if( track->pixel_format == kFskBitmapFormatYUV420 )
        {
            dlog("returning x-video-codec/yuv420\n");;
            property->value.str = FskStrDoCopy("x-video-codec/yuv420");
        }
        else if( track->pixel_format == kFskBitmapFormatUYVY )
        {
            dlog("returning x-video-codec/2vuy\n");;
            property->value.str = FskStrDoCopy("x-video-codec/2vuy");
        }
        else if( track->pixel_format == kFskBitmapFormatYUV420spvu )
        {
            dlog("returning x-video-codec/yuv420spvu\n");;
            property->value.str = FskStrDoCopy("x-video-codec/yuv420sp");
        }
        else if( track->pixel_format == kFskBitmapFormatYUV420spuv )
        {
            dlog("returning x-video-codec/yuv420spuv\n");;
            property->value.str = FskStrDoCopy("x-video-codec/yuv420sp");
        }
    }

bail:
	dlog("out of cameraGenericTrackGetFormat(), camera_preview_pixel_format: %s\n", property->value.str );
	return err;
}


static FskErr cameraGenericTrackGetFormatInfo(void *state_in, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGenericTrack track = (CameraGenericTrack)state_in;
	CameraGeneric	 state = track->state;
	QTImageDescription	desc = NULL;
    int				descSize = 0;
	FskErr			err   = kFskErrNone;

	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericTrackGetFormatInfo()\n");;
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if( !state->initialized )
	{
		dlog("camera not initialized, bailing!!!\n");;
		err = kFskErrBadState;
		goto bail;
	}

    if(	0 == FskStrCompareCaseInsensitive("video-preview", track->media_type) )
    {
        int desc_ext_size = 12; //size+4cc+value

        descSize  = sizeof(QTImageDescriptionRecord) + desc_ext_size;
        err = FskMemPtrNewClear(descSize, (FskMemPtr *)&desc);
        BAIL_IF_ERR( err );

        desc->cType	 = track->pixel_format;
        dlog("set  desc->cType: %d\n", (int)desc->cType);
        desc->width  = track->current_dimension.width;
        desc->height = track->current_dimension.height;

        if(0)
        {//set image description extension to tell decoder sample data can be directly wrapped as bitmap
            UInt32          const_bitmap = 'btmp';
            int             value = 1;
            unsigned char   *ext = (unsigned char *)desc;
            ext += sizeof(QTImageDescriptionRecord);
            FskMemCopy(ext+0, &desc_ext_size, 4);
            FskMemCopy(ext+4, &const_bitmap, 4);
            FskMemCopy(ext+8, &value, 4);
            desc->idSize = descSize;
        }
    }

	property->type = kFskMediaPropertyTypeData;
	property->value.data.dataSize = descSize;
	property->value.data.data = desc;

bail:
	dlog("out of cameraGenericTrackGetFormatInfo(), desc\n" );
	return err;
}


static FskErr cameraGenericTrackGetDimensionIndex(void *state_in, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGenericTrack	track = (CameraGenericTrack)state_in;
	CameraGeneric		state = track->state;
	FskErr				err   = kFskErrNone;

	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericTrackGetDimensionIndex()\n");;
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if( !state->initialized )
	{
		dlog("camera not initialized, bailing!!!\n");;
		err = kFskErrBadState;
		goto bail;
	}
	
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->dimension_index;

bail:
	dlog("out of cameraGenericTrackGetDimensionIndex(), index: %d\n", (int)track->dimension_index );
	return err;
}


static FskErr cameraGenericTrackSetDimensionIndex(void *state_in, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGenericTrack	track = (CameraGenericTrack)state_in;
	CameraGeneric		state = track->state;
    int                 dimension_count;
    int                 dimension_index;
	FskErr				err   = kFskErrNone;

	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericTrackSetDimensionIndex()\n");;
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if( !state->initialized )
	{
		dlog("camera not initialized, bailing!!!\n");;
		err = kFskErrBadState;
		goto bail;
	}

    track->dimension_index = property->value.integer;

bail:
	mlog("out of cameraGenericTrackSetDimensionIndex(), index: %d\n", (int)track->dimension_index );
	return err;
}


static FskErr cameraGenericTrackGetDimensionsList(void *state_in, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGenericTrack	track = (CameraGenericTrack)state_in;
	CameraGeneric		state = track->state;
    int                 width = 0;
    int                 height = 0;
	FskErr				err   = kFskErrNone;
    int                 idx;
    int                 total = 0;
    FskDimensionRecord  *list;
    
    
	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	mlog("into cameraGenericTrackGetDimensionsList(), track: %x, track->dimension_index: %d\n", (int)track, track->dimension_index);
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
    
	if( !state->initialized )
	{
		dlog("camera not initialized, bailing!!!\n");;
		err = kFskErrBadState;
		goto bail;
	}
    
    total = size_queue_total( track->dimension_list );
    err = FskMemPtrNewClear(sizeof(FskDimensionRecord)*total, (void **)&list);
    BAIL_IF_ERR( err );
    
    for( idx = 1; idx < total+1; idx++ )
    {
        int width  = 0;
        int height = 0;
        size_queue_check_by_index( track->dimension_list, idx, &width,  &height );
        list[idx-1].width = width;
        list[idx-1].height = height;
    }
    
	property->type = kFskMediaPropertyTypeUInt32List;
    property->value.integers.count   = total * 2;
	property->value.integers.integer = (UInt32 *)list;
    
bail:
    mlog("out of cameraGenericTrackGetDimensionsList(), total: %d\n", (int)total );
    return err;
}


static FskErr cameraGenericTrackGetDimensions(void *state_in, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGenericTrack	track = (CameraGenericTrack)state_in;
	CameraGeneric		state = track->state;
    int                 width = 0;
    int                 height = 0;
	FskErr				err   = kFskErrNone;

	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	mlog("into cameraGenericTrackGetDimensions(), track: %x, track->dimension_index: %d\n", (int)track, track->dimension_index);
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if( !state->initialized )
	{
		dlog("camera not initialized, bailing!!!\n");;
		err = kFskErrBadState;
		goto bail;
	}

	property->type = kFskMediaPropertyTypeDimension;

    if( track->dimension_index == 0 )
    {
		dlog("checking current dimension\n");;
        width  = track->current_dimension.width;
        height = track->current_dimension.height;
    }
    else
    {
        int dimension_count = FskListMutexCount(track->dimension_list);
        dlog("dimension_count: %d\n", dimension_count);
        if( track->dimension_index > 0 && track->dimension_index <= dimension_count )
        {
            dlog("checking dimension with index: %d\n", track->dimension_index);
            size_queue_check_by_index( track->dimension_list, track->dimension_index, &width,  &height );
        }
    }

bail:
    property->value.dimension.width  = width;
    property->value.dimension.height = height;

    mlog("out of cameraGenericTrackGetDimensions(), width: %d, height: %d\n", (int)width, (int)height );
	return err;
}

static FskErr cameraGenericTrackSetDimensions(void *state_in, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGenericTrack	track = (CameraGenericTrack)state_in;
	CameraGeneric		state = track->state;
	FskErr				err   = kFskErrNone;

	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	mlog("into cameraGenericTrackSetDimensions(), track: %x, track->dimension_index: %d\n", (int)track, track->dimension_index);
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if( !state->initialized )
	{
		dlog("camera not initialized, bailing!!!\n");;
		err = kFskErrBadState;
		goto bail;
	}

	if (track->current_dimension.width == property->value.dimension.width &&
		track->current_dimension.height == property->value.dimension.height)
	{
		dlog("Setting the same dimension, skip!\n");
		goto bail;
	}

	char tmp[64];
	if(	0 == FskStrCompareCaseInsensitive("image", track->media_type) )
	{
		snprintf(tmp, sizeof(tmp), "{\"pictureSize\":{\"width\"=\"%d\",\"height\"=\"%d\"}}", property->value.dimension.width, property->value.dimension.height);
	}
	else if(	0 == FskStrCompareCaseInsensitive("video-preview", track->media_type) )
	{
		snprintf(tmp, sizeof(tmp), "{\"previewSize\":{\"width\"=\"%d\",\"height\"=\"%d\"}}", property->value.dimension.width, property->value.dimension.height);
	}

	track->current_dimension = property->value.dimension;
	state->mFskCamera->setParameters(tmp);

bail:
	mlog("out of cameraGenericTrackSetDimensions(), width: %d, height: %d\n", (int)track->current_dimension.width, (int)track->current_dimension.height );
	return err;
}


static FskErr cameraGenericTrackSetEnabled(void *state_in, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGenericTrack	track = (CameraGenericTrack)state_in;
	CameraGeneric		state = track->state;
	FskErr		  err = kFskErrNone;
	Boolean		  enabled	= property->value.b;

	mlog("\n\n\n");
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	mlog("into cameraGenericSetEnabled()\n");;
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if( !state->initialized )
	{
		dlog("camera not initialized, bailing!!!\n");;
		err = kFskErrBadState;
		goto bail;
	}

	if( enabled )
    {
		mlog("taking a picture!!!\n");;
		state->mFskCamera->takePicture();
	}

bail:
	mlog("out of cameraGenericSetEnabled() : %d\n", enabled);
	return err;
}

/*
FskErr cameraGenericGetCameraCount(void *state_in, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGeneric state = (CameraGeneric)state_in;
	FskErr			err = kFskErrNone;

    dlog("\n\n\n");
    dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
    dlog("into cameraGenericGetCameraCount()\n");;
    dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

    if( !state->initialized )
    {
        dlog("camera not initialized!!!\n");;
        err = kFskErrBadState;
        goto bail;
    }

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->camera_count;

bail:
	dlog("out of cameraGenericGetCameraCount(), camera_count: %d\n", (int)state->camera_count);
	return err;
}
*/


static FskErr cameraGenericGetJSON(void *state_in, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGeneric state = (CameraGeneric)state_in;
	FskErr			err = kFskErrNone;

	mlog("\n\n\n");
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	mlog("into cameraGenericGetCameraParameter()\n");;
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

    if( !state->initialized )
    {
        dlog("camera not initialized, bailing!!!\n");;
        err = kFskErrBadState;
        goto bail;
    }

	property->type = kFskMediaPropertyTypeString;
	property->value.str = state->mFskCamera->getParameters();

bail:
	mlog("out of cameraGenericGetCameraParameters(): %s\n", property->value.str);
	return err;
}

static FskErr cameraGenericSetJSON(void *state_in, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGeneric state = (CameraGeneric)state_in;
	FskErr		  err = kFskErrNone;
	char		  *s	= property->value.str;

	mlog("\n\n\n");
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	mlog("into cameraGenericSetJSON(), camera parameters: %s\n", s);;
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

    if( !state->initialized )
    {
        dlog("camera not initialized, bailing!!!\n");;
        err = kFskErrBadState;
        goto bail;
    }

	state->mFskCamera->setParameters(s);


bail:
	mlog("out of cameraGenericSetJSON() : %s\n", s);
	return err;
}


static FskErr cameraGenericSetLens(void *state_in, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGeneric state = (CameraGeneric)state_in;
	FskErr		  err = kFskErrNone;
	char		  *s	= property->value.str;
    
	mlog("\n\n\n");
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	mlog("into cameraGenericSetLens(), camera name: %s\n", s);;
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

    if( !state->initialized )
    {
        dlog("camera not initialized, bailing!!!\n");;
        err = kFskErrBadState;
        goto bail;
    }

	if( strcmp( s, "back" ) == 0 )
    {
		mlog("choosing back camera!!!\n");;
		state->mFskCamera->chooseCamera(0);
	}
	else if( strcmp( s, "front" ) == 0 )
    {
		mlog("choosing front camera!!!\n");;
		state->mFskCamera->chooseCamera(1);
	}

	//Clear the old state
	size_queue_flush(state->video_preview_track->dimension_list);
	size_queue_flush(state->photo_track->dimension_list);

	err = cameraParameterInitialize(state);
	BAIL_IF_ERR(err);

bail:
	mlog("out of cameraGenericSetLens() : %s\n", s);
	return err;
}


static FskErr cameraGenericSetAutoFocusState(void *state_in, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGeneric state = (CameraGeneric)state_in;
	FskErr		  err   = kFskErrNone;
	int		  af_state	= property->value.integer;
    
    mlog("into cameraGenericSetAutoFocusState\n");
    
    if( !state->initialized )
    {
        dlog("camera not initialized, bailing!!!\n");;
        err = kFskErrBadState;
        goto bail;
    }

	if( af_state == 1 )
    {
		mlog("autoFocus!!!\n");;
		state->mFskCamera->autoFocus();
	}

bail:
	mlog("out of cameraGenericSetAutoFocusState() : %d\n", af_state);
	return err;

}


static FskErr cameraGenericSetAutoFocusArea(void *state_in, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGeneric state = (CameraGeneric)state_in;
	FskErr		  err   = kFskErrNone;
	FskRectangleRecord	area = property->value.rect;

    mlog("into cameraGenericSetAutoFocusArea\n");

	if( !state->initialized )
	{
		dlog("camera not initialized, bailing!!!\n");;
		err = kFskErrBadState;
		goto bail;
	}

	state->mFskCamera->setAutoFocusArea(&area);

bail:
	mlog("out of cameraGenericSetAutoFocusArea()\n");
	return err;

}


static FskErr cameraGenericGetAutoFocusArea(void *state_in, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	CameraGeneric state = (CameraGeneric)state_in;
	FskRectangleRecord	area;
	FskErr			err = kFskErrNone;

	mlog("\n\n\n");
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	mlog("into cameraGenericGetAutoFocusArea()\n");;
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if( !state->initialized )
	{
		dlog("camera not initialized, bailing!!!\n");;
		err = kFskErrBadState;
		goto bail;
	}

	state->mFskCamera->getAutoFocusArea(&area);
	property->type = kFskMediaPropertyTypeRectangle;
	property->value.rect = area;

bail:
	mlog("out of cameraGenericGetAutoFocusArea()\n");
	return err;
}
