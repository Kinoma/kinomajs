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
#define __FSKMEMORY_KPL_PRIV__
#include "Fsk.h"
#include "KplMemory.h"
#include "stdlib.h"
#include <unistd.h>
#include "FskThread.h"

FskInstrumentedSimpleType(KplMemory, KplMemory);

#undef USE_MEMORY_MONITOR_THREAD
#define USE_MEMORY_MONITOR_THREAD 0

// The following resources describe the /proc/meminfo file we read when USE_MEMORY_MONITOR_THREAD is enabled
// http://www.redhat.com/advice/tips/meminfo.html
// i-scream libstatgrab
// http://www.i-scream.org

#undef USE_OVERCOMMIT_RATIO
#if BG3CDP
#define USE_OVERCOMMIT_RATIO 0
#else
#define USE_OVERCOMMIT_RATIO 1
#endif

// The following resources describe the /proc/sys/vm files we use when USE_OVERCOMMIT_RATIO is enabled
// https://www.kernel.org/doc/Documentation/sysctl/vm.txt
// http://www.linuxdevcenter.com/pub/a/linux/2006/11/30/linux-out-of-memory.html

#undef TEST_FILL_MEMORY
#define TEST_FILL_MEMORY 0


#if SUPPORT_LINUX_GTK
#undef USE_OVERCOMMIT_RATIO
#undef USE_MEMORY_MONITOR_THREAD
#endif

// The TEST_FILL_MEMORY code can be enabled to verify that the FSK low memory callback fires.  The test allocates memory continually until the system isn't able to.

#if TEST_FILL_MEMORY
	static void fillMemoryCallback(FskTimeCallBack timer, const FskTime time, void *param);
#endif

#if USE_MEMORY_MONITOR_THREAD
static void KplMemoryMonitorThreadProc(void* refCon);

static FskThread gKplMemoryMonitorThread = NULL;

extern Boolean gQuitting;
#endif

static void doLowMemoryWarning(void);

FskErr KplMemPtrNew(UInt32 size, KplMemPtr *newMemory)
{
	*newMemory = (KplMemPtr)malloc(size);
	
	if (NULL == *newMemory) {
#if USE_OVERCOMMIT_RATIO
		doLowMemoryWarning();
		*newMemory = malloc(size);
#if SUPPORT_INSTRUMENTATION
		if (NULL == *newMemory)
			FskKplMemoryPrintfDebug("malloc FAILED after call to FskMemoryLowMemoryWarning");
		else
			FskKplMemoryPrintfDebug("malloc SUCCESS after call to FskMemoryLowMemoryWarning");
#endif
		if (NULL == *newMemory)
			return kFskErrMemFull;
#else
		return kFskErrMemFull;
#endif
	}

	return kFskErrNone;
}

FskErr KplMemPtrDispose(void *ptr)
{
	if (ptr)
		free(ptr);
	return kFskErrNone;
}

FskErr KplMemPtrRealloc(UInt32 size, KplMemPtr *newMemory)
{
	KplMemPtr result;
	result = (KplMemPtr)realloc(*newMemory, size);
	*newMemory = result;
	return (NULL == result && 0 != size) ? kFskErrMemFull : kFskErrNone;
}

FskErr KplMemPtrNewClear(UInt32 size, KplMemPtr *newMemory)
{
	*newMemory = (KplMemPtr)calloc(1, size);
	
	if (NULL == *newMemory) {
#if USE_OVERCOMMIT_RATIO
		doLowMemoryWarning();
		*newMemory = (KplMemPtr)calloc(1, size);
#if SUPPORT_INSTRUMENTATION
		if (NULL == *newMemory)
			FskKplMemoryPrintfDebug("malloc FAILED after call to FskMemoryLowMemoryWarning");
		else
			FskKplMemoryPrintfDebug("malloc SUCCESS after call to FskMemoryLowMemoryWarning");
#endif
		if (NULL == *newMemory)
			return kFskErrMemFull;
#else
		return kFskErrMemFull;
#endif
	}
	
	return kFskErrNone;
}

void KplMemoryInitialize()
{
}

void KplMemoryTerminate()
{
#if USE_OVERCOMMIT_RATIO
	int ret;
	ret = system("echo 0 > /proc/sys/vm/overcommit_memory");
	ret = system("echo 50 > /proc/sys/vm/overcommit_ratio");
#endif
}

