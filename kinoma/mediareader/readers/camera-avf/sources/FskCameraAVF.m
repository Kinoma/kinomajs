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
#include "FskFiles.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CVPixelBuffer.h>
#import <ImageIO/ImageIO.h>


#include "FskPlatformImplementation.h"
FskInstrumentedSimpleType(CameraAVF, CameraAVF);
#define mlog  FskCameraAVFPrintfMinimal
#define nlog  FskCameraAVFPrintfNormal
#define vlog  FskCameraAVFPrintfVerbose
#define dlog  FskCameraAVFPrintfDebug

#include "kinoma_utilities.h"


#define OUTPUT_BITMAP               0
#define REALTIME_MODE               1

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

@interface CaptureDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
    void *refCon_;
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection;

@end


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

    AVCaptureSession        *session;
    AVCaptureDevice         *device;
    AVCaptureDeviceInput    *captureInput;
    AVCaptureStillImageOutput *captureStillOutput;
    AVCaptureVideoDataOutput *captureVideoOutput;
    CaptureDelegate         *captureDelegate;

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
	{kFskMediaPropertyAutoFocusArea,        kFskMediaPropertyTypeRectangle,   	cameraGenericGetAutoFocusArea,  cameraGenericSetAutoFocusArea},
	//{kFskMediaPropertyJSON,                 kFskMediaPropertyTypeString,        cameraGenericGetJSON,       cameraGenericSetJSON},
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

static FskMediaReaderTrackDispatchRecord gCameraGenericVideoTrack = {cameraGenericVideoTrackProperties};

static Boolean cameraGenericCanHandle(const char *mimeType);
static FskErr cameraGenericNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr cameraGenericDispose(FskMediaReader reader, void *readerState);
static FskErr cameraGenericGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr cameraGenericStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr cameraGenericStop(FskMediaReader reader, void *readerState);
static FskErr cameraGenericExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr cameraGenericGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
static FskErr cameraGenericSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

FskMediaReaderDispatchRecord gCameraAVF = {cameraGenericCanHandle, cameraGenericNew, cameraGenericDispose, cameraGenericGetTrack, cameraGenericStart, cameraGenericStop, cameraGenericExtract, cameraGenericGetMetadata, cameraGenericProperties, cameraGenericSniff};

static SInt32 gDisposedId = 0;
static SInt32 gCurrentId = 0;

static void preview_event_callback(void *refCon, int event, unsigned char *data);

@implementation CaptureDelegate

- (id)initWithRefcon:refCon
{
    [super init];
    dlog( "into CaptureDelegate init, %x", (int)refCon );
    refCon_ = refCon;
    //exit(-1);
    return self;
}

-(void)dealloc
{
    dlog( "into CaptureDelegate dealloc" );
    [super dealloc];
}


-(void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
	CameraGeneric state = (CameraGeneric)refCon_;
    CameraGenericTrack track = state->video_preview_track;

	FskErr   err = kFskErrNone;

    dlog( "into captureOutput()!!!" );
    // Create a UIImage from the sample buffer data
    CMTime presentationTime = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    CMTime decodeTime       = CMSampleBufferGetDecodeTimeStamp(sampleBuffer);
    CVPixelBufferRef pixBuf = CMSampleBufferGetImageBuffer(sampleBuffer);

    CVPixelBufferLockBaseAddress(pixBuf,0);
    //CVBufferRetain(pixBuf);

    size_t  plane_count     = CVPixelBufferGetPlaneCount(pixBuf);
    Boolean is_planar		= CVPixelBufferIsPlanar(pixBuf);
    OSType src_pix_format	= CVPixelBufferGetPixelFormatType(pixBuf);
    unsigned char *pix_data = (unsigned char *)CVPixelBufferGetBaseAddress(pixBuf);
    int		width		= CVPixelBufferGetWidth(pixBuf);
    int		height		= CVPixelBufferGetHeight(pixBuf);
    int		strd			= CVPixelBufferGetBytesPerRow(pixBuf);
    int		frame_size	= CVPixelBufferGetDataSize(pixBuf);

    dlog( "width: %d", width );
    dlog( "height: %d", height );
    dlog( "strd: %d", strd );
    dlog( "frame_size: %d", frame_size );
    dlog( "plane_count: %d", plane_count );
    dlog( "is_planar: %d", is_planar );
    dlog( "src_pix_format: %d", src_pix_format );
    dlog( "pix_data: %x", (int)pix_data );

    //Sometimes dimension is changed to photo track dimension, skip such frames
    if (width != track->current_dimension.width || height != track->current_dimension.height) {
        dlog("Preview frame size does not match the track dimension, bail!");
        goto bail;
    }

    if( (track->data_size) != frame_size && (track->data != NULL) )
    {
        FskMemPtrDisposeAt(&track->data);
        track->data      = NULL;
        track->data_size = 0;
    }

    if (is_planar) {
        if( track->data == NULL ) {
            track->data = FskMemPtrAlloc(frame_size);
        }
 
        unsigned char *dst = track->data;
        unsigned char *src;
        
        for (int i=0; i<plane_count; i++) {
            src = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixBuf, i);
            int size = CVPixelBufferGetBytesPerRowOfPlane(pixBuf, i) * CVPixelBufferGetHeightOfPlane(pixBuf, i);
            
            memcpy(dst, src, size);
            dst += size;
        }
    }
    else {
        if( track->data == NULL )
            err = FskMemPtrNewFromData( frame_size, pix_data, &track->data );
        else
            memcpy( track->data, pix_data, frame_size );
    }

    if( err == kFskErrNone )
        track->data_size = frame_size;

    preview_event_callback((void *)state, 666, track->data);

