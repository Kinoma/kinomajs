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

#include "FskECMAScript.h"
#include "FskMediaReader.h"
#include "FskHardware.h"
#include "FskArch.h"
#include <dlfcn.h>

#include "FskPlatformImplementation.h"
FskInstrumentedSimpleType(CameraAndroidExtension, CameraAndroidExtension);
#define mlog  FskCameraAndroidExtensionPrintfMinimal
#define nlog  FskCameraAndroidExtensionPrintfNormal
#define vlog  FskCameraAndroidExtensionPrintfVerbose
#define dlog  FskCameraAndroidExtensionPrintfDebug

extern FskMediaReaderDispatchRecord gCameraAVF;

FskMediaReaderDispatchRecord gCameraAndroid_func[] =
{
	{
		NULL,//cameraAndroidCanHandle, 
		NULL,//cameraAndroidNew, 
		NULL,//cameraAndroidDispose, 
		NULL,//cameraAndroidGetTrack, 
		NULL,//cameraAndroidStart, 
		NULL,//cameraAndroidStop, 
		NULL,//cameraAndroidExtract, 
		NULL,//cameraAndroidGetMetadata, 
		NULL,//cameraAndroidProperties, 
		NULL //cameraAndroidSniff
	}
	,{NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
};

typedef Boolean	(*FskMediaReaderCanHandle_type)(const char *mimeType);
typedef FskErr	(*FskMediaReaderNew_type)(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
typedef FskErr	(*FskMediaReaderDispose_type)(FskMediaReader reader, void *readerState);
typedef FskErr	(*FskMediaReaderGetTrack_type)(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
typedef FskErr	(*FskMediaReaderStart_type)(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
typedef FskErr	(*FskMediaReaderStop_type)(FskMediaReader reader, void *readerState);
typedef FskErr	(*FskMediaReaderExtract_type)(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
typedef FskErr	(*FskMediaReaderGetMetadata_type)(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
typedef FskErr	(*FskMediaReaderSniff_type)(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

FskMediaPropertyEntry	(*cameraAndroidGetProperties_func)()=NULL;

Boolean		(*cameraAndroidCanHandle_func)(const char *mimeType)=NULL;
FskErr		(*cameraAndroidNew_func)(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source)=NULL;
FskErr		(*cameraAndroidDispose_func)(FskMediaReader reader, void *readerState)=NULL;
FskErr		(*cameraAndroidGetTrack_func)(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track)=NULL;
FskErr		(*cameraAndroidStart_func)(FskMediaReader reader, void *readerState, double *startTime, double *endTime)=NULL;
FskErr		(*cameraAndroidStop_func)(FskMediaReader reader, void *readerState)=NULL;
FskErr		(*cameraAndroidExtract_func)(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data)=NULL;
FskErr		(*cameraAndroidGetMetadata_func)(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)=NULL;
FskErr		(*cameraAndroidSniff_func)(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)=NULL;

static void *lib_handle = NULL;
static char	*modelName  = NULL;
static char	*osVersion  = NULL; 

int init_lib()
{
	FskErr err = kFskErrNone;
	
	dlog( "into init_lib, "); 
	if( lib_handle == NULL )
	{
		int android_version = 0;
        int support_camera_java_api = 0;
		
		ANDROID_GET_VERSION(modelName, osVersion, android_version, err);
		BAIL_IF_ERR(err);
        
		//if( android_version >= ANDROID_JELLY )
            support_camera_java_api = 1;
        
        if( support_camera_java_api )
        {
            dlog( "loading libFskCameraAndroid_java.so");
            lib_handle = dlopen("libFskCameraAndroid_java.so", RTLD_NOW);
            if( lib_handle == NULL ) {
                dlog( "loading libFskCameraAndroid_java.so failed, err:%s", dlerror()); 
                err = kFskErrUnimplemented;
                BAIL_IF_ERR(err);
            }
        }
        else
        {
            if( android_version == ANDROID_FROYO )
            {
                dlog( "loading libFskCameraAndroid_F.so, "); 
                lib_handle = dlopen("libFskCameraAndroid_F.so", RTLD_NOW);
            }
            else if( android_version == ANDROID_GINGER )
            {
                dlog( "loading libFskCameraAndroid_G.so, "); 
                lib_handle = dlopen("libFskCameraAndroid_G.so", RTLD_NOW);
            }
            else if( android_version == ANDROID_HONEY )
            {
                dlog( "loading libFskCameraAndroid_H.so, "); 
                lib_handle = dlopen("libFskCameraAndroid_H.so", RTLD_NOW);
            }
            else if( android_version == ANDROID_ICE )
            {
                dlog( "loading libFskCameraAndroid_I.so, "); 
                lib_handle = dlopen("libFskCameraAndroid_I.so", RTLD_NOW);
            }
            else if( android_version == ANDROID_JELLY )
            {
                dlog( "loading libFskCameraAndroid_J.so, "); 
                lib_handle = dlopen("libFskCameraAndroid_J.so", RTLD_NOW);
            }
            else
            {
                dlog("this model is not supported: android_version/modelName/osVersion::%d/%s/%s\n", android_version, modelName, osVersion );
            }

            if (lib_handle == NULL)
            {
                dlog( "failed\n"); 
                err = kFskErrUnimplemented;
            }	
            else
            {
                dlog( "succeeded\n"); 
            }	
            BAIL_IF_ERR(err );
        }
	}
	
	//video
	ANDROID_LOAD_FUNC( lib_handle, "cameraGenericGetProperties",	cameraAndroidGetProperties_func, 1 );
	
	ANDROID_LOAD_FUNC( lib_handle, "cameraGenericCanHandle",		cameraAndroidCanHandle_func, 1 );
	ANDROID_LOAD_FUNC( lib_handle, "cameraGenericNew",				cameraAndroidNew_func, 1 );
	ANDROID_LOAD_FUNC( lib_handle, "cameraGenericDispose",			cameraAndroidDispose_func, 1 );
	ANDROID_LOAD_FUNC( lib_handle, "cameraGenericGetTrack",			cameraAndroidGetTrack_func, 1 );
	ANDROID_LOAD_FUNC( lib_handle, "cameraGenericStart",			cameraAndroidStart_func, 1 );
	ANDROID_LOAD_FUNC( lib_handle, "cameraGenericStop",				cameraAndroidStop_func, 1 );
	ANDROID_LOAD_FUNC( lib_handle, "cameraGenericExtract",			cameraAndroidExtract_func, 1 );
	ANDROID_LOAD_FUNC( lib_handle, "cameraGenericGetMetadata",		cameraAndroidGetMetadata_func, 1 );
	ANDROID_LOAD_FUNC( lib_handle, "cameraGenericSniff",			cameraAndroidSniff_func, 1 );
	
	gCameraAndroid_func[0].doCanHandle	= (FskMediaReaderCanHandle_type)cameraAndroidCanHandle_func;
	gCameraAndroid_func[0].doNew			= (FskMediaReaderNew_type)cameraAndroidNew_func;
	gCameraAndroid_func[0].doDispose		= (FskMediaReaderDispose_type)cameraAndroidDispose_func;
	gCameraAndroid_func[0].doGetTrack	= (FskMediaReaderGetTrack_type)cameraAndroidGetTrack_func;
	gCameraAndroid_func[0].doStart		= (FskMediaReaderStart_type)cameraAndroidStart_func;
	gCameraAndroid_func[0].doStop		= (FskMediaReaderStop_type)cameraAndroidStop_func;
	gCameraAndroid_func[0].doExtract		= (FskMediaReaderExtract_type)cameraAndroidExtract_func;
	gCameraAndroid_func[0].doGetMetadata	= (FskMediaReaderGetMetadata_type)cameraAndroidGetMetadata_func;
	
	gCameraAndroid_func[0].properties	= (FskMediaPropertyEntry)cameraAndroidGetProperties_func();
	
	gCameraAndroid_func[0].doSniff		= (FskMediaReaderSniff_type)cameraAndroidSniff_func;
	
bail:
	return err;
}


FskExport(FskErr) FskCameraAndroid_fskLoad(FskLibrary library)
{
	FskErr err = kFskErrNone;
	
	dlog( "into FskCameraAndroid_fskLoad()\n"); 
	err = init_lib();
	if (err) goto bail;

	FskMediaReaderInstall(&gCameraAndroid_func);
	
bail:
	dlog( "out of FskCameraAndroid_fskLoad()\n"); 
	return err;
}

FskExport(FskErr) FskCameraAndroid_fskUnload(FskLibrary library)
{
	dlog( "into FskCameraAndroid_fskUnload()\n"); 
	FskMediaReaderUninstall(&gCameraAndroid_func);
	
	if( lib_handle != NULL )
	{
		dlclose( lib_handle );
		lib_handle = NULL;
	}
	
	dlog( "out of FskCameraAndroid_fskUnload()\n"); 
	return kFskErrNone;
}