void KplNotificationInitialize()
{
#if USE_MEMORY_MONITOR_THREAD
	FskThreadCreate(&gKplMemoryMonitorThread, KplMemoryMonitorThreadProc, kFskThreadFlagsJoinable | kFskThreadFlagsWaitForInit, NULL, "kplMemoryMonitor");
#elif USE_OVERCOMMIT_RATIO
	int ret;
	FskKplMemoryPrintfDebug("Setting overcommit values");
	ret = system("echo 2 > /proc/sys/vm/overcommit_memory");
	ret = system("echo 95 > /proc/sys/vm/overcommit_ratio");
#endif

#if TEST_FILL_MEMORY
	{
	FskTimeCallBack timer;
	FskTimeRecord when;
	FskTimeCallbackNew(&timer);
	FskTimeGetNow(&when);
	FskTimeAddSecs(&when, 2);
	FskTimeCallbackSet(timer, &when, fillMemoryCallback, NULL);
	}
#endif
}

static void doLowMemoryWarning(void)
{
	FskKplMemoryPrintfDebug("Issuing low memory warning");
	FskNotificationPost(kFskNotificationLowMemory);
}

#if USE_MEMORY_MONITOR_THREAD

#define kMemTotal "MemTotal:"
#define kMemFree "MemFree:"
#define kMemCached "Cached:"

static void KplMemoryMonitorThreadProc(void* refCon)
{
	char *p, buffer[256];
	
	FskThreadInitializationComplete(FskThreadGetCurrent());

	while (!gQuitting) {
		UInt32 totalMemory = 0;
		UInt32 cachedMemory = 0;
		UInt32 freeMemory = 0;
		UInt32 value;
		FILE *f = NULL;
		
		sleep(60);
		
		FskKplMemoryPrintfDebug("Reading memory stats");

		f = fopen("/proc/meminfo", "r");
		if (NULL == f) continue;
		
		while ((p = fgets(buffer, sizeof(buffer), f)) != NULL) {
			if (sscanf(buffer, "%*s %ld kB", &value) != 1)
				continue;

			if (strncmp(buffer, kMemTotal, sizeof(kMemTotal) - 1) == 0)
				totalMemory = value;
			else if (strncmp(buffer, kMemFree, sizeof(kMemFree) - 1) == 0)
				freeMemory = value;
			else if (strncmp(buffer, kMemCached, sizeof(kMemCached) - 1) == 0)
				cachedMemory = value;
				
			if (totalMemory && freeMemory && cachedMemory)
				break;
		}

		freeMemory += cachedMemory;
		fclose(f);
		
		FskKplMemoryPrintfDebug("Total = %ldkB free = %ldkB", totalMemory, freeMemory);
		
		if (freeMemory < (0.1 * totalMemory)) {
			doLowMemoryWarning();
		}
	}
}

#endif // USE_MEMORY_MONITOR_THREAD

#if TEST_FILL_MEMORY
#define kFillBuffers 20
static char *buffers[kFillBuffers] = {0};

static void lowMemoryWarningProc(void *refcon)
{
	int n;
	for (n = 0; n < kFillBuffers; ++n) {
		if (buffers[n]) {
			FskKplMemoryPrintfDebug("Freeing buffer %d", n);
			FskMemPtrDisposeAt(&buffers[n]);
		}
	}
}

static void fill_memory(void)
{
	int n = 0;
	char *p;

	// Register our own low memory warning callback
	FskKplMemoryPrintfDebug("Registering our callback");
	FskNotificationRegister(kFskNotificationLowMemory, lowMemoryWarningProc, NULL);
	
	// Allocate some 1MB test buffers
	for (n = 0; n < kFillBuffers; ++n) {
		FskKplMemoryPrintfDebug("Allocating buffer %d", n);
		FskMemPtrNew(1L<<20, (void*)&p);
		FskKplMemoryPrintfDebug("Allocated buffer %d", n);
		buffers[n] = p;
	}
	n = 0;
	
	// Allocate memory until the system fails an allocation and our low memory warning gets invoked
	while (1) {
		if (kFskErrNone != KplMemPtrNew(1L<<20, (void*)&p)) {
			FskKplMemoryPrintfDebug("malloc failure after %d MiB\n", n);
			return;
		}
		memset (p, 0, (1L<<20));
		FskKplMemoryPrintfDebug("got %d MiB\n", ++n);
	}
}

static void fillMemoryCallback(FskTimeCallBack timer, const FskTime time, void *param)
{
	FskKplMemoryPrintfDebug("In fillMemoryCallback");
	fill_memory();
}
#endif


