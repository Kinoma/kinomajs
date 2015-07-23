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
#define __FSKTHREAD_PRIV__

#include "kpr.h"
#include "kprJNI.h"
#include "kprShell.h"
#include "kprZeroconf.h"
#include "kprZeroconfCommon.h"
#include "kprZeroconfAdvertisement.h"
#include "kprZeroconfBrowser.h"

typedef struct KprZeroconfPlatformAdvertisementStruct KprZeroconfPlatformAdvertisementRecord, *KprZeroconfPlatformAdvertisement;
typedef struct KprZeroconfPlatformBrowserStruct KprZeroconfPlatformBrowserRecord, *KprZeroconfPlatformBrowser;
typedef struct KprZeroconfDisposableStruct KprZeroconfDisposableRecord, *KprZeroconfDisposable;

struct KprZeroconfPlatformAdvertisementStruct {
	Boolean running;
	FskInstrumentedItemDeclaration
};

struct KprZeroconfPlatformBrowserStruct {
	Boolean running;
	FskInstrumentedItemDeclaration
};

struct KprZeroconfDisposableStruct {
	KprZeroconfDisposable next;
	KprZeroconfServiceInfo serviceInfo;
	FskTimeCallBack timer;
	FskInstrumentedItemDeclaration
};

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprZeroconfPlatformAdvertisementInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprZeroconfPlatformAdvertisement", FskInstrumentationOffset(KprZeroconfPlatformAdvertisementRecord), NULL, 0, NULL, NULL, NULL, 0 };
static FskInstrumentedTypeRecord KprZeroconfPlatformBrowserInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprZeroconfPlatformBrowser", FskInstrumentationOffset(KprZeroconfPlatformBrowserRecord), NULL, 0, NULL, NULL, NULL, 0 };
static FskInstrumentedTypeRecord KprZeroconfDisposableInstrumentation = { NULL, sizeof(KprZeroconfDisposableRecord), "KprZeroconfDisposable", FskInstrumentationOffset(KprZeroconfDisposableRecord), NULL, 0, NULL, NULL, NULL, 0 };
#endif

#if 0
#pragma mark - KprZeroconf Java
#endif

KprZeroconfDisposable gKprZeroconfDisposableList = NULL;
void KprZeroconfDisposableDispose(KprZeroconfDisposable self);

void KprZeroconfDisposableCallback(FskTimeCallBack timer UNUSED, const FskTime time UNUSED, void *param)
{
	KprZeroconfDisposable disposable = param;
	KprZeroconfServiceInfo serviceInfo = disposable->serviceInfo;
	disposable->serviceInfo = NULL;
	KprZeroconfBrowserServiceDown(NULL, serviceInfo);
	KprZeroconfDisposableDispose(disposable);
}

void KprZeroconfDisposableDispose(KprZeroconfDisposable self)
{
	if (self) {
		FskListRemove(&gKprZeroconfDisposableList, self);
		KprZeroconfServiceInfoDispose(self->serviceInfo);
		FskTimeCallbackDispose(self->timer);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

Boolean KprZeroconfAndroidRemoveDisposable(char* type, char* name)
{
	KprZeroconfDisposable disposable = gKprZeroconfDisposableList, next;
	Boolean found = false;
	for (disposable = gKprZeroconfDisposableList; disposable; disposable = next) {
		KprZeroconfServiceInfo serviceInfo = disposable->serviceInfo;
		next = disposable->next;
		if (!FskStrCompare(serviceInfo->type, type) && (name ? !FskStrCompare(serviceInfo->name, name) : true)) {
			KprZeroconfDisposableDispose(disposable);
			found = true;
		}
	}
	return found;
}

void KprZeroconfAndroidServiceDown(KprZeroconfServiceInfo serviceInfo)
{
	FskErr err = kFskErrNone;
	KprZeroconfDisposable disposable = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprZeroconfDisposableRecord), &disposable));
	FskTimeCallbackNew(&disposable->timer);
	FskInstrumentedItemNew(disposable, NULL, &KprZeroconfDisposableInstrumentation);
	disposable->serviceInfo = serviceInfo;
	FskListAppend(&gKprZeroconfDisposableList, disposable);
	FskTimeCallbackScheduleFuture(disposable->timer, 2, 0, KprZeroconfDisposableCallback, disposable);
bail:
	if (err)
		KprZeroconfServiceInfoDispose(serviceInfo);
	return;
}

void KprZeroconfAndroidServiceUp(KprZeroconfServiceInfo serviceInfo)
{
	if (KprZeroconfAndroidRemoveDisposable(serviceInfo->type, serviceInfo->name))
		KprZeroconfServiceInfoDispose(serviceInfo);
	else
		KprZeroconfBrowserServiceUp(NULL, serviceInfo);
}

