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
#include <string.h>
#include <utils/threads.h>

#define __FSKTHREAD_PRIV__
#define MAX_FRAME_TO_BUFFER			2

#include "FskBitmap.h"
#include "FskThread.h"
#include "FskList.h"
#include "FskTime.h"
#include "camera_jni.h"

#include "FskPlatformImplementation.h"
FskInstrumentedSimpleType(CameraAndroidJava, CameraAndroidJava);
#define mlog  FskCameraAndroidJavaPrintfMinimal     //LOGE
#define nlog  FskCameraAndroidJavaPrintfNormal
#define vlog  FskCameraAndroidJavaPrintfVerbose
#define dlog  FskCameraAndroidJavaPrintfDebug       //LOGD

//Mock up a android camera jni context for get the camera instance
class JNICameraContext: public CameraListener {
public:
    JNICameraContext(JNIEnv* env, jobject weak_this, jclass clazz, const sp<Camera>& camera);
    ~JNICameraContext() { release(); }
    virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
    virtual void postData(int32_t msgType, const sp<IMemory>& dataPtr);
    virtual void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);
    void addCallbackBuffer(JNIEnv *env, jbyteArray cbb);
    void setCallbackMode(JNIEnv *env, bool installed, bool manualMode);
    sp<Camera> getCamera() { /*Mutex::Autolock _l(mLock); */return mCamera; }
    void release();
	
private:
    void copyAndPost(JNIEnv* env, const sp<IMemory>& dataPtr, int msgType);
    void clearCallbackBuffers_l(JNIEnv *env);
	
    jobject     mCameraJObjectWeak;     // weak reference to java object
    jclass      mCameraJClass;          // strong reference to java class
    sp<Camera>  mCamera;                // strong reference to native object
    //Mutex       mLock;
	
    Vector<jbyteArray> mCallbackBuffers; // Global reference application managed byte[]
    bool mManualBufferMode;              // Whether to use application managed buffers.
    bool mManualCameraCallbackSet;       // Whether the callback has been set, used to reduce unnecessary calls to set the callback.
};

static jclass gClassFskCamera = NULL;
static jobject gObjectFskCamera = NULL;
static JavaVM *gJavaVM = NULL;
static jfieldID gNativeContextID = 0;
static jfieldID gPreviewWidthID = 0;
static jfieldID gPreviewHeightID = 0;
static jfieldID gPreviewFormatID = 0;
static jfieldID gCameraCountID = 0;

typedef struct FrameRecord FrameRecord;		//output callback
typedef struct FrameRecord *FramePtr;
struct FrameRecord
{
	FramePtr			next;
	unsigned char		*frame;
	int					size;
	FskInt64			time;
};

//#define BAIL_IF_ERR(err) if (err) goto bail;
#define CALL_JAVA_METHOD(cls, mtd, obj) do {  \
	jmethodID id = env->GetMethodID(cls, mtd, "()V");	 \
	if (id) {	\
		env->CallVoidMethod(obj, id);    \
		if(env->ExceptionCheck()){   \
			dlog("Calling Java %s() throw exception", mtd); \
			env->ExceptionClear();   \
			err = kFskErrBadState;   \
		}    \
		else    \
			dlog("Calling Java %s() done", mtd);  \
	}   \
	else {   \
		dlog("Java %s() Not found", mtd);   \
		err = kFskErrUnimplemented;   \
	}   \
} while(0)

#define CALL_JAVA_METHOD_ARG1(cls, mtd, obj, arg1) do {  \
    jmethodID id = env->GetMethodID(cls, mtd, "(I)V");	 \
    if (id) {	\
        env->CallVoidMethod(obj, id, arg1);    \
        if(env->ExceptionCheck()){   \
            dlog("Calling Java %s() throw exception", mtd); \
            env->ExceptionClear();   \
            err = kFskErrBadState;   \
        }    \
        else    \
            dlog("Calling Java %s() done", mtd);  \
    }   \
    else {   \
        dlog("Java %s() Not found", mtd);   \
        err = kFskErrUnimplemented;   \
    }   \
} while(0)



