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
#define __FSKTHREAD_PRIV__

#include "kpr.h"
#include "kprJNI.h"

#if TARGET_OS_ANDROID

#include "FskPlatformImplementation.h"
FskInstrumentedSimpleType(JNI, KprJNI);
#define KprJNIPrintfMinimal FskJNIPrintfMinimal
#define KprJNIPrintfNormal  FskJNIPrintfNormal
#define KprJNIPrintfVerbose FskJNIPrintfVerbose
#define KprJNIPrintfDebug   FskJNIPrintfDebug

FskErr KprJNICall(JNIEnv *env, jvalue* jniValue, jobject jniObject, const char* name, const char* signature, ...)
{
	FskErr err = kFskErrNone;
	va_list args;
	const char *p = FskStrChr(signature, ')');

	bailIfNULL(p);
	va_start(args, signature);
	if ((*env)->EnsureLocalCapacity(env, 2) == JNI_OK) {
		jclass jniClass = (*env)->GetObjectClass(env, jniObject);
		jmethodID jniMethod = (*env)->GetMethodID(env, jniClass, name, signature);
		if (jniMethod) {
			p++;
			switch (*p) {
				case 'V':
					(*env)->CallVoidMethodV(env, jniObject, jniMethod, args);
					break;
				case '[':
				case 'L':
					jniValue->l = (*env)->CallObjectMethodV(env, jniObject, jniMethod, args);
					break;
				case 'Z':
					jniValue->z = (*env)->CallBooleanMethodV(env, jniObject, jniMethod, args);
					break;
				case 'B':
					jniValue->b = (*env)->CallByteMethodV(env, jniObject, jniMethod, args);
					break;
				case 'C':
					jniValue->c = (*env)->CallCharMethodV(env, jniObject, jniMethod, args);
					break;
				case 'S':
					jniValue->s = (*env)->CallShortMethodV(env, jniObject, jniMethod, args);
					break;
				case 'I':
					jniValue->i = (*env)->CallIntMethodV(env, jniObject, jniMethod, args);
					break;
				case 'J':
					jniValue->j = (*env)->CallLongMethodV(env, jniObject, jniMethod, args);
					break;
				case 'F':
					jniValue->f = (*env)->CallFloatMethodV(env, jniObject, jniMethod, args);
					break;
				case 'D':
					jniValue->d = (*env)->CallDoubleMethodV(env, jniObject, jniMethod, args);
					break;
				default:
					(*env)->FatalError(env, "illegal signature");
					break;
			}
		}
		(*env)->DeleteLocalRef(env, jniClass);
	}
	if ((*env)->ExceptionCheck(env)) {
		err = kFskErrOperationFailed;
		(*env)->ExceptionClear(env);
	}
	va_end(args);
bail:
	KprJNIPrintfDebug("%s %s (%d)", __FUNCTION__, name, err);
	return err;
}

FskErr KprJNICallStatic(JNIEnv *env, jvalue* jniValue, jclass jniClass, const char* name, const char* signature, ...)
{
	FskErr err = kFskErrNone;
	va_list args;
	const char *p = FskStrChr(signature, ')');

	bailIfNULL(p);
	va_start(args, signature);
	if ((*env)->EnsureLocalCapacity(env, 1) == JNI_OK) {
		jmethodID jniMethod = (*env)->GetStaticMethodID(env, jniClass, name, signature);
		if (jniMethod) {
			p++;
			switch (*p) {
				case 'V':
					(*env)->CallStaticVoidMethodV(env, jniClass, jniMethod, args);
					break;
				case '[':
				case 'L':
					jniValue->l = (*env)->CallStaticObjectMethodV(env, jniClass, jniMethod, args);
					break;
				case 'Z':
					jniValue->z = (*env)->CallStaticBooleanMethodV(env, jniClass, jniMethod, args);
					break;
				case 'B':
					jniValue->b = (*env)->CallStaticByteMethodV(env, jniClass, jniMethod, args);
					break;
				case 'C':
					jniValue->c = (*env)->CallStaticCharMethodV(env, jniClass, jniMethod, args);
					break;
				case 'S':
					jniValue->s = (*env)->CallStaticShortMethodV(env, jniClass, jniMethod, args);
					break;
				case 'I':
					jniValue->i = (*env)->CallStaticIntMethodV(env, jniClass, jniMethod, args);
					break;
				case 'J':
					jniValue->j = (*env)->CallStaticLongMethodV(env, jniClass, jniMethod, args);
					break;
				case 'F':
					jniValue->f = (*env)->CallStaticFloatMethodV(env, jniClass, jniMethod, args);
					break;
				case 'D':
					jniValue->d = (*env)->CallStaticDoubleMethodV(env, jniClass, jniMethod, args);
					break;
				default:
					(*env)->FatalError(env, "illegal signature");
					break;
			}
		}
	}
	if ((*env)->ExceptionCheck(env)) {
		err = kFskErrOperationFailed;
		(*env)->ExceptionClear(env);
	}
	va_end(args);
bail:
	KprJNIPrintfDebug("%s %s (%d)", __FUNCTION__, name, err);
	return err;
}

