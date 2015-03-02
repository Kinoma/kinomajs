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
#include <jni.h>
#include <camera/Camera.h>
#include <binder/IMemory.h>
#include "FskRectangle.h"

using namespace android;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef OBJECTBASE
        #define OBJECTBASE      Java_com_kinoma_kinomaplay_
#endif /* OBJECTBASE */

#define mY( x, y ) x##y
#define mYY( x, y ) mY( x, y )
#define JAVANAME( x ) mYY( OBJECTBASE , x )

	void JAVANAME(FskCamera_nativeInit)(JNIEnv* env, jobject thiz);
	void JAVANAME(FskCamera_setNativeCallback)( JNIEnv* env, jobject thiz, jobject cam );	
	void JAVANAME(FskCamera_unsetNativeCallback)( JNIEnv* env, jobject thiz, jobject cam );
	void JAVANAME(FskCamera_nativeDataCallback)( JNIEnv* env, jobject thiz, jbyteArray data, jobject camera, int32_t type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

//Our own listener
class FskCamera : public CameraListener {
public:	
	virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
    virtual void postData(int32_t msgType, const sp<IMemory>& data, camera_frame_metadata_t * metadata);
    virtual void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);

    FskCamera();
	static FskCamera *getFskCamera();
	FskErr init();
	FskErr finish();
	FskErr startPreview();
	FskErr stopPreview();
	FskErr takePicture();
	FskErr autoFocus();
	FskErr switchCamera();
	FskErr chooseCamera(int index);
	int  getCameraCount();
	char  *getParameters();
	FskErr setParameters(char *para);
	void setpostEventProc(void *proc, void *refCon);
    void getPhotoData(unsigned char **photo_data, int *photo_data_size);
    void setPhotoData(unsigned char *photo_data, int photo_data_size);
    FskErr getAutoFocusArea(FskRectangle FocusArea);
    FskErr setAutoFocusArea(FskRectangle FocusArea);
	int pullPreviewFrame(unsigned char **frame, int *size, FskInt64 *time);
	int pullLatestPreviewFrame(unsigned char **frame, int *size, FskInt64 *time);
	int pushPreviewFrame(unsigned char *frame, int size);
	int getPreviewSize(int *width, int *height);
	int getPreviewFormat();
	FskTime getStartTime(){return &mStartTime;};


    void (*postEventProc)(void *refcon, int event);
    void *postEventProcRefcon;


	FskListMutex mFrameList;
	FskSemaphore mSemaphore;
	FskTimeRecord mStartTime;

	int frameTotal;
	int frameDropped;

    unsigned char *photo_data;
    int photo_data_size;
};

