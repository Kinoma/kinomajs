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
#include "camera_android.h"

//#define	BUF_PRINTF
#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
//FskInstrumentedSimpleType(FskCameraAndroid, FskCameraAndroid);
#else
#define dlog(...)
#endif

#include "kinoma_utilities.h"

#ifdef BNIE_LOG
FILE* fErr = NULL; 
#elif defined( BNIE_INSTRUMENT )
FskInstrumentedTypeRecord gFskCameraAndroidTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "FskCameraAndroid"};
//FskInstrumentedSimpleType(FskCameraAndroid, FskCameraAndroid);
#define dlog(...)  do { FskInstrumentedTypePrintfDebug  (&gFskCameraAndroidTypeInstrumentation, __VA_ARGS__); } while(0)
#endif

#ifdef	BUF_PRINTF
#include "../../../../kinoma-ipp-lib/buf_printf.c"
#endif

using namespace android;

FskErr frame_queue_in( FskListMutex frame_list, unsigned char *frame,  int size, FskInt64 time )
{
	FramePtr item = NULL;
	FskErr      err = kFskErrNone;
	
	err = FskMemPtrNewClear(sizeof(FrameRecord), (FskMemPtr *)&item);
	BAIL_IF_ERR( err );
	
	item->frame = frame;
	item->size  = size;
	item->time	= time;
	
	FskListMutexAppend(frame_list, item);
	
bail:
	return err;
}


FskErr frame_queue_out( FskListMutex frame_list, unsigned char **frame, int *size, FskInt64 *time)
{
	FramePtr	item = NULL;
	FskErr      err = kFskErrNone;
	
	item = (FramePtr)FskListMutexRemoveFirst(frame_list);
	
	if( item != NULL )
	{
		dlog("return cached\n" );		
		*frame = item->frame;
		*size  = item->size;
		*time  = item->time;
	}
	else
	{
		dlog("return default\n" );		
		*frame = NULL;
		*size  = 0;
		*time  = 0;
		
		err = 1;
	}
	
	//bail:
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
			dlog("no more func_item in queue!!!\n" );		
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


FskErr size_queue_in( FskListMutex size_list, int width,  int height )
{
	SizePtr item = NULL;
	FskErr   err = kFskErrNone;
	
	err = FskMemPtrNewClear(sizeof(FrameRecord), (FskMemPtr *)&item);
	BAIL_IF_ERR( err );
	
	item->width  = width;
	item->height = height;
	
	//dlog("appending to list, item: %x, width/height: %d/%d\n", (int)item, width, height );		
	FskListMutexAppend(size_list, item);
	
bail:
	return err;
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






SimpleSurface::SimpleSurface(void *refCon)
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
	m_refcon = refCon;
	
	FskListMutexNew(&m_frame_list, "FrameList");
	//BAIL_IF_ERR( err ); 
	
}

SimpleSurface::~SimpleSurface()
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
	if( m_frame_list != NULL )
	{	
		dlog( "calling frame_queue_flush\n" ); 
		frame_queue_flush( m_frame_list );
		FskListMutexDispose(m_frame_list);		
	}
}

void SimpleSurface::clearStat() 
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
    Mutex::Autolock _l(mLock);
	if( m_frame_list != NULL )
	{	
		dlog( "calling frame_queue_flush\n" ); 
		frame_queue_flush( m_frame_list );
	}
}


int SimpleSurface::pull_data(unsigned char **frame, int *size, FskInt64 *time) 
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
    Mutex::Autolock _l(mLock);
	int err = 0;
	
	*frame = NULL;
	*size  = 0;
	*time  = 0;
	
	if( m_frame_list == NULL )
		return -666;
	
	while (true) 
	{
		int frame_total = (int)FskListMutexCount(m_frame_list);
		if(	frame_total	> 0 )
		{
			dlog("SSSSSSSSSSSSSSSSSSSS=> frame_total: %d break!!!\n", frame_total);
			break;
        }
		
		dlog("SSSSSSSSSSSSSSSSSSSS=> calling mCond.wait\n");
        mCond.wait(mLock);
    }
	
	frame_queue_out( m_frame_list, frame, size, time);
	dlog("popped up a frame, time: %d\n", (int)*time);
	
	return err;
}