FskErr frame_queue_in( FskListMutex frame_list, unsigned char *frame,  int size, FskInt64 time )
{
	FramePtr item = NULL;
	FskErr      err = kFskErrNone;

	if (!frame_list) {
		err = kFskErrBadState;
		goto bail;
	}

	err = FskMemPtrNewClear(sizeof(FrameRecord), (FskMemPtr *)&item);
	BAIL_IF_ERR( err );
	
	item->frame = frame;
	item->size  = size;
	item->time	= time;
	
	FskListMutexAppend(frame_list, item);
	
bail:
	return err;
}


FskErr frame_queue_out_first( FskListMutex frame_list, unsigned char **frame, int *size, FskInt64 *time)
{
	FramePtr	item = NULL;
	FskErr      err = kFskErrNone;

	if (!frame_list) {
		err = kFskErrBadState;
		goto bail;
	}
	
	item = (FramePtr)FskListMutexRemoveFirst(frame_list);
	
	if( item != NULL )
	{
		*frame = item->frame;
		*size  = item->size;
		*time  = item->time;
	}
	else
	{
		dlog("return NULL frame\n" );
		*frame = NULL;
		*size  = 0;
		*time  = 0;
		
		err = kFskErrNotFound;
	}
	
bail:
	return err;
}

FskErr frame_queue_out_last( FskListMutex frame_list, unsigned char **frame, int *size, FskInt64 *time)
{
	FramePtr	item = NULL;
	FskErr      err = kFskErrNone;

	if (!frame_list) {
		err = kFskErrBadState;
		goto bail;
	}
	
	item = (FramePtr)FskListMutexRemoveLast(frame_list);
	
	if( item != NULL )
	{
		*frame = item->frame;
		*size  = item->size;
		*time  = item->time;
	}
	else
	{
		dlog("return NULL frame\n" );
		*frame = NULL;
		*size  = 0;
		*time  = 0;
		
		err = kFskErrNotFound;
	}
	
bail:
	return err;
}


static void frame_queue_flush( FskListMutex frame_list )
{
	if( frame_list == NULL )
		return;
	
	while(1)
	{
		FramePtr item = (FramePtr)FskListMutexRemoveFirst(frame_list);;
		if( item == NULL )
		{
			dlog("no more frame in queue!!!\n" );
			break;
		}
		
		if( item->frame != NULL )
		{
			dlog("dispose this frame buffer!!!\n");
			FskMemPtrDispose(item->frame);
		}
		else
		{
			dlog("func_item->completionFunction == NULL!!!\n" );
		}
		
		FskMemPtrDispose(item);
	}
}

void dumpYUV(int width, int height, unsigned char * addr, int size, const char *path )
{
	static FILE *f = NULL;
			
	if( f == NULL )
	{
		f = fopen(path, "wb");
		if( f == NULL )
		{
			dlog( "SSSSSSSSSSSSSSSSSSSS=> can't create yuv output file!!! \n");
			return;
		}		
	}
					
	fwrite(addr, 1, size, f);			
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
	JNIEnv *env;
	
	//dlog("JNI_OnLoad");

	gJavaVM = vm;
	if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
		//mlog("Failed to get env using GetEnv()");
		return -1;
	}
	
	jclass cls = env->FindClass(CLASSNAME);	
	if (!cls) {
		//mlog("Failed to get class com.kinoma.kinomaplay.FskCamera");
		return -1;
	}
	
	gClassFskCamera = reinterpret_cast<jclass>(env->NewGlobalRef(cls));

    gNativeContextID = env->GetFieldID(gClassFskCamera, "mNativeContext", "I");
	if (!gNativeContextID) {
		//mlog("Can't find com/kinoma/kinomaplay/FskCamera.mNativeContext");
		return -1;
	}
	
	return JNI_VERSION_1_4;
}

void java_call_init()
{
	int status;
	JNIEnv		*env;
	FskThread self = FskThreadGetCurrent();
	
	if (!self->jniEnv) {

		status = gJavaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
		if (status < 0) {
			dlog("failed to get JNI Env - AttachCurrentThread");
			status = gJavaVM->AttachCurrentThread(&env, NULL);
			if (status < 0) {
				dlog("failed to attach");
				return;
			}
			self->attachedJava = 1;
		}
		
    	self->jniEnv = (int)env;
	}
}

