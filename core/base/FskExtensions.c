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
#define __FSKEXTENSIONS_PRIV__
#define __FSKTHREAD_PRIV__
#include "FskExtensions.h"
#include "FskFiles.h"
#include "FskThread.h"
#include "FskUtilities.h"
#include "FskHardware.h"

static FskMutex gExtensionsMutex;

FskInstrumentedSimpleType(Extensions, extensions);

FskErr FskExtensionInstall(UInt32 extensionType, void *value)
{
	FskErr err;
	FskThread thread = FskThreadGetCurrent();

	if ((0 == extensionType) || (NULL == thread))
		return kFskErrInvalidParameter;

	FskMutexAcquire(gExtensionsMutex);

	// make sure we have an entry for this extension type
	if (thread->extensionsTypeCount <= extensionType) {
		UInt32 i;

		err = FskMemPtrRealloc((extensionType + 1) * sizeof(FskExtensionTypeRecord), (FskMemPtr*)(void*)&thread->extensions);
		BAIL_IF_ERR(err);

		for (i=thread->extensionsTypeCount; i<=extensionType; i++) {
			((FskExtensionType)thread->extensions)[i].count = 0;
			((FskExtensionType)thread->extensions)[i].list = NULL;
		}

		thread->extensionsTypeCount = extensionType + 1;
	}

	// add this item
	err = FskMemPtrRealloc((((FskExtensionType)thread->extensions)[extensionType].count + 1) * sizeof(void*), (FskMemPtr*)(void*)&((FskExtensionType)thread->extensions)[extensionType].list);
	BAIL_IF_ERR(err);

	((FskExtensionType)thread->extensions)[extensionType].list[((FskExtensionType)thread->extensions)[extensionType].count++] = value;

bail:
	FskMutexRelease(gExtensionsMutex);

	return err;
}

FskErr FskExtensionUninstall(UInt32 extensionType, void *value)
{
	FskErr err = kFskErrInvalidParameter;
	FskThread thread = FskThreadGetCurrent();
	Boolean found;
	UInt32 i;

	FskMutexAcquire(gExtensionsMutex);

	if ((NULL == thread) || (extensionType >= thread->extensionsTypeCount) || (0 == extensionType))
		goto bail;

	for (i=0, found = false; i<((FskExtensionType)thread->extensions)[extensionType].count; i++) {
		if (!found)
			found = ((FskExtensionType)thread->extensions)[extensionType].list[i] == value;
		else
			((FskExtensionType)thread->extensions)[extensionType].list[i - 1] = ((FskExtensionType)thread->extensions)[extensionType].list[i];
	}

	if (found) {
		((FskExtensionType)thread->extensions)[extensionType].count -= 1;
		err = FskMemPtrRealloc(((FskExtensionType)thread->extensions)[extensionType].count * sizeof(void*), (FskMemPtr*)(void*)&((FskExtensionType)thread->extensions)[extensionType].list);
	}

bail:
	FskMutexRelease(gExtensionsMutex);

	return err;
}

void *FskExtensionGetByIndex(UInt32 extensionType, UInt32 index)
{
	void *result = NULL;
	FskThread thread;
	SInt32	totalCount;

	// if extensions aren't initialized yet, then there can be no get.
	// (early case of FskFileOpen)
	if ((NULL == gExtensionsMutex) || (0 == extensionType))
		return NULL;

	thread = FskThreadGetCurrent();
	if (NULL == thread) {
		thread = FskThreadGetMain();
		if (NULL == thread)
			return NULL;
	}

	FskMutexAcquire(gExtensionsMutex);

	if ((extensionType < thread->extensionsTypeCount) && (index < ((FskExtensionType)thread->extensions)[extensionType].count)) {
		totalCount = ((FskExtensionType)thread->extensions)[extensionType].count;
		result = ((FskExtensionType)thread->extensions)[extensionType].list[totalCount - 1 - index];
	}
	else
	if (0 == (kFskThreadFlagsIsMain & thread->flags)) {
		if (extensionType < thread->extensionsTypeCount)
			index -= ((FskExtensionType)thread->extensions)[extensionType].count;
		thread = FskThreadGetMain();
		if ((extensionType < thread->extensionsTypeCount) && (index < ((FskExtensionType)thread->extensions)[extensionType].count)) {
			totalCount = ((FskExtensionType)thread->extensions)[extensionType].count;
			result = ((FskExtensionType)thread->extensions)[extensionType].list[totalCount -1 - index];
		}
	}

	FskMutexRelease(gExtensionsMutex);

	return result;
}

UInt32 FskExtensionGetCount(UInt32 extensionType)
{
	FskThread thread;
	UInt32 count = 0;

	if ((NULL == gExtensionsMutex) || (0 == extensionType))
		goto bail;

	thread = FskThreadGetCurrent();
	if (NULL == thread) {
		thread = FskThreadGetMain();
		if (NULL == thread)
			goto bail;
	}

	FskMutexAcquire(gExtensionsMutex);

	if (extensionType < thread->extensionsTypeCount)
		count = ((FskExtensionType)thread->extensions)[extensionType].count;

	if (0 == (kFskThreadFlagsIsMain & thread->flags)) {
		thread = FskThreadGetMain();
		if (extensionType < thread->extensionsTypeCount)
			count += ((FskExtensionType)thread->extensions)[extensionType].count;
	}

	FskMutexRelease(gExtensionsMutex);

bail:
	return count;
}

FskErr FskExtensionsInitialize(void)
{
	return FskMutexNew(&gExtensionsMutex, "gExtensionsMutex");
}