KPR_NATIVE_JAVA(void, KprZeroconfAndroid, serviceRegistered)(JNIEnv* env, jobject thiz, jstring jniType, jstring jniName, int jniPort)
{
	FskErr err = kFskErrNone;
	const char *type = (*env)->GetStringUTFChars(env, jniType, 0);
	const char *name = (*env)->GetStringUTFChars(env, jniName, 0);
	KprZeroconfServiceInfo serviceInfo = NULL;
	KprZeroconfServiceInfoNew(&serviceInfo, type, name, NULL, NULL, jniPort, NULL);
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprZeroconfAdvertisementServiceRegistered, NULL, serviceInfo, NULL, NULL);
bail:
	(*env)->ReleaseStringUTFChars(env, jniName, name);
	(*env)->ReleaseStringUTFChars(env, jniType, type);
}

KPR_NATIVE_JAVA(void, KprZeroconfAndroid, serviceUp)(JNIEnv* env, jobject thiz, jstring jniType, jstring jniName, jstring jniHostname, jstring jniIP, int jniPort)
{
	FskErr err = kFskErrNone;
	const char *type = (*env)->GetStringUTFChars(env, jniType, 0);
	const char *name = (*env)->GetStringUTFChars(env, jniName, 0);
	const char *hostname = (*env)->GetStringUTFChars(env, jniHostname, 0);
	const char *ip = (*env)->GetStringUTFChars(env, jniIP, 0);
	KprZeroconfServiceInfo serviceInfo = NULL;
	KprZeroconfServiceInfoNew(&serviceInfo, type, name, hostname, ip, jniPort, NULL);
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprZeroconfAndroidServiceUp, serviceInfo, NULL, NULL, NULL);
bail:
	(*env)->ReleaseStringUTFChars(env, jniIP, ip);
	(*env)->ReleaseStringUTFChars(env, jniHostname, hostname);
	(*env)->ReleaseStringUTFChars(env, jniName, name);
	(*env)->ReleaseStringUTFChars(env, jniType, type);
}

KPR_NATIVE_JAVA(void, KprZeroconfAndroid, serviceDown)(JNIEnv* env, jobject thiz, jstring jniType, jstring jniName)
{
	FskErr err = kFskErrNone;
	const char *type = (*env)->GetStringUTFChars(env, jniType, 0);
	const char *name = (*env)->GetStringUTFChars(env, jniName, 0);
	KprZeroconfServiceInfo serviceInfo = NULL;
	KprZeroconfServiceInfoNew(&serviceInfo, type, name, NULL, NULL, 0, NULL);
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprZeroconfAndroidServiceDown, serviceInfo, NULL, NULL, NULL);
bail:
	(*env)->ReleaseStringUTFChars(env, jniName, name);
	(*env)->ReleaseStringUTFChars(env, jniType, type);
}

#if 0
#pragma mark - KprZeroconfPlatformAdvertisement
#endif

FskErr KprZeroconfPlatformAdvertisementNew(KprZeroconfAdvertisement self)
{
	FskErr err = kFskErrNone;
	KprZeroconfPlatformAdvertisement advertisement = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprZeroconfPlatformAdvertisementRecord), &advertisement));
	FskInstrumentedItemNew(advertisement, NULL, &KprZeroconfPlatformAdvertisementInstrumentation);

	self->platform = advertisement;
bail:
	if (err)
		KprZeroconfPlatformAdvertisementDispose(self);
	return err;
}

void KprZeroconfPlatformAdvertisementDispose(KprZeroconfAdvertisement self)
{
	KprZeroconfPlatformAdvertisement advertisement = self->platform;
	if (advertisement) {
		KprZeroconfPlatformAdvertisementStop(self);
		FskInstrumentedItemDispose(advertisement);
		FskMemPtrDispose(advertisement);
		self->platform = NULL;
	}
}

FskErr KprZeroconfPlatformAdvertisementStart(KprZeroconfAdvertisement self)
{
	FskErr err = kFskErrNone;
	KprZeroconfPlatformAdvertisement advertisement = self->platform;
	FskThread thread = FskThreadGetCurrent();
	JNIEnv* env = (JNIEnv*)thread->jniEnv;
	jclass jniClass = NULL;
	jobject jniContext = NULL;
	jstring jniServiceType = NULL;
	jstring jniServiceName = NULL;

	if (!advertisement->running) {
		bailIfNULL(env);
		jniServiceType = (*env)->NewStringUTF(env, self->serviceType);
		bailIfNULL(jniServiceType);
		jniServiceName = (*env)->NewStringUTF(env, self->serviceName);
		bailIfNULL(jniServiceName);

		bailIfError(KprJNIGetClass(env, &jniClass, "KprZeroconfAndroid"));
		bailIfError(KprJNIGetContext(env, &jniContext));
		bailIfError(KprJNICallStatic(env, NULL, jniClass, "addAdvertisement", "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;I)V", jniContext, jniServiceType, jniServiceName, self->port));
		advertisement->running = true;
	}
bail:
	if (jniServiceType)
		(*env)->DeleteLocalRef(env, jniServiceType);;
	if (jniServiceName)
		(*env)->DeleteLocalRef(env, jniServiceName);;
	return err;
}