FskErr KprJNIGetClass(JNIEnv* env, jclass* jniClass, char* name)
{
	FskErr err = kFskErrNone;
	jmethodID jniMethod = NULL;
	jclass it = NULL;
	jstring jniName = NULL;
	FskThread thread = FskThreadGetCurrent();

	bailIfNULL(((JNIEnv*)thread->jniEnv) == env);
	bailIfNULL(thread->play2AndroidClass);
	bailIfError(KprJNIGetStaticMethod(env, &jniMethod, (jclass)thread->play2AndroidClass, "getClass", "(Ljava/lang/String;)Ljava/lang/Class;"));
	jniName = (*env)->NewStringUTF(env, name);
	bailIfNULL(jniName);
	it = (*env)->CallStaticObjectMethod(env, (jclass)thread->play2AndroidClass, jniMethod, jniName);
	if (it)
		*jniClass = it;
bail:
	if (jniName)
		(*env)->DeleteLocalRef(env, jniName);;
	if ((*env)->ExceptionCheck(env)) {
		err = kFskErrOperationFailed;
		(*env)->ExceptionClear(env);
	}
	KprJNIPrintfDebug("%s %p (%d)", __FUNCTION__, it, err);
	return err;
}

FskErr KprJNIGetContext(JNIEnv* env, jobject* jniContext)
{
	FskErr err = kFskErrNone;
	jmethodID jniMethod = NULL;
	jobject it = NULL;
	FskThread thread = FskThreadGetCurrent();

	bailIfNULL(((JNIEnv*)thread->jniEnv) == env);
	bailIfNULL(thread->play2AndroidClass);
	bailIfError(KprJNIGetStaticMethod(env, &jniMethod, (jclass)thread->play2AndroidClass, "getContext", "()Landroid/content/Context;"));
	it = (*env)->CallStaticObjectMethod(env, (jclass)thread->play2AndroidClass, jniMethod);
	if (it)
		*jniContext = it;
bail:
	if ((*env)->ExceptionCheck(env)) {
		err = kFskErrOperationFailed;
		(*env)->ExceptionClear(env);
	}
	KprJNIPrintfDebug("%s %p (%d)", __FUNCTION__, it, err);
	return err;
}

FskErr KprJNIGetEnv(JNIEnv** env)
{
	FskErr err = kFskErrNone;
	FskThread thread = FskThreadGetCurrent();

	*env = (JNIEnv*)thread->jniEnv;
	bailIfNULL(env);
bail:
	return err;
}

FskErr KprJNIGetMethod(JNIEnv* env, jmethodID* jniMethod, jclass jniClass, const char* name, const char* signature)
{
	FskErr err = kFskErrNone;
	jmethodID it = (*env)->GetMethodID(env, jniClass, name, signature);
	if (it)
		*jniMethod = it;
bail:
	if ((*env)->ExceptionCheck(env)) {
		err = kFskErrOperationFailed;
		(*env)->ExceptionClear(env);
	}
	KprJNIPrintfDebug("%s %p (%d)", __FUNCTION__, it, err);
	return err;
}

FskErr KprJNIGetStaticMethod(JNIEnv* env, jmethodID* jniMethod, jclass jniClass, const char* name, const char* signature)
{
	FskErr err = kFskErrNone;
	jmethodID it = (*env)->GetStaticMethodID(env, jniClass, name, signature);
	if (it)
		*jniMethod = it;
bail:
	if ((*env)->ExceptionCheck(env)) {
		err = kFskErrOperationFailed;
		(*env)->ExceptionClear(env);
	}
	KprJNIPrintfDebug("%s %p (%d)", __FUNCTION__, it, err);
	return err;
}

FskErr KprJNINew(JNIEnv* env, jobject* jniObject, jclass jniClass, const char* signature, ...)
{
	FskErr err = kFskErrNone;
	jmethodID jniMethod = NULL;
	va_list args;
	jobject it = NULL;

	bailIfError(KprJNIGetMethod(env, &jniMethod, jniClass, "<init>", signature));
	va_start(args, signature);
	it = (*env)->NewObjectV(env, jniClass, jniMethod, args);
	if (it)
		*jniObject = it;
bail:
	if ((*env)->ExceptionCheck(env)) {
		err = kFskErrOperationFailed;
		(*env)->ExceptionClear(env);
	}
	va_end(args);
	KprJNIPrintfDebug("%s %p (%d)", __FUNCTION__, it, err);
	return err;
}

#endif // TARGET_OS_ANDROID
