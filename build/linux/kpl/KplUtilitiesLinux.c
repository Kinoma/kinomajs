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
#include "FskString.h"
#include "FskUtilities.h"
#include "KplUtilities.h"
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

FskErr KplUtilitiesInitialize(void)
{
	struct sigaction action;
	FskMemSet(&action, 0, sizeof(action));
	action.sa_handler = SIG_IGN;
	action.sa_flags = 0;
	sigaction(SIGPIPE, &action, NULL);
//	sigaction(SIGHUP, &action, NULL);
//	action.sa_handler = sInvokeCleanup;
//	sigaction(SIGINT, &action, NULL);

	return kFskErrNone;
}

void KplUtilitiesTerminate(void)
{
	return;
}

FskErr KplUtilitiesHardwareInitialize(void)
{
	return kFskErrNone;
}

FskErr KplUtilitiesHardwareTerminate(void)
{
	return kFskErrNone;
}

UInt32 KplUtilitiesRandomSeedInit(void)
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	return (UInt32) tv.tv_usec;
}

char *KplUtilitiesGetApplicationPath(void)
{
    char *p, *name, *ret, canonPath[PATH_MAX];
	char **argv;
	int argc;

	FskUtilsGetArgs(&argc, &argv);
    p = FskStrDoCopy(argv[0]);
    name = FskStrRChr(p, '/');
    if (!name) {
        ret = FskStrDoCopy("./");
        FskMemPtrDispose(p);
    }
    else {
        *++name = '\0';
        ret = p;
    }
    realpath(ret, canonPath);
    FskMemPtrDispose(ret);
    ret = FskStrDoCat(canonPath, "/");
    return ret;
}

FskErr KplBrowserOpenURI(const char *uri)
{
#if TARGET_OS_KPL && SUPPORT_LINUX_GTK
	char *command = FskStrDoCat("xdg-open ", uri);
	system(command);
	FskMemPtrDispose(command);
	return kFskErrNone;
#else
	return kFskErrUnimplemented;
#endif
}

FskErr KplLauncherOpenURI(const char *uri)
{
	return kFskErrUnimplemented;
}

void KplUtilitiesDelay(UInt32 ms)
{
	if (ms > (0x7fffffff / 1000))
		sleep(ms / 1000);
	else
		usleep(ms * 1000);
}

#ifndef  ANDROID_PLATFORM
SInt32 KplUtilitiesRandom(UInt32 *randomSeed)
{
	unsigned int seed = *randomSeed;
	SInt32 result;
	result = rand_r(&seed);
	*randomSeed = seed;
	return result;
}

#else
// int androidGetRandomNumber(UInt32 *seed) {		// Reentrant random function from POSIX.1c.
SInt32 KplUtilitiesRandom(UInt32 *seed)
{
	unsigned int next = *seed;
	int result;

	next *= 1103515245;
	next += 12345;
	result = (unsigned int) (next / 65536) % 2048;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int) (next / 65536) % 1024;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int) (next / 65536) % 1024;

	*seed = next;

	return result;
}

#endif