bail:
    CVPixelBufferUnlockBaseAddress(pixBuf,0);

    dlog( "out of captureOutput, track->data_size: %d: track->data: %x", track->data_size, (int)track->data );
}

@end


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

FskErr size_queue_in_check_duplication( FskListMutex size_list, int width, int height )
{
    SizePtr item   = NULL;
    FskErr   err = kFskErrNone;

    while(1)
    {
        item = FskListMutexGetNext(size_list, (void *)item);

        if( item == NULL ) {
            err = size_queue_in(size_list, width, height);
            break;
        }
        else {
            if (item->d.width == width && item->d.height == height)
                break;
        }
    }
    
    return err;
}


int size_queue_total( FskListMutex size_list )
{
	SizePtr item   = NULL;
    int     total  = 0;

    while(1)
    {
        item = FskListMutexGetNext(size_list, (void *)item);
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
        item = FskListMutexGetNext(size_list, (void *)item);

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

    //Sometimes this callback get called after state is disposed
    if (gDisposedId != (SInt32)arg2)
        (state->reader->eventHandler)(state->reader, state->reader->eventHandlerRefCon, kFskEventMediaReaderDataArrived, NULL);
}


static void preview_event_callback(void *refCon, int event, unsigned char *data)
{
	CameraGeneric  state  = (CameraGeneric)refCon;
    FskMediaReader reader = state->reader;

	dlog("////////////////////////////////////////////////////");
    dlog("CAMERA FRAME ARRIVING: refCon: %x, event: %d, id: %d", (int)refCon, event, state->id);
    dlog("////////////////////////////////////////////////////");
    FskThreadPostCallback(state->mainUIThread, post_frame_arrived, (void*)state, (void *)event, state->id, NULL);
}

void cameraDeviceSetActiveFormat(CameraGeneric state, int width, int height) {
    
    [state->device lockForConfiguration:nil];

    for (AVCaptureDeviceFormat *f in state->device.formats) {
        CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(f.formatDescription);
        if ((dimensions.width == width) && (dimensions.height == height)) {
            mlog("Setting Active Formats: %s", [f.description UTF8String]);
            [state->device setActiveFormat:f];
            break;
        }
    }

    [state->device unlockForConfiguration];
}

FskErr cameraDeviceInitialize(CameraGeneric state)
{
    FskErr			err = kFskErrNone;

    NSNumber* pixel_format = [state->captureVideoOutput.videoSettings valueForKey:(id)kCVPixelBufferPixelFormatTypeKey];
    
    if (pixel_format.intValue == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange) {
        state->video_preview_track->pixel_format = kFskBitmapFormatYUV420spuv;
    }
    else if (pixel_format.intValue == kCVPixelFormatType_422YpCbCr8) {
        state->video_preview_track->pixel_format = kFskBitmapFormatUYVY;
    }

    //Clear the old state
    size_queue_flush(state->video_preview_track->dimension_list);
    size_queue_flush(state->photo_track->dimension_list);
    state->video_preview_track->current_dimension.width = 0;
    state->video_preview_track->current_dimension.height = 0;

    NSArray *formats = [state->device formats];
    for (AVCaptureDeviceFormat *f in formats)
    {
        mlog("Formats: %s", [f.description UTF8String]);
        CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(f.formatDescription);
        int pixel_count = dimensions.width * dimensions.height;
        int existing_pixel_count = state->video_preview_track->current_dimension.width * state->video_preview_track->current_dimension.height;
        
        mlog("OBJECTIVEC: got a dimension: %d/%d", dimensions.width, dimensions.height);
        
        if( pixel_count > existing_pixel_count && pixel_count <= kDefaultPreviewPixelCount )
        {
            state->video_preview_track->current_dimension.width = dimensions.width;
            state->video_preview_track->current_dimension.height = dimensions.height;
        }

        //Now the devices seems have the same dimensions for preview and photo
        err = size_queue_in_check_duplication(state->video_preview_track->dimension_list, dimensions.width, dimensions.height);
        err = size_queue_in_check_duplication(state->photo_track->dimension_list, dimensions.width, dimensions.height);
    }

    if( state->video_preview_track->current_dimension.width == 0 || state->video_preview_track->current_dimension.height ==  0)
        size_queue_check_by_index( state->video_preview_track->dimension_list, 1, &state->video_preview_track->current_dimension.width,  &state->video_preview_track->current_dimension.height );

    cameraDeviceSetActiveFormat(state, state->video_preview_track->current_dimension.width, state->video_preview_track->current_dimension.height);

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

	dlog("initializing camera service\n");;
    mlog("OBJECTIVEC: create new device\n");
    NSArray *devices = [AVCaptureDevice devices];
    for (AVCaptureDevice *device in devices)
    {
        dlog("Device name: %s", [[[device localizedName] description] UTF8String]);
        if ([device hasMediaType:AVMediaTypeVideo])
        {
            if ([device position] == AVCaptureDevicePositionBack)
            {
                dlog("Device position : back");
            }
            else
            {
                dlog("Device position : front");
            }
        }
    }
   
    //1.
    mlog("OBJECTIVEC: new AVCaptureSession");
    state->session = [[AVCaptureSession alloc] init];
    [state->session setSessionPreset:AVCaptureSessionPresetHigh];
  
    //2.
    mlog("OBJECTIVEC: new AVCaptureDevice");
    state->device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];

    if (nil == state->device) {
        mlog("No AVCaptureDevice found, bailing!!");
        err = kFskErrNotFound;
        goto bail;
    }

    NSError *error = nil;
    dlog( "new AVCaptureDeviceInput");
    state->captureInput = [AVCaptureDeviceInput deviceInputWithDevice:state->device error:&error];
    if ( [state->session canAddInput:state->captureInput] )
    {
        mlog("OBJECTIVEC: setting captureInput");
        [state->session addInput:state->captureInput];
    }
    
    //3.
    mlog("OBJECTIVEC: new captureStillOutput");
    state->captureStillOutput = [[AVCaptureStillImageOutput alloc] init];
    NSDictionary *outputSettings = [[NSDictionary alloc] initWithObjectsAndKeys:AVVideoCodecJPEG,AVVideoCodecKey,nil];
    [state->captureStillOutput setOutputSettings:outputSettings];
    [outputSettings release];
	[state->session addOutput:state->captureStillOutput];

    //4.
    mlog("OBJECTIVEC: new CaptureDelegate");
    state->captureDelegate = [[CaptureDelegate alloc] initWithRefcon:state];
    mlog("OBJECTIVEC: new AVCaptureVideoDataOutput");
    state->captureVideoOutput = [[AVCaptureVideoDataOutput alloc] init];
    [state->session addOutput:state->captureVideoOutput];
    dispatch_queue_t queue = dispatch_queue_create("cameraQueue", NULL);
    [state->captureVideoOutput setSampleBufferDelegate: state->captureDelegate queue:queue];
    dispatch_release(queue);

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

    //We already have the device
    err = cameraDeviceInitialize(state);
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
	dlog("into AVFCameraGetProperties\n" );
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	
	return cameraGenericProperties;
}