sp<Camera> get_native_camera(JNIEnv *env, jobject cam)
{
    sp<Camera> camera;
    //Mutex::Autolock _l(mLock);
		
	jclass clazz = env->FindClass("android/hardware/Camera");
	if (clazz == NULL) {
		dlog("Can't find android/hardware/Camera");
		return 0;
	}
	
	jfieldID fieldID = env->GetFieldID(clazz, "mNativeContext", "I");
	if (fieldID == NULL) {
		dlog("Can't find android/hardware/Camera.mNativeContext");
		return 0;
	}
	
    JNICameraContext* context = reinterpret_cast<JNICameraContext*>(env->GetIntField(cam, fieldID));
    if (context != NULL) {
        camera = context->getCamera();
    }
	
    dlog("get_native_camera: context=%p, camera=%p", context, camera.get());
	
    if (camera == 0) {
		dlog("Camera get released");
    }

    return camera;
}

FskCamera *FskCamera::getFskCamera()
{
	java_call_init();

	FskThread self = FskThreadGetCurrent();
	JNIEnv		*env = (JNIEnv*)self->jniEnv;

	FskCamera *camera = reinterpret_cast<FskCamera*>(env->GetIntField(gObjectFskCamera, gNativeContextID));
	dlog("Get FskCamera %p", camera);
	
	return camera;
}

FskCamera::FskCamera()
{
	//dlog("FskCamera()");
	mFrameList = NULL;
    photo_data = NULL;
    photo_data_size = 0;
}

FskErr FskCamera::init()
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv		*env = (JNIEnv*)self->jniEnv;
	FskErr		err = kFskErrNone;

	CALL_JAVA_METHOD(gClassFskCamera, "init", gObjectFskCamera);

	return err;
}

FskErr FskCamera::finish()
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv		*env = (JNIEnv*)self->jniEnv;
	FskErr		err = kFskErrNone;

	CALL_JAVA_METHOD(gClassFskCamera, "finish", gObjectFskCamera);

	return err;
}

FskErr FskCamera::stopPreview()
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv		*env = (JNIEnv*)self->jniEnv;
	FskErr		err = kFskErrNone;

	CALL_JAVA_METHOD(gClassFskCamera, "stopPreview", gObjectFskCamera);

	if (mFrameList) {
		frame_queue_flush(mFrameList);
		FskListMutexDispose(mFrameList);
	}

	mFrameList = NULL;

	return err;
}

FskErr FskCamera::startPreview()
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv		*env = (JNIEnv*)self->jniEnv;
	FskErr		err = kFskErrNone;

	CALL_JAVA_METHOD(gClassFskCamera, "startPreview", gObjectFskCamera);

	FskListMutexNew(&mFrameList, "FskCameraFrameList");
	
	frameTotal = 0;
	frameDropped = 0;
	FskTimeGetNow(&mStartTime);

	return err;
}

FskErr FskCamera::takePicture()
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv		*env = (JNIEnv*)self->jniEnv;
	FskErr		err = kFskErrNone;

	CALL_JAVA_METHOD(gClassFskCamera, "takePicture", gObjectFskCamera);

	return err;
}

FskErr FskCamera::autoFocus()
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv		*env = (JNIEnv*)self->jniEnv;
	FskErr		err = kFskErrNone;

	CALL_JAVA_METHOD(gClassFskCamera, "autoFocus", gObjectFskCamera);

	return err;
}

FskErr FskCamera::switchCamera()
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv		*env = (JNIEnv*)self->jniEnv;
	FskErr		err = kFskErrNone;

	CALL_JAVA_METHOD(gClassFskCamera, "switchCamera", gObjectFskCamera);

	return err;
}

FskErr FskCamera::chooseCamera(int index)
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv		*env = (JNIEnv*)self->jniEnv;
	FskErr		err = kFskErrNone;

	CALL_JAVA_METHOD_ARG1(gClassFskCamera, "chooseCamera", gObjectFskCamera, index);

	return err;
}

int FskCamera::pullPreviewFrame(unsigned char **frame, int *size, FskInt64 *time)
{
	frame_queue_out_first(mFrameList, frame, size, time);
    
	dlog("Pop frame %p", *frame);
    
	return 0;
}