void SimpleSurface::push_data(int size, unsigned char *frame) 
{
	int err = 0;
	
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
    Mutex::Autolock _l(mLock);
    
	push_data_0(m_refcon, m_frame_list, size, frame);
	
bail:	
    dlog("SSSSSSSSSSSSSSSSSSSS=> out of %s and signaling\n", __func__);
    mCond.signal();
	
}


#ifdef SUPPORT_F_G_SURFACE_PREVIEW
FGSurface::FGSurface(void *refCon)
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
	m_refcon = refCon;
	
	FskListMutexNew(&m_frame_list, "FrameList");
	//BAIL_IF_ERR( err ); 
	
}


FGSurface::~FGSurface()
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
	if( m_frame_list != NULL )
	{	
		dlog( "calling frame_queue_flush\n" ); 
		frame_queue_flush( m_frame_list );
		FskListMutexDispose(m_frame_list);		
	}
}


void FGSurface::clearStat() 
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
    Mutex::Autolock _l(mLock);
    registerBuffersCount = 0;
    postBufferCount = 0;
    unregisterBuffersCount = 0;
	
	if( m_frame_list != NULL )
	{	
		dlog( "calling frame_queue_flush\n" ); 
		frame_queue_flush( m_frame_list );
	}
}


int FGSurface::pull_data(unsigned char **frame, int *size, FskInt64 *time) 
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
    Mutex::Autolock _l(mLock);
	int err = 0;
	
	*frame = NULL;
	*size  = 0;
	*time  = 0;
	
	if( m_frame_list == NULL )
		return -666;
	
	while (true) 
	{
		int frame_total = (int)FskListMutexCount(m_frame_list);
		if(	frame_total	> 0 )
		{
			dlog("SSSSSSSSSSSSSSSSSSSS=> frame_total: %d break!!!\n", frame_total);
			break;
        }
		
		dlog("SSSSSSSSSSSSSSSSSSSS=> calling mCond.wait\n");
        mCond.wait(mLock);
    }
	
	frame_queue_out( m_frame_list, frame, size, time);
	dlog("popped up a frame, time: %d\n", (int)*time);
	
	return err;
}


void FGSurface::push_data(int size, unsigned char *frame) 
{
	int err = 0;
	
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
    Mutex::Autolock _l(mLock);
    
	push_data_0(m_refcon, m_frame_list, size, frame);
	
bail:	
    dlog("SSSSSSSSSSSSSSSSSSSS=> out of %s and signaling\n", __func__);
    mCond.signal();
	
}


status_t FGSurface::registerBuffers(const BufferHeap& buffers) 
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
	
	m_buf = buffers;
	
    dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.w: %d\n", m_buf.w);
    dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.h: %d\n", m_buf.h);
    dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.hor_stride: %d\n", m_buf.hor_stride);
    dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.ver_stride: %d\n", m_buf.ver_stride);
    dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.format: %d\n",		(int)m_buf.format);
    dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.transform: %d\n",	(int)m_buf.transform);
    dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.flags: %d\n",	(int)m_buf.flags);
	
	// dlog("m_buf.heap: %x\n",	(int)m_buf.heap);
	if( m_buf.heap != NULL )
	{
		int32_t id    = m_buf.heap->heapID();
		void*   addr  = m_buf.heap->base();
		size_t  vsize = m_buf.heap->virtualSize();
		dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.heap available:\n");
		dlog("SSSSSSSSSSSSSSSSSSSS=> id: %d\n",		(int)id);
		dlog("SSSSSSSSSSSSSSSSSSSS=> addr: %x\n",	(int)addr);
		dlog("SSSSSSSSSSSSSSSSSSSS=> vsize: %d\n",	(int)vsize);
	}
	
    Mutex::Autolock _l(mLock);
    ++registerBuffersCount;
    dlog("SSSSSSSSSSSSSSSSSSSS=> registerBuffersCount: %d, singalling\n", registerBuffersCount);
    mCond.signal();
    return NO_ERROR;
}