static Boolean cameraGenericCanHandle(const char *mimeType)
{
	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericCanHandle(), mimeType: %s\n", mimeType);
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if( 0 == FskStrCompareCaseInsensitive( mimeType, "video/x-kinoma-capture" ) )
	{
		dlog("bingo!!!\n");		
		return true;
	}
	
	return false;
}


static FskErr cameraGenericSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	int err = kFskErrUnknownElement;
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


static FskErr cameraGenericNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
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
		mlog("initialization failing, dispoing!!!\n");
		(state->reader->doSetState)(state->reader, kFskMediaPlayerStateClosed);
		cameraGenericDispose(reader, state);
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


static FskErr cameraGenericDispose(FskMediaReader reader, void *readerState)
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

            if (track->data)
            {
                FskMemPtrDispose(track->data);
            }

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

            if (track->data)
            {
                FskMemPtrDispose(track->data);
            }

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

static FskErr cameraGenericGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track_out)
{
	CameraGeneric	  state  = (CameraGeneric)readerState;
	CameraGenericTrack track  = NULL;

	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericGetTrack(): index: %d\n", index);
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

	if( !state->initialized )
		return -1;

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


static FskErr cameraGenericStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
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
		err = kFskErrBadState;
		goto bail;
	}
	
	state->debug_captured_frame_count	= 0;
	state->debug_received_frame_count	= 0;
	state->camera_start_time	= 0;

    mlog("OBJECTIVEC: state->session startRunning\n");
    [state->device lockForConfiguration:nil];
    [state->session startRunning];
    [state->device unlockForConfiguration];