int FskCamera::pullLatestPreviewFrame(unsigned char **frame, int *size, FskInt64 *time)
{
	frame_queue_out_last(mFrameList, frame, size, time);

    frame_queue_flush( mFrameList );
    
	dlog("Pop last frame %p", *frame);
    
	return 0;
}

int FskCamera::pushPreviewFrame(unsigned char *frame, int size)
{
	//Some devices still call the callback even stopPreview called.
	if (!mFrameList)
		return 0;

	unsigned char *newFrame = NULL;

	frameTotal += 1;
	int frame_total = (int)FskListMutexCount(mFrameList);
	if( frame_total >= MAX_FRAME_TO_BUFFER )
	{
		int this_size;
		unsigned char *frame = NULL;
		FskInt64 time;

		dlog("frame_total exceeding MAX_FRAME_TO_BUFFER, popup one!!!", newFrame);
		frameDropped += 1;
		frame_queue_out_first( mFrameList, &frame, &this_size, &time);
		if( frame != NULL )
		{
            dlog("==============================================>drop frame time: %lld\n", time);
			if( this_size == size )
                newFrame = frame;
            else
                FskMemPtrDispose(frame);
		}
	}

    if( newFrame == NULL )
        FskMemPtrNew(size, &newFrame);
	
    memcpy(newFrame, frame, size);
	
    FskTimeRecord now;
    FskTimeGetNow(&now);
    FskTimeSub(&mStartTime, &now);
    
    //Using relative time as timestamp, absolute time too easy to overflow
    //FskTimeInMS returns a SInt32, it will overflow after running days
    FskInt64 time = (FskInt64)FskTimeInMS(&now);
    dlog("===========================================>push frame time: %lld\n", time);

	//Check if the camera could be stopped, as the memcpy() may take some time.
	if (kFskErrNone != frame_queue_in(mFrameList, newFrame, size, time)) {
		FskMemPtrDispose(newFrame);
		return 0;
	}

	postEventProc(postEventProcRefcon, 888);
	
	dlog("Pushing frame %p, frameTotal: %d, dropped: %d", newFrame, frameTotal, frameDropped);
	return 0;
}

int FskCamera::getPreviewSize(int *width, int *height)
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv *env = (JNIEnv*)self->jniEnv;

	if (!gPreviewWidthID || !gPreviewHeightID) {

		gPreviewWidthID = env->GetFieldID(gClassFskCamera, "mPreviewWidth", "I");	
		if (!gPreviewWidthID) {			
			dlog("Can not get the preview width field");
			return -1;
		}

		gPreviewHeightID = env->GetFieldID(gClassFskCamera, "mPreviewHeight", "I");	
		if (!gPreviewWidthID) {			
			dlog("Can not get the preview height field");
			return -1;
		}
	}

	*width = env->GetIntField(gObjectFskCamera, gPreviewWidthID);
	*height = env->GetIntField(gObjectFskCamera, gPreviewHeightID);

	return 0;
}

int FskCamera::getCameraCount()
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv *env = (JNIEnv*)self->jniEnv;

	if (!gCameraCountID) {
		gCameraCountID = env->GetFieldID(gClassFskCamera, "mCameraCount", "I");
		if (!gCameraCountID) {
			dlog("Can not get mCameraCount field");
			return 0;
		}
	}

	int count = env->GetIntField(gObjectFskCamera, gCameraCountID);
	dlog("camera count: %d", count);

	return count;
}

int FskCamera::getPreviewFormat()
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv *env = (JNIEnv*)self->jniEnv;

	if (!gPreviewFormatID) {
		gPreviewFormatID = env->GetFieldID(gClassFskCamera, "mPreviewFormat", "I");	
		if (!gPreviewFormatID) {			
			dlog("Can not get the preview format field");
			return 0;
		}
	}

	int format = env->GetIntField(gObjectFskCamera, gPreviewFormatID);
	dlog("Preview format: %d", format);

	if (format == 17) //NV12 according to google docs, but it's YUV420sp here
		format = kFskBitmapFormatYUV420spvu;
	else
		format = kFskBitmapFormatYUV420;

	return format;
}