void FskExtensionsTerminateThread(FskThread thread)
{
	UInt32 i;

	for (i = 0; i < thread->extensionsTypeCount; i++)
		FskMemPtrDispose(((FskExtensionType)thread->extensions)[i].list);

	FskMemPtrDisposeAt((void **)&thread->extensions);
	thread->extensionsTypeCount = 0;
}

void FskExtensionsTerminate(void)
{
	FskMutexDispose(gExtensionsMutex);
	gExtensionsMutex = NULL;
}

#if TARGET_OS_WIN32

FskErr FskLibraryLoad(FskLibrary *libraryOut, const char *path)
{
	FskErr err;
	FskLibrary library = NULL;
	wchar_t *nativePath = NULL;

	err = FskMemPtrNewClear(sizeof(FskLibraryRecord), (FskMemPtr *)&library);
	BAIL_IF_ERR(err);

	err = FskFilePathToNative(path, (char **)&nativePath);
	BAIL_IF_ERR(err);

	library->module = LoadLibraryW(nativePath);
	BAIL_IF_NULL(library->module, err, kFskErrOperationFailed);

bail:
	FskMemPtrDispose(nativePath);

	if (err) {
		FskLibraryUnload(library);
		library = NULL;
	}
	*libraryOut = library;
	return err;
}

FskErr FskLibraryUnload(FskLibrary library)
{
	if (library) {
		if (library->module)
			FreeLibrary(library->module);
		FskMemPtrDispose(library);
	}
	return kFskErrNone;
}

FskErr FskLibraryGetSymbolAddress_(FskLibrary library, const char *symbol, void **address)
{
	*address = GetProcAddress(library->module, symbol);

	return (NULL == *address) ? kFskErrUnknownElement : kFskErrNone;
}

#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
#include <mach-o/dyld.h>
#include <dlfcn.h>

FskErr FskLibraryLoad(FskLibrary *libraryOut, const char *path)
{
	FskErr err;
	FskLibrary library = NULL;
	char *nativePath = NULL;

	err = FskMemPtrNewClear(sizeof(FskLibraryRecord), (FskMemPtr *)&library);
	BAIL_IF_ERR(err);

	err = FskFilePathToNative(path, &nativePath);
	BAIL_IF_ERR(err);

    library->module = dlopen(nativePath, RTLD_NOW);
	if (NULL == library->module) {
		FskExtensionsPrintfDebug("FskLibraryLoad %s failed %s\n", path, dlerror());
		err = kFskErrOperationFailed;
	}
    
	BAIL_IF_NULL(library->module, err, kFskErrOperationFailed);

bail:
	FskMemPtrDispose(nativePath);

	if (err) {
		FskLibraryUnload(library);
		library = NULL;
	}
	*libraryOut = library;
	return err;
}

FskErr FskLibraryUnload(FskLibrary library)
{
	if (library) {
// closing library leads to intermittent crash on exit...
//		if (library->module)
//            dlclose(library->module);

		FskMemPtrDispose(library);
	}

	return kFskErrNone;
}

FskErr FskLibraryGetSymbolAddress_(FskLibrary library, const char *symbol, void **address)
{
	*address = dlsym(library->module, symbol);
	if (NULL == *address)
		return kFskErrUnknownElement;
	return kFskErrNone;
}

#elif TARGET_OS_LINUX || TARGET_OS_IPHONE
#include <dlfcn.h>

FskErr FskLibraryLoad(FskLibrary *libraryOut, const char *path)
{
	FskErr err;
	FskLibrary library = NULL;
	char *nativePath = NULL;

	err = FskMemPtrNewClear(sizeof(FskLibraryRecord), (FskMemPtr*)(void*)&library);
	BAIL_IF_ERR(err);

	err = FskFilePathToNative(path, &nativePath);
	BAIL_IF_ERR(err);

	library->module = dlopen(nativePath, RTLD_NOW);
	if (NULL == library->module) {
		FskExtensionsPrintfDebug("FskLibraryLoad %s failed %s\n", path, dlerror());
		err = kFskErrOperationFailed;
	}

bail:

	FskMemPtrDispose(nativePath);

	if (err) {
		FskLibraryUnload(library);
		library = NULL;
	}
	*libraryOut = library;
	return err;
}

FskErr FskLibraryUnload(FskLibrary library)
{
	if (library) {
		FskExtensionsPrintfDebug("FskLibraryUnload %x\n", library->module);
		if (library->module) {
			dlclose(library->module);
		}
		FskMemPtrDispose(library);
	}
	return kFskErrNone;
}

FskErr FskLibraryGetSymbolAddress_(FskLibrary library, const char *symbol, void **address)
{
	*address = dlsym(library->module, symbol);
	return (NULL == *address) ? kFskErrUnknownElement : kFskErrNone;
}

#elif TARGET_OS_KPL

#include "KplExtensions.h"

FskErr FskLibraryLoad(FskLibrary *library, const char *path)
{
	return KplLibraryLoad(library, path);
}

FskErr FskLibraryUnload(FskLibrary library)
{
	return KplLibraryUnload(library);
}

FskErr FskLibraryGetSymbolAddress_(FskLibrary library, const char *symbol, void **address)
{
	return KplLibraryGetSymbolAddress(library, symbol, address);
}

#else

FskErr FskLibraryLoad(FskLibrary *library, const char *path)
{
	return kFskErrUnimplemented;
}

FskErr FskLibraryUnload(FskLibrary library)
{
	return kFskErrUnimplemented;
}

FskErr FskLibraryGetSymbolAddress_(FskLibrary library, const char *symbol, void **address)
{
	return kFskErrUnimplemented;
}

#endif