bail:
	dlog("out of cameraGenericStart, err: %d\n", err);
	return err;
}


static FskErr cameraGenericStop(FskMediaReader reader, void *readerState)
{
	CameraGeneric state = (CameraGeneric)readerState;
	//status_t status;
	
	dlog("\n\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	dlog("into cameraGenericStop()\n");;
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");

    [state->session stopRunning];
	
	dlog("out of cameraGenericStop\n");
	return kFskErrNone;		//@@
}


static FskErr cameraGenericExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *trackOut, UInt32 *infoCountOut, FskMediaReaderSampleInfo *infoOut, unsigned char **data)
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
        dlog("check if there is any photo data\n");
        if( state->photo_track->data_size!= 0 )
        {
            mlog("allocating a new sample record for photo\n");
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

                char *path = NULL;
                FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeDocument, true, NULL, &path);
                char *jpg_path = FskStrDoCat(path, "kinoma_avf_camera_ouput.jpg");
                FILE *f = NULL;
                f = fopen(jpg_path, "wb");
                if( f == NULL )
                {
                    dlog( "can't create kinoma camera output jpg file!!! \n");
                }
                else
                {
                    fwrite(track->data, 1, track->data_size, f);
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

    frame = track->data;
    size  = track->data_size;
    track->data = 0;
    track->data_size = 0;
    
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


static FskErr cameraGenericGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
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

    dlog("checking current dimension\n");;
    width  = track->current_dimension.width;
    height = track->current_dimension.height;

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

    if( 0 == FskStrCompareCaseInsensitive("image", track->media_type) )
    {
        state->photo_track->current_dimension = property->value.dimension;
    }
    else if( 0 == FskStrCompareCaseInsensitive("video-preview", track->media_type) )
    {
        state->video_preview_track->current_dimension = property->value.dimension;
        cameraDeviceSetActiveFormat(state, property->value.dimension.width, property->value.dimension.height);
    }

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
		mlog("taking a picture!!!\n");

        cameraDeviceSetActiveFormat(state, track->current_dimension.width, track->current_dimension.height);

        [state->captureStillOutput captureStillImageAsynchronouslyFromConnection:[state->captureStillOutput connectionWithMediaType:AVMediaTypeVideo] completionHandler:^(CMSampleBufferRef imageDataSampleBuffer, NSError *error)
        {
            //Now we have the picture data, restore the preview dimension
            cameraDeviceSetActiveFormat(state, state->video_preview_track->current_dimension.width, state->video_preview_track->current_dimension.height);

            //Link error on iOS for kCGImagePropertyExifDictionary
            CFDictionaryRef exifAttachments = NULL;//CMGetAttachment(imageDataSampleBuffer, kCGImagePropertyExifDictionary, NULL);
            if (exifAttachments) {
                // Do something with the attachments.
            }

            mlog("image captured!!!!!\n");

            // Continue as appropriate.
            NSData *imageData = [AVCaptureStillImageOutput jpegStillImageNSDataRepresentation:imageDataSampleBuffer];
            //unsigned char *bytes = imageData.bytes;

            int photo_data_size = [imageData length];
            Byte *photo_data = [imageData bytes];
            mlog("image captured, len: %d\n", photo_data_size);

            if( track->data != NULL )
                FskMemPtrDisposeAt(&track->data);

                track->data      = NULL;
                track->data_size = 0;
                
                FskErr err = FskMemPtrNewFromData( photo_data_size, photo_data, &track->data );
                if( err == kFskErrNone )
                    track->data_size = photo_data_size;

                preview_event_callback((void *)state, 999, track->data);
        }];
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
//***bnie 	property->value.str = state->mFskCamera->getParameters();

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

//***bnie 	state->mFskCamera->setParameters(s);


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

    AVCaptureDevicePosition position;
    if( strcmp( s, "back" ) == 0 )
    {
        mlog("choosing back camera!!!\n");
        position = AVCaptureDevicePositionBack;
    }
    else if( strcmp( s, "front" ) == 0 )
    {
        mlog("choosing front camera!!!\n");
        position = AVCaptureDevicePositionFront;
    }

#if TARGET_OS_IPHONE
    if ([state->device position] == position)
    {
        mlog("We are using the same camera, no need to switch!\n");
        goto bail;
    }
#endif

    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice *device in devices)
    {
#if TARGET_OS_IPHONE
        if ([device position] == position) {
#else
        if (device != state->device) {
#endif
            [state->session beginConfiguration];

            [state->session removeInput:state->captureInput];

            state->device = device;
            state->captureInput = [AVCaptureDeviceInput deviceInputWithDevice:state->device error:nil];
            if ( [state->session canAddInput:state->captureInput] )
            {
                mlog("OBJECTIVEC: setting captureInput");
                [state->session addInput:state->captureInput];
            }

            [state->session commitConfiguration];

            //We already have the device
            err = cameraDeviceInitialize(state);
            BAIL_IF_ERR( err );

            break;
        }
    }

bail:
	mlog("out of cameraGenericSetCameraParameters() : %s\n", s);
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

    if (![state->device isFocusModeSupported:AVCaptureFocusModeAutoFocus]) {
        dlog("camera does not support auto focus, bailing!!!\n");;
        err = kFskErrInvalidParameter;
        goto bail;
    }

	if( af_state == 1 )
    {
		mlog("autoFocus!!!\n");

        [state->device lockForConfiguration:nil];
        state->device.focusMode = AVCaptureFocusModeAutoFocus;
        [state->device unlockForConfiguration];
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

    if (!state->device.isFocusPointOfInterestSupported) {
        dlog("camera does not support focus area, bailing!!!\n");;
        err = kFskErrInvalidParameter;
        goto bail;
    }

    CGFloat x = ((CGFloat)area.x + 1000.0) / 2000.0;
    CGFloat y = ((CGFloat)area.y + 1000.0) / 2000.0;

    mlog("Camera focus Point (%f, %f)\n", x, y);

    [state->device lockForConfiguration:nil];
    state->device.focusPointOfInterest = CGPointMake(x, y);
    [state->device unlockForConfiguration];

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

    if (!state->device.isFocusPointOfInterestSupported) {
        dlog("camera does not support focus area, bailing!!!\n");;
        err = kFskErrInvalidParameter;
        goto bail;
    }

	//state->mFskCamera->getAutoFocusArea(&area);
    CGPoint focusPoint = state->device.focusPointOfInterest;
    area.x = (SInt32)focusPoint.x;
    area.y = (SInt32)focusPoint.y;
    area.width = 100;
    area.height = 100;

	property->type = kFskMediaPropertyTypeRectangle;
	property->value.rect = area;

bail:
	mlog("out of cameraGenericGetAutoFocusArea()\n");
	return err;
}