char *FskCamera::getParameters()
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv		*env = (JNIEnv*)self->jniEnv;
	jobject result;

	jmethodID id = env->GetMethodID(gClassFskCamera, "getParametersJSON", "()Ljava/lang/String;");
	if (id) {
		result = env->CallObjectMethod(gObjectFskCamera, id);
		if(env->ExceptionCheck()){
			dlog("Calling Java getParametersJSON() throw exception");
			env->ExceptionClear();
			return NULL;
		}
		else
			dlog("Calling Java getParametersJSON() done");
	}
	else {
		dlog("Java getParametersJSON() Not found");
		return NULL;
	}

	const char *str = env->GetStringUTFChars((jstring)result, 0);
	char *para = FskStrDoCopy(str);
	env->ReleaseStringUTFChars((jstring)result, str);

	dlog("Get camera parameter %s", para);

	return para;
}

FskErr FskCamera::setParameters(char *para)
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv		*env = (JNIEnv*)self->jniEnv;
	FskErr		err = kFskErrNone;

	jstring jstrPara= env->NewStringUTF(para);

	jmethodID id = env->GetMethodID(gClassFskCamera, "setParametersJSON", "(Ljava/lang/String;)V");
	if (id) {
		env->CallVoidMethod(gObjectFskCamera, id, jstrPara);
		if(env->ExceptionCheck()){
			dlog("Calling Java setParameter() throw exception" );
			env->ExceptionClear();
			err = kFskErrBadState;
		}
		else
			dlog("Calling Java setParametersJSON() done");
	}
	else {
		dlog("Java setParametersJSON() Not found");
		err = kFskErrUnimplemented;
	}

	dlog("Set camera parameter %s", para);

	return err;
}

void FskCamera::notify(int32_t msgType, int32_t ext1, int32_t ext2)
{
    dlog("notify");
}

void FskCamera::setpostEventProc(void *proc, void *refCon )
{
    postEventProc = (void (*)(void *, int ))proc;
    postEventProcRefcon = refCon;
}

void FskCamera::postData(int32_t msgType, const sp<IMemory>& dataPtr , camera_frame_metadata_t * metadata)
{
	int size;
	unsigned char * addr;

    dlog("FskCamera::postData: (%d, %p)", msgType, dataPtr.get());
    
    switch(msgType) {
		case CAMERA_MSG_VIDEO_FRAME:
			// should never happen
			dlog("CAMERA_MSG_VIDEO_FRAME");
			break;
			// don't return raw data to Java
		case CAMERA_MSG_RAW_IMAGE:
			dlog("CAMERA_MSG_RAW_IMAGE");
			break;
		
		case CAMERA_MSG_PREVIEW_FRAME:
			dlog("CAMERA_MSG_PREVIEW_FRAME");
	
			size = dataPtr->size();
			addr = (unsigned char *)dataPtr->pointer();
			pushPreviewFrame(addr, size);

#ifdef DUMP_YUV
			dumpYUV(width, height, addr, size, "/sdcard/camera_yuv420.yuv");
#endif
			break;
		default:
			break;
    }
}

void FskCamera::postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr)
{
    // TODO: plumb up to Java. For now, just drop the timestamp
    //postData(msgType, dataPtr);
	dlog("postDataTimestamp");
}


void FskCamera::getPhotoData(unsigned char **photo_data, int *photo_data_size)
{
    *photo_data      = this->photo_data;
    *photo_data_size = this->photo_data_size;

    this->photo_data = NULL;
    this->photo_data_size = 0;

    dlog("out of FskCamera::getPhotoData(), *photo_data: %x, *photo_data_size: %d", (int)*photo_data, (int)*photo_data_size);
}


void FskCamera::setPhotoData(unsigned char *photo_data, int photo_data_size)
{
    FskErr	err = kFskErrNone;

    if( this->photo_data != NULL )
        FskMemPtrDisposeAt(&this->photo_data);

    this->photo_data      = NULL;
    this->photo_data_size = 0;

    err = FskMemPtrNewFromData( photo_data_size, photo_data, &this->photo_data );
    if( err == kFskErrNone )
        this->photo_data_size = photo_data_size;

    postEventProc(postEventProcRefcon, 999);
}

