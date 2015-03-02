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
#ifndef __KINOMA_ARCH_H__
#define __KINOMA_ARCH_H__

#include "Fsk.h"

#define FSK_ARCH_AUTO		0
#define FSK_ARCH_C			1
#define FSK_ARCH_ARM_V4		2
#define FSK_ARCH_ARM_V5		3
#define FSK_ARCH_XSCALE		4
#define FSK_ARCH_ARM_V6		5
#define FSK_ARCH_ARM_V7		6

#define ANDROID_ECLAIR		21
#define ANDROID_FROYO		22
#define ANDROID_GINGER		23
#define ANDROID_HONEY		30
#define ANDROID_ICE			40
#define ANDROID_JELLY		41
#define ANDROID_JELLY2		42
#define ANDROID_JELLY3		43
#define ANDROID_KITKAT		44
#define ANDROID_LOLLIPOP	50

#define SUPPORT_STAGEFRIGHT(android_version)	(android_version >= ANDROID_FROYO)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GET_ARM_CPU
FskAPI(int) FskHardwareGetARMCPU(void);
#endif

FskAPI(int)		FskHardwareGetARMCPU_All(void);
FskAPI(FskErr)	FskHardwareForceARMCPU(int arch);	//!!!before setting it, make sure the devie you are running on supports this architecture
FskAPI(int)		FskHardwareGetForcedARMCPU(void);

#define ANDROID_GET_VERSION(modelName, osVersion, android_version, err)								\
if( modelName == NULL || osVersion == NULL || android_version == 0 )								\
{																									\
	gAndroidCallbacks->getModelInfoCB(&modelName, &osVersion, NULL, NULL, NULL);							\
	{																								\
		dlog("from gAndroidCallbacks->GetModelInfom, modelName: %s, osVersion: %s\n", modelName,  osVersion );	\
	}																								\
																									\
	{																								\
		int is_dkbtd = !strcmp(modelName, "dkbtd");													\
		int ver_hi = osVersion[8]  - '0';															\
		int ver_lo = osVersion[10] - '0';															\
																									\
		if(	ver_hi==2 && ver_lo==1 )																\
		{																							\
			android_version = ANDROID_ECLAIR;														\
			dlog("android version detected: ANDROID_ECLAIR\n" );									\
		}																							\
		else if( ver_hi==2 && ver_lo==2 )															\
		{																							\
			android_version = ANDROID_FROYO;														\
			dlog("android version detected: ANDROID_FROYO\n" );									\
		}																							\
		else if( ver_hi==2 && ver_lo==3 )															\
		{																							\
			android_version = ANDROID_GINGER;														\
			dlog("android version detected: ANDROID_GINGER\n" );									\
		}																							\
		else if( ver_hi==3 )																		\
		{																							\
			android_version = ANDROID_HONEY;														\
			dlog("android version detected: ANDROID_HONEY\n" );									\
		}																							\
		else if( ver_hi==4 && ver_lo==0  )															\
		{																							\
			android_version = ANDROID_ICE;															\
			dlog("android version detected: ANDROID_ICE\n" );									\
		}																							\
		else if( ver_hi==4 && ver_lo==1  )															\
		{																							\
			android_version = ANDROID_JELLY;														\
			dlog("android version detected: ANDROID_JELLY\n" );									\
		}																							\
		else if( ver_hi==4 && ver_lo==2  )															\
		{																							\
			android_version = ANDROID_JELLY2;														\
			dlog("android version detected: ANDROID_JELLY2\n" );									\
		}																							\
		else if( ver_hi==4 && ver_lo==3  )															\
		{																							\
			android_version = ANDROID_JELLY3;														\
			dlog("android version detected: ANDROID_JELLY3\n" );									\
		}																							\
        else if( ver_hi==4 && ver_lo==4  )															\
        {																							\
            android_version = ANDROID_KITKAT;														\
            dlog("android version detected: ANDROID_KITKAT\n" );									\
        }																							\
        else if( ver_hi==5 && ver_lo==0  )															\
        {																							\
            android_version = ANDROID_LOLLIPOP;														\
            dlog("android version detected: ANDROID_LOLLIPOP\n" );									\
        }																							\
		else																						\
		{																							\
			err = kFskErrUnimplemented;																\
			dlog("this model is not supported: is_dkbtd/ver_hi/ver_lo::%d/%d/%d\n", is_dkbtd, ver_hi, ver_lo );	\
		}																							\
	}																								\
}


#define ANDROID_LOAD_FUNC( lib_handle, name, func, bail_when_fail )		\
if( func == NULL )											\
{															\
	dlog( "loading func name: %s, ", name);				\
	func	= dlsym( lib_handle, name);						\
	if( func == NULL )										\
	{														\
		dlog( "failed\n");								\
		if( bail_when_fail )								\
		{													\
			BAIL( kFskErrNotFound );						\
		}													\
		else												\
		{													\
			dlog( "failed on this platform, but ok\n");	\
			err = kFskErrNone;								\
		}													\
	}														\
	else													\
	{														\
		dlog( "succeeded\n");							\
	}														\
}




#ifdef __cplusplus
}
#endif



#endif	//__KINOMA_IPP_LIB_H__

