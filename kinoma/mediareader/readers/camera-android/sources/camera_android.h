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

#include "FskFiles.h"
#include "FskEndian.h"
#include "FskList.h"
//#define LOG_NDEBUG		0

#if (defined(Froyo_Build)||defined(Ginger_Build))
#define F_G_Build	1
#define SUPPORT_F_G_SURFACE_PREVIEW
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef Jellybean_Build
#include <gui/ISurface.h>
#include <gui/SurfaceTexture.h>
#elif defined Icecreamsanwich_Build
#include <gui/SurfaceTexture.h>
#include <surfaceflinger/ISurface.h>
#else
#include <surfaceflinger/ISurface.h>
#endif
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <ui/GraphicBuffer.h>
#include <camera/ICamera.h>
#include <camera/ICameraClient.h>
#include <camera/ICameraService.h>
#ifdef F_G_Build
#include <ui/Overlay.h>
#endif
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <utils/KeyedVector.h>
#include <utils/Log.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
extern FskInstrumentedTypeRecord gFskCameraAndroidTypeInstrumentation;
//FskInstrumentedSimpleType(FskCameraAndroid, FskCameraAndroid);
#else
#define dlog(...)
#endif

#include "kinoma_utilities.h"

using namespace android;

typedef struct SizeRecord SizeRecord;		//output callback
typedef struct SizeRecord *SizePtr;
struct SizeRecord
{
	SizePtr				next;
	int					width;
	int					height;
};

//class MCameraClient;
//class FGSurface;
//class SimpleSurface;
class SimpleSurface 
{
public:
    // new functions
	SimpleSurface(void *refCon);
	~SimpleSurface();
	
    void clearStat();
	void push_data(int size, unsigned char *frame); 
	int  pull_data(unsigned char **frame, int *size, FskInt64 *time);
	
private:
    // check callback count
    Condition mCond;
    Mutex mLock;
	void *m_refcon;
	FskListMutex m_frame_list;
};

#ifdef SUPPORT_F_G_SURFACE_PREVIEW
class FGSurface : public BnSurface 
{
public:
    virtual status_t registerBuffers(const BufferHeap& buffers);
    virtual sp<OverlayRef> createOverlay( uint32_t w, uint32_t h, int32_t format, int32_t orientation);
    virtual void postBuffer(ssize_t offset);
    virtual void unregisterBuffers();
    virtual sp<GraphicBuffer> requestBuffer(int bufferIdx, int usage);
	virtual sp<GraphicBuffer> requestBuffer(int a, uint32_t b, uint32_t c, uint32_t d, uint32_t e); 
	virtual status_t setBufferCount(int bufferCount);
    void waitUntil(int c0, int c1, int c2);
	
    // new functions
	FGSurface(void *refCon);
	~FGSurface();
	
    void clearStat();
	void push_data(int size, unsigned char *frame); 
	int  pull_data(unsigned char **frame, int *size, FskInt64 *time);
	
private:
    // check callback count
    Condition mCond;
    Mutex mLock;
    int registerBuffersCount;
    int postBufferCount;
    int unregisterBuffersCount;
	
	BufferHeap m_buf;
	void *m_refcon;
	
	FskListMutex m_frame_list;
};
#endif

//
//  A mock CameraClient
//
class MCameraClient : public BnCameraClient 
{
public:
    virtual void notifyCallback(int32_t msgType, int32_t ext1, int32_t ext2);
#ifdef F_G_Build
    virtual void dataCallback(int32_t msgType, const sp<IMemory>& data);
#elif defined(Honey_Build)
    virtual void dataCallback(int32_t msgType, const sp<IMemory>& data);
#else	
    virtual void dataCallback(int32_t msgType, const sp<IMemory>& data, camera_frame_metadata_t *metadata);
#endif
    virtual void dataCallbackTimestamp(nsecs_t timestamp,
									   int32_t msgType, const sp<IMemory>& data);
	
	MCameraClient(void *refCon, int use_preview_display);
    // new functions
    void clearStat();
    enum OP { EQ, GE, LE, GT, LT };
    void assertNotify(int32_t msgType, OP op, int count);
    void assertData(int32_t msgType, OP op, int count);
    void waitNotify(int32_t msgType, OP op, int count);
    void waitData(int32_t msgType, OP op, int count);
    void assertDataSize(int32_t msgType, OP op, int dataSize);
	
    void setReleaser(ICamera *releaser) 
	{
		//dlog("into MCameraClient:setReleaser()\n");
        mReleaser = releaser;
    }
	
	int  pull_data(unsigned char **frame, int *size, FskInt64 *time);
	int  set_preview_display(sp<ICamera> cam);
	
private:
    Mutex mLock;
    Condition mCond;
    DefaultKeyedVector<int32_t, int> mNotifyCount;
    DefaultKeyedVector<int32_t, int> mDataCount;
    DefaultKeyedVector<int32_t, int> mDataSize;
    bool test(OP op, int v1, int v2);
    void assertTest(OP op, int v1, int v2);
	void *m_refcon;
	
	int						m_use_preview_display;
#ifdef SUPPORT_F_G_SURFACE_PREVIEW	
	sp<FGSurface>			m_fg_surface;
#else
	sp<SurfaceTexture>		m_surface_texture;
#endif
	SimpleSurface			*m_simple_surface;
	
    ICamera *mReleaser;
};


typedef struct FrameRecord FrameRecord;		//output callback
typedef struct FrameRecord *FramePtr;
struct FrameRecord
{
	FramePtr			next;
	unsigned char		*frame;
	int					size;
	FskInt64			time;
};


FskErr size_queue_in( FskListMutex size_list, int width,  int height );
void size_queue_flush( FskListMutex size_list );
void push_data_0(void *refCon, FskListMutex m_frame_list, int size, unsigned char *addr);
FskErr frame_queue_out( FskListMutex frame_list, unsigned char **frame, int *size, FskInt64 *time);
FskErr frame_queue_in( FskListMutex frame_list, unsigned char *frame,  int size, FskInt64 time );