FskErr FskCamera::getAutoFocusArea(FskRectangle area)
{
	FskThread self = FskThreadGetCurrent();
	JNIEnv		*env = (JNIEnv*)self->jniEnv;
	FskErr		err = kFskErrNone;
	jobject 	result;

	jmethodID id = env->GetMethodID(gClassFskCamera, "getFocusArea", "()Ljava/lang/String;");
	if (id) {
		result = env->CallObjectMethod(gObjectFskCamera, id);
		if(env->ExceptionCheck()){
			dlog("Calling Java getFocusArea throw exception");
			env->ExceptionDescribe();
			env->ExceptionClear();
			err = kFskErrBadState;
		}
		else
			dlog("Calling Java getFocusArea done");
	}
	else {
		dlog("Java getFocusArea() Not found");
		err = kFskErrUnimplemented;
	}

	if (result) {
		const char *str = env->GetStringUTFChars((jstring)result, 0);
		dlog("Get auto focus area %s", str);
		int left, top, right, bottom;

		sscanf(str, "[%d,%d][%d,%d]", &left, &top, &right, &bottom);

		area->x = left;
		area->y = top;
		area->width = (right > left) ? (right - left) : 0;
		area->height = (bottom > top) ? (bottom - top) : 0;

		env->ReleaseStringUTFChars((jstring)result, str);
	}
	else {
		area->x = 0;
		area->y = 0;
		area->width = 0;
		area->height = 0;
	}

	return err;
}

FskErr FskCamera::setAutoFocusArea(FskRectangle area)
{
	FskErr		err = kFskErrNone;

	char tmp[128];
	snprintf(tmp, sizeof(tmp), "{\"focusAreas\":[{\"left\"=\"%d\",\"top\"=\"%d\",\"right\"=\"%d\",\"bottom\"=\"%d\",\"weight\"=\"%d\"}]}",
		area->x, area->y, area->x + area->width, area->y + area->height, 1000);

	err = setParameters(tmp);

	return err;
}
void
JAVANAME(FskCamera_nativeInit)(JNIEnv* env, jobject thiz)
{
	//dlog("nativeInit");

	gObjectFskCamera = reinterpret_cast<jobject>(env->NewGlobalRef(thiz));

	sp<FskCamera> listener= new FskCamera();
	listener->incStrong(thiz);

	// save context in opaque field
	//dlog("Save Context");
    env->SetIntField(thiz, gNativeContextID, (int)listener.get());
}

void
JAVANAME(FskCamera_setNativeCallback)( JNIEnv* env, jobject thiz, jobject cam )
{
	dlog("setNativeCallback");

	sp<Camera> camera = get_native_camera(env, cam);

	FskCamera *listener = reinterpret_cast<FskCamera*>(env->GetIntField(thiz, gNativeContextID));

	camera->setListener(listener);
	camera->setPreviewCallbackFlags(5);
}

void
JAVANAME(FskCamera_unsetNativeCallback)( JNIEnv* env, jobject thiz, jobject cam )
{
	dlog("unsetNativeCallback");
	
	sp<Camera> camera = get_native_camera(env, cam);
	
	camera->setListener(0);
}

void
JAVANAME(FskCamera_nativeDataCallback)( JNIEnv* env, jobject thiz, jbyteArray data, jobject cam , int32_t type)
{
	dlog("nativeDataCallback");

	FskCamera *fskcam = reinterpret_cast<FskCamera*>(env->GetIntField(thiz, gNativeContextID));
	
	jbyte *frame = env->GetByteArrayElements(data, 0);
	int size = (int)env->GetArrayLength(data);

	if (frame != NULL)
    {
        if( type == 0 )
        {
            dlog("FskCamera_nativeDataCallback=>fskcam->pushPreviewFrame()");
            fskcam->pushPreviewFrame((unsigned char*)frame, size);
        }
        else
        {
            dlog("FskCamera_nativeDataCallback=>Photo data");
            fskcam->setPhotoData( (unsigned char *)frame, size );
        }

        env->ReleaseByteArrayElements(data, frame, 0);
	}
}