FskErr KprZeroconfPlatformAdvertisementStop(KprZeroconfAdvertisement self)
{
	FskErr err = kFskErrNone;
	KprZeroconfPlatformAdvertisement advertisement = self->platform;
	FskThread thread = FskThreadGetCurrent();
	JNIEnv* env = (JNIEnv*)thread->jniEnv;
	jclass jniClass = NULL;
	jobject jniContext = NULL;
	jstring jniServiceType = NULL;
	jstring jniServiceName = NULL;

	if (advertisement->running) {
		bailIfNULL(env);
		jniServiceType = (*env)->NewStringUTF(env, self->serviceType);
		bailIfNULL(jniServiceType);
		jniServiceName = (*env)->NewStringUTF(env, self->serviceName);
		bailIfNULL(jniServiceName);

		bailIfError(KprJNIGetClass(env, &jniClass, "KprZeroconfAndroid"));
		bailIfError(KprJNIGetContext(env, &jniContext));
		bailIfError(KprJNICallStatic(env, NULL, jniClass, "removeAdvertisement", "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;I)V", jniContext, jniServiceType, jniServiceName, self->port));
		advertisement->running = false;
	}
bail:
	if (jniServiceType)
		(*env)->DeleteLocalRef(env, jniServiceType);;
	if (jniServiceName)
		(*env)->DeleteLocalRef(env, jniServiceName);;
	return err;
}

#if 0
#pragma mark - KprZeroconfPlatformBrowser
#endif

FskErr KprZeroconfPlatformBrowserNew(KprZeroconfBrowser self)
{
	FskErr err = kFskErrNone;
	KprZeroconfPlatformBrowser browser = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprZeroconfPlatformBrowserRecord), &browser));
	FskInstrumentedItemNew(browser, NULL, &KprZeroconfPlatformBrowserInstrumentation);
	self->platform = browser;
bail:
	if (err)
		KprZeroconfPlatformBrowserDispose(self);
	return err;
}

void KprZeroconfPlatformBrowserDispose(KprZeroconfBrowser self)
{
	KprZeroconfPlatformBrowser browser = self->platform;
	if (browser) {
		KprZeroconfPlatformBrowserStop(self);
		FskInstrumentedItemDispose(browser);
		FskMemPtrDispose(browser);
		self->platform = NULL;
	}
}

FskErr KprZeroconfPlatformBrowserStart(KprZeroconfBrowser self)
{
	FskErr err = kFskErrNone;
	KprZeroconfPlatformBrowser browser = self->platform;
	FskThread thread = FskThreadGetCurrent();
	JNIEnv* env = (JNIEnv*)thread->jniEnv;
	jclass jniClass = NULL;
	jobject jniContext = NULL;
	jstring jniServiceType = NULL;

	if (!browser->running) {
		bailIfNULL(env);
		jniServiceType = (*env)->NewStringUTF(env, self->serviceType);
		bailIfNULL(jniServiceType);

		bailIfError(KprJNIGetClass(env, &jniClass, "KprZeroconfAndroid"));
		bailIfError(KprJNIGetContext(env, &jniContext));
		bailIfError(KprJNICallStatic(env, NULL, jniClass, "addService", "(Landroid/content/Context;Ljava/lang/String;)V", jniContext, jniServiceType));
		browser->running = true;
	}
bail:
	if (jniServiceType)
		(*env)->DeleteLocalRef(env, jniServiceType);;
	return err;
}

FskErr KprZeroconfPlatformBrowserStop(KprZeroconfBrowser self)
{
	FskErr err = kFskErrNone;
	KprZeroconfPlatformBrowser browser = self->platform;
	FskThread thread = FskThreadGetCurrent();
	JNIEnv* env = (JNIEnv*)thread->jniEnv;
	jclass jniClass = NULL;
	jobject jniContext = NULL;
	jstring jniServiceType = NULL;

	if (browser->running) {
		bailIfNULL(env);
		jniServiceType = (*env)->NewStringUTF(env, self->serviceType);
		bailIfNULL(jniServiceType);
		KprZeroconfAndroidRemoveDisposable(self->serviceType, NULL);
		bailIfError(KprJNIGetClass(env, &jniClass, "KprZeroconfAndroid"));
		bailIfError(KprJNIGetContext(env, &jniContext));
		bailIfError(KprJNICallStatic(env, NULL, jniClass, "removeService", "(Landroid/content/Context;Ljava/lang/String;)V", jniContext, jniServiceType));
		browser->running = false;
	}
bail:
	if (jniServiceType)
		(*env)->DeleteLocalRef(env, jniServiceType);;
	return err;
}
