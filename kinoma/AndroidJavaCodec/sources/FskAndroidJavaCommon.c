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
#define __FSKIMAGE_PRIV__
#define __FSKBITMAP_PRIV__
#define __FSKTHREAD_PRIV__

#include "Fsk.h"
#include "FskBitmap.h"
#include "FskUtilities.h"
#include "FskImage.h"
#include <jni.h>

static JavaVM *gJavaVM;
jclass gMediaCodecCoreClass;

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
	JNIEnv *env;

	gJavaVM = vm;
	if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_4) != JNI_OK) {
		return -1;
	}

	/* find class MediaCodecCore */
	jclass cls = (*env)->FindClass(env, CLASSNAME);
	gMediaCodecCoreClass = (*env)->NewGlobalRef(env, cls);

	return JNI_VERSION_1_4;
}

void Init_JNI_Env()
{
    int status;
	JNIEnv *env;
	FskThread self = FskThreadGetCurrent();

	if (!self->jniEnv) {
		status = (*gJavaVM)->GetEnv(gJavaVM, (void**)&env, JNI_VERSION_1_4);
		if (status < 0) {
			status = (*gJavaVM)->AttachCurrentThread(gJavaVM, &env, NULL);
			if (status < 0) {
				return;
			}
			self->attachedJava = 1;
		}

	self->jniEnv = (int)env;
	}
}