void FGSurface::unregisterBuffers() 
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
    Mutex::Autolock _l(mLock);
    ++unregisterBuffersCount;
    dlog("SSSSSSSSSSSSSSSSSSSS=> unregisterBuffersCount: %d, singalling\n", unregisterBuffersCount);
    mCond.signal();
}

sp<GraphicBuffer> FGSurface::requestBuffer(int bufferIdx, int usage) 
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\b", __func__);
    return NULL;
}


sp<GraphicBuffer> FGSurface::requestBuffer(int a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
    return NULL;
}


status_t FGSurface::setBufferCount(int bufferCount) 
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
    return NULL;
}


void FGSurface::waitUntil(int c0, int c1, int c2) 
{
    dlog("SSSSSSSSSSSSSSSSSSSS=> into FGSurface::waitUntil: %d %d %d\n", c0, c1, c2);
    dlog("SSSSSSSSSSSSSSSSSSSS=> registerBuffersCount: %d, postBufferCount: %d, unregisterBuffersCount %d\n", 
			registerBuffersCount, postBufferCount, unregisterBuffersCount);
	
    dlog("SSSSSSSSSSSSSSSSSSSS=> calling Mutex::Autolock _l(mLock)\n");
	Mutex::Autolock _l(mLock);
    dlog("SSSSSSSSSSSSSSSSSSSS=> called Mutex::Autolock _l(mLock)\n");
	
	while (true) 
	{
        if(	registerBuffersCount	>= c0 && 
		   postBufferCount			>= c1 &&
		   unregisterBuffersCount	>= c2 ) 
		{
			dlog("SSSSSSSSSSSSSSSSSSSS=> condition met, break!!!\n");
			break;
        }
		
		dlog("SSSSSSSSSSSSSSSSSSSS=> calling mCond.wait\n");
        mCond.wait(mLock);
    }
    dlog("SSSSSSSSSSSSSSSSSSSS=> out of FGSurface::waitUntil\n");
}


void FGSurface::postBuffer(ssize_t offset) 
{
	int	err = 0;
	
    dlog("SSSSSSSSSSSSSSSSSSSS=> %s\n", __func__);
    Mutex::Autolock _l(mLock);
    ++postBufferCount;
    dlog("SSSSSSSSSSSSSSSSSSSS=> postBufferCount: %d, offset: %d\n", postBufferCount, offset);
	
	dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.w: %d\n", m_buf.w);
    dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.h: %d\n", m_buf.h);
    dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.hor_stride: %d\n", m_buf.hor_stride);
    dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.ver_stride: %d\n", m_buf.ver_stride);
    dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.format: %d\n",		(int)m_buf.format);
    dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.transform: %d\n",	(int)m_buf.transform);
    dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.flags: %d\n",	(int)m_buf.flags);
	if( m_buf.heap != NULL )
	{
		int32_t id    = m_buf.heap->heapID();
		void*   addr0  = m_buf.heap->base();
		unsigned char * addr  = (unsigned char*)addr0 + offset;
		size_t  vsize = m_buf.heap->virtualSize();
		int   width = m_buf.w;
		int   height = m_buf.h;
		int   yuv420_size = width * height * 3 / 2;
		
		dlog("SSSSSSSSSSSSSSSSSSSS=> m_buf.heap available:\n");
		dlog("SSSSSSSSSSSSSSSSSSSS=> id: %d\n",		(int)id);
		dlog("SSSSSSSSSSSSSSSSSSSS=> addr: %x\n",	(int)addr);
		dlog("SSSSSSSSSSSSSSSSSSSS=> vsize: %d\n",	(int)vsize);
		
		push_data_0(m_refcon, m_frame_list, yuv420_size, addr);
		
	}	
	
bail:	
    dlog("SSSSSSSSSSSSSSSSSSSS=> out of %s and signaling\n", __func__);
    mCond.signal();
	
}


sp<OverlayRef> FGSurface::createOverlay(uint32_t w, uint32_t h, int32_t format, int32_t orientation) 
{
	// Not implemented.
	dlog("SSSSSSSSSSSSSSSSSSSS=> into FGSurface::createOverlay, not implemented!!!\n");
    //ASSERT(0);
    return NULL;
}

#endif //SUPPORT_F_G_SURFACE_PREVIEW


