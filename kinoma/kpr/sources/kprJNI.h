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
#ifndef __KPRJNI__
#define __KPRJNI__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if TARGET_OS_ANDROID

#include "jni.h"

#define KPR_NATIVE_JAVA_PASTER(jniType, jniNamespace, jniClass, jniMethod) JNIEXPORT jniType JNICALL Java_ ## jniNamespace ## _ ## jniClass ## _ ## jniMethod
#define KPR_NATIVE_JAVA_EVALUATOR(jniType, jniNamespace, jniClass, jniMethod)  KPR_NATIVE_JAVA_PASTER(jniType, jniNamespace, jniClass, jniMethod)
#define KPR_NATIVE_JAVA(jniType, jniClass, jniMethod) KPR_NATIVE_JAVA_EVALUATOR(jniType, KPR_NATIVE_JAVA_NAMESPACE, jniClass, jniMethod)

FskErr KprJNICall(JNIEnv *env, jvalue* jniValue, jobject jniObject, const char* name, const char* signature, ...);
FskErr KprJNICallStatic(JNIEnv *env, jvalue* jniValue, jclass jniClass, const char* name, const char* signature, ...);
FskErr KprJNIGetClass(JNIEnv* env, jclass* jniClass, char* name);
FskErr KprJNIGetContext(JNIEnv* env, jobject* jniContext);
FskErr KprJNIGetEnv(JNIEnv** env);
FskErr KprJNIGetMethod(JNIEnv* env, jmethodID* jniMethod, jclass jniClass, const char* name, const char* signature);
FskErr KprJNIGetStaticMethod(JNIEnv* env, jmethodID* jniMethod, jclass jniClass, const char* name, const char* signature);
FskErr KprJNINew(JNIEnv* env, jobject* jniObject, jclass jniClass, const char* signature, ...);

#endif // ANDROID

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // TARGET_OS_ANDROID