MCameraClient::MCameraClient(void *refCon, int use_preview_display )
{
    dlog("MCMCMCMCMCMCMCMCMCMCM=> into MCameraClient::MCameraClient(), refCon: %x\n", (int)refCon);
	m_refcon = refCon;
	
	m_use_preview_display = use_preview_display;
#ifdef SUPPORT_F_G_SURFACE_PREVIEW		
	if( m_use_preview_display )
	{
		m_fg_surface = new FGSurface(refCon);
		//dlog( "calling c->setPreviewDisplay()\n");
		//err = state->cam->setPreviewDisplay(state->fg_surface);
	}
#else
	dlog( "Creating a SurfaceTexture\n");
	m_surface_texture = new SurfaceTexture(123);
	m_simple_surface = new SimpleSurface(refCon);
#endif


}	


int MCameraClient::set_preview_display(sp<ICamera>	cam)
{
    int err = 0;
	dlog("MCMCMCMCMCMCMCMCMCMCM=> into MCameraClient::set_preview_display()\n");
#ifdef SUPPORT_F_G_SURFACE_PREVIEW	
	if( m_use_preview_display )
	{
		dlog( "calling c->setPreviewDisplay()\n");
		err = cam->setPreviewDisplay(m_fg_surface);
	}
#elif defined Icecreamsanwich_Build
	dlog( "Setting the preview texture\n");
	err = cam->setPreviewTexture(m_surface_texture);
#else
	dlog( "Setting the preview texture\n");
	err = cam->setPreviewTexture(m_surface_texture->getBufferQueue());
#endif

	return err;
}	


void MCameraClient::clearStat() 
{
	dlog("into MCameraClient::clearStat\n");
    Mutex::Autolock _l(mLock);
    mNotifyCount.clear();
    mDataCount.clear();
    mDataSize.clear();
	
#ifdef SUPPORT_F_G_SURFACE_PREVIEW
	if( m_use_preview_display )
		m_fg_surface->clearStat();
	else
#endif
	m_simple_surface->clearStat();
}

bool MCameraClient::test(OP op, int v1, int v2) 
{
	dlog("into MCameraClient::test\n");
    switch (op) 
	{
        case EQ: 
			dlog("case EQ\n");
			
			return v1 == v2;
        case GT: 
			dlog("case GT\n");
			
			return v1 > v2;
        case LT: 
			dlog("case LT\n");
			
			return v1 < v2;
        case GE: 
			dlog("case GE\n");
			
			return v1 >= v2;
        case LE: 
			dlog("case LE\n");
			
			return v1 <= v2;
        default: 
			dlog("case default\n");
			
			//ASSERT(0); 
			break;
    }
    return false;
}

void MCameraClient::assertTest(OP op, int v1, int v2) 
{
	dlog("into MCameraClient::assertTest\n");
	
    if (!test(op, v1, v2)) 
	{
        dlog("assertTest failed: op=%d, v1=%d, v2=%d\n", op, v1, v2);
		
        //ASSERT(0);
    }
}

void MCameraClient::assertNotify(int32_t msgType, OP op, int count) 
{
	dlog("into MCameraClient::assertNotify\n");
    Mutex::Autolock _l(mLock);
    int v = mNotifyCount.valueFor(msgType);
    assertTest(op, v, count);
}

void MCameraClient::assertData(int32_t msgType, OP op, int count) 
{
	dlog("into MCameraClient::assertData\n");
    Mutex::Autolock _l(mLock);
    int v = mDataCount.valueFor(msgType);
    assertTest(op, v, count);
}

void MCameraClient::assertDataSize(int32_t msgType, OP op, int dataSize) 
{
	dlog("into MCameraClient::assertDataSize\n");
    Mutex::Autolock _l(mLock);
    int v = mDataSize.valueFor(msgType);
    assertTest(op, v, dataSize);
}

void MCameraClient::notifyCallback(int32_t msgType, int32_t ext1, int32_t ext2) 
{
	dlog("into MCameraClient::notifyCallback\n");
    dlog("%s\n", __func__);
    Mutex::Autolock _l(mLock);
    ssize_t i = mNotifyCount.indexOfKey(msgType);
    if (i < 0) 
	{
        mNotifyCount.add(msgType, 1);
    } 
	else 
	{
        ++mNotifyCount.editValueAt(i);
    }
    mCond.signal();
}

#ifdef F_G_Build
void MCameraClient::dataCallback(int32_t msgType, const sp<IMemory>& data) 
#elif defined(Honey_Build)
void MCameraClient::dataCallback(int32_t msgType, const sp<IMemory>& data) 
#else
void MCameraClient::dataCallback(int32_t msgType, const sp<IMemory>& data, camera_frame_metadata_t *metadata) 
#endif
{
	dlog("into MCameraClient::dataCallback, msgType: %d\n", (int)msgType);
    int dataSize;
	void *addr;
	
	if( data == NULL )
	{
		dlog("data==NULL, returning\n");
		return;
	}
	
    dataSize = data->size();
	addr	 = data->pointer();
	
    dlog("data type = %d, size = %d, addr: %x\n", msgType, dataSize, (int)addr);
    Mutex::Autolock _l(mLock);
	ssize_t i = mDataCount.indexOfKey(msgType);
	if( msgType == CAMERA_MSG_COMPRESSED_IMAGE )
	{
		char jpg_path[] = "/sdcard/kinoma_camera_ouput.jpg";
		FILE *f = NULL;
		f = fopen(jpg_path, "wb");
		if( f == NULL )
		{
			dlog( "can't create kinoma camera output jpg file!!! \n");
			return;
		}
		
		fwrite(addr, 1, dataSize, f);
		fclose(f);
	}
	else if (msgType == CAMERA_MSG_PREVIEW_FRAME) 
	{
		dlog( "got preview frame, call surface push_data()\n");
		//CameraAndroid state	= (CameraAndroid)m_refcon;
		if( addr != NULL )
		{
#ifdef SUPPORT_F_G_SURFACE_PREVIEW
			if( m_use_preview_display )
				m_fg_surface->push_data( dataSize, (unsigned char *)addr );
			else
#endif
			m_simple_surface->push_data( dataSize, (unsigned char *)addr );
		}
	}		
	
    if (i < 0) 
	{
        mDataCount.add(msgType, 1);
        mDataSize.add(msgType, dataSize);
    } 
	else 
	{
        ++mDataCount.editValueAt(i);
        mDataSize.editValueAt(i) = dataSize;
    }
	
    mCond.signal();
	
    if (msgType == CAMERA_MSG_VIDEO_FRAME) 
	{
        //ASSERT(mReleaser != NULL);
		if( mReleaser != NULL )
			mReleaser->releaseRecordingFrame(data);
    }
}


void MCameraClient::dataCallbackTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& data) 
{
	dlog("into MCameraClient::dataCallbackTimestamp\n");
#ifdef F_G_Build
    dataCallback(msgType, data);
#elif defined(Honey_Build)
    dataCallback(msgType, data);
#else
    dataCallback(msgType, data, NULL);
#endif	
}

void MCameraClient::waitNotify(int32_t msgType, OP op, int count) 
{
	dlog("into MCameraClient::waitNotify\n");
    dlog("waitNotify: %d, %d, %d\n", msgType, op, count);
    Mutex::Autolock _l(mLock);
    while (true) 
	{
        int v = mNotifyCount.valueFor(msgType);
        if (test(op, v, count)) 
		{
            break;
        }
        mCond.wait(mLock);
    }
}


void MCameraClient::waitData(int32_t msgType, OP op, int count) 
{
	dlog("into MCameraClient::waitData\n");
    dlog("waitData: %d, %d, %d\n", msgType, op, count);
    Mutex::Autolock _l(mLock);
    while (true) 
	{
        int v = mDataCount.valueFor(msgType);
        if (test(op, v, count)) 
		{
            break;
        }
        mCond.wait(mLock);
    }
}


int  MCameraClient::pull_data(unsigned char **frame, int *size, FskInt64 *time)
{
#ifdef SUPPORT_F_G_SURFACE_PREVIEW
	if( m_use_preview_display )
		return m_fg_surface->pull_data(frame, size, time);
	else
#endif
	return m_simple_surface->pull_data(frame, size, time);
}
