/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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

#include "FskPin.h"

#include "FskThread.h"

#include <poll.h>
#include <stddef.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>

/*
	Linux GPIO
*/

static Boolean linuxDigitalCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber);
static FskErr linuxDigitalNew(FskPinDigital *pin, SInt32 number, const char *name, FskPinsDigitalDirection direction);
static void linuxDigitalDispose(FskPinDigital pin);
static FskErr linuxDigitalSetDirection(FskPinDigital pin, FskPinsDigitalDirection direction);
static FskErr linuxDigitalGetDirection(FskPinDigital pin, FskPinsDigitalDirection *direction);
static FskErr linuxDigitalRead(FskPinDigital pin, Boolean *value);
static FskErr linuxDigitalWrite(FskPinDigital pin, Boolean value);
static FskErr linuxDigitalRepeat(FskPinDigital pin, FskPinDigitalRepeatTriggerProc triggered, void *refCon);

FskPinDigitalDispatchRecord gLinuxPinDigital = {
	linuxDigitalCanHandle,
	linuxDigitalNew,
	linuxDigitalDispose,
	linuxDigitalSetDirection,
	linuxDigitalGetDirection,
	linuxDigitalRead,
	linuxDigitalWrite,
	linuxDigitalRepeat
};

typedef struct {
	FskPinDigitalRecord				pd;

	int								pin;
	int								file;
	Boolean							canInterrupt;
	FskPinsDigitalDirection			direction;

	FskPinDigitalRepeatTriggerProc	triggeredCallback;
	void							*triggeredRefCon;

	struct linuxDigitalRecord		*next;
} linuxDigitalRecord, *linuxDigital;

static FskListMutex gLinuxDigitalPins;

Boolean linuxDigitalCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber)
{
	return NULL == name;
}

FskErr linuxDigitalNew(FskPinDigital *pin, SInt32 number, const char *name, FskPinsDigitalDirection direction)
{
	FskErr err;
	FILE *f;
	linuxDigital ld;
	int file = -1;
	char buffer[50];

	if (NULL == gLinuxDigitalPins) {
		err = FskListMutexNew(&gLinuxDigitalPins, "linux digital repeat thread");
		if (err) return err;
	}

	f = fopen("/sys/class/gpio/export", "w");
	if (!f) printf("failed to export pin\n");
	if (!f) return kFskErrFileNotFound;

	fprintf(f, "%d", (int)number);
	fclose(f);

	err = FskMemPtrNewClear(sizeof(linuxDigitalRecord), (FskMemPtr *)&ld);
	if (err) return err;

	ld->pin = (int)number;
	ld->file = -1;
	ld->canInterrupt = false;

	FskListMutexAppend(gLinuxDigitalPins, &ld->next);

	ld->canInterrupt = false;

	*pin = (FskPinDigital)ld;
	return linuxDigitalSetDirection((FskPinDigital)ld, direction);
}

void linuxDigitalDispose(FskPinDigital pin)
{
	linuxDigital ld = (linuxDigital)pin;

	linuxDigitalRepeat(pin, NULL, NULL);

	if (ld->file >= 0)
		close(ld->file);

	FskListMutexRemove(gLinuxDigitalPins, &ld->next);
	FskMemPtrDispose(pin);
}

FskErr linuxDigitalSetDirection(FskPinDigital pin, FskPinsDigitalDirection direction)
{
	linuxDigital ld = (linuxDigital)pin;
	FskErr err = kFskErrNone;
	FILE *f = NULL;
	int file = -1;
	char buffer[50];

	if (ld->file >= 0) {
		close(ld->file);
		ld->file = -1;
	}

	snprintf(buffer, sizeof(buffer), "/sys/class/gpio/gpio%d/value", ld->pin);
	file = open(buffer, O_RDWR);
	if (file <= 0) {
		err = kFskErrFileNotFound;
		goto bail;
	}
	ld->file = file;

	if (kFskPinDigitalDirectionUndefined != direction) {
		snprintf(buffer, sizeof(buffer), "/sys/class/gpio/gpio%d/direction", ld->pin);
		f = fopen(buffer, "w+");
		if (!f) return kFskErrFileNotFound;

		if (direction == kFskPinDigitalDirectionOut){
			Boolean value;
			err = linuxDigitalRead(pin, &value);
			fprintf(f, value ? "high" : "low");
		}else if (direction == kFskPinDigitalDirectionIn){
			int edgeFile = -1;

			snprintf(buffer, sizeof(buffer), "/sys/class/gpio/gpio%d/edge", ld->pin);
			edgeFile = open(buffer, O_WRONLY);
			if (edgeFile >= 0) {
				ld->canInterrupt = write(edgeFile, "both", 5) > 0;
				close(edgeFile);
			}

			fprintf(f, "in");
		}

		fclose(f);
		f = NULL;
	}

bail:
	if (err)
		direction = kFskPinDigitalDirectionError;

	ld->direction = direction;

	return err;
}

FskErr linuxDigitalGetDirection(FskPinDigital pin, FskPinsDigitalDirection *direction)
{
	linuxDigital ld = (linuxDigital)pin;
	FskErr err = kFskErrNone;
	FILE *f = NULL;
	char buffer[50];
    signed char existingDirection = -1;

	snprintf(buffer, sizeof(buffer), "/sys/class/gpio/gpio%d/direction", ld->pin);

    f = fopen(buffer, "r+");
    if (!f) return kFskErrFileNotFound;
    rewind(f);
    fscanf(f, "%c", &existingDirection);
    rewind(f);
	fclose(f);

    if ('i' == existingDirection)
        *direction = kFskPinDigitalDirectionIn;
    else if ('o' == existingDirection)
        *direction = kFskPinDigitalDirectionOut;
    else
        *direction = kFskPinDigitalDirectionUndefined;

    return err;
}

FskErr linuxDigitalRead(FskPinDigital pin, Boolean *valueOut)
{
	linuxDigital ld = (linuxDigital)pin;
	int result, value = -1;
	char buffer[6];

	lseek(ld->file, 0, SEEK_SET);
	result = read(ld->file, buffer, sizeof(buffer) - 1);
	if (result < 1) return kFskErrOperationFailed;
	buffer[result - 1] = 0;
	value = FskStrToNum(buffer);
	*valueOut = 1 == value;

	return kFskErrNone;
}

FskErr linuxDigitalWrite(FskPinDigital pin, Boolean value)
{
	linuxDigital ld = (linuxDigital)pin;
	int result = write(ld->file, value ? "1\n" : "0\n", 2);
	return (2 == result) ? kFskErrNone : kFskErrOperationFailed;
}

static int gGPIOPollersSeed = 0;
static int gGPIOEventFD = -1;
static FskThread gGPIOThread;
static Boolean gGPIOThreadQuit = false;

static void gpioThreadQuit(void);
static void gpioThread(void *param);

FskErr linuxDigitalRepeat(FskPinDigital pin, FskPinDigitalRepeatTriggerProc triggeredCallback, void *refCon)
{
	linuxDigital ld = (linuxDigital)pin;
	linuxDigital walker;
	int interruptCount;

	if (!ld->canInterrupt)
		return kFskErrUnimplemented;

	if ((ld->triggeredCallback == triggeredCallback) && (ld->triggeredRefCon == refCon))
		return kFskErrNone;

	ld->triggeredCallback = triggeredCallback,
	ld->triggeredRefCon = refCon;

	for (walker = gLinuxDigitalPins->list, interruptCount = 0; NULL != walker; walker = *(linuxDigital *)walker) {
		linuxDigital w = (linuxDigital)(((char *)walker) - offsetof(linuxDigitalRecord, next));

		if (!w->canInterrupt)
			continue;

		if (!w->triggeredCallback)
			continue;

		interruptCount += 1;
	}

	gGPIOPollersSeed++;
	if (0 == interruptCount) {
		if (gGPIOThread)
			gpioThreadQuit();
	}
	else if ((1 == interruptCount) && (NULL == gGPIOThread)) {
		gGPIOEventFD = eventfd(0, EFD_NONBLOCK);
		if (gGPIOEventFD >= 0)
			FskThreadCreate(&gGPIOThread, gpioThread, kFskThreadFlagsDefault, NULL, "linux digital repeat thread");
	}
	else {
		uint64_t one = 1;
		write(gGPIOEventFD, &one, sizeof(one));
	}

	return kFskErrNone;
}

void gpioThread(void *param)
{
	int seed = -1;
	struct pollfd *fds = NULL;
	int fdsInUse = 0;
	int pollersCount = 0;

	while (!gGPIOThreadQuit) {
		int result;
		linuxDigital walker;

		FskMutexAcquire(gLinuxDigitalPins->mutex);
			if (seed != gGPIOPollersSeed) {
				FskErr err;

				pollersCount = (int)FskListMutexCount(gLinuxDigitalPins);
				err = FskMemPtrRealloc((pollersCount + 1) * sizeof(struct pollfd), &fds);
				if (err) {
					FskMutexRelease(gLinuxDigitalPins->mutex);
					break;
				}
				seed = gGPIOPollersSeed;

				fds[0].fd = gGPIOEventFD;
				fds[0].events = POLLIN | POLLERR | POLLHUP;
				fdsInUse = 1;

				for (walker = (linuxDigital)gLinuxDigitalPins->list; NULL != walker; walker = *(linuxDigital *)walker) {
					linuxDigital w = (linuxDigital)(((char *)walker) - offsetof(linuxDigitalRecord, next));

					if (w->canInterrupt && w->triggeredCallback) {
						fds[fdsInUse].fd = w->file;
						fds[fdsInUse].events = POLLPRI;
						fdsInUse += 1;
					}
				}
			}
		FskMutexRelease(gLinuxDigitalPins->mutex);

		result = poll(fds, fdsInUse, -1);

		if (fds[0].revents & POLLIN) {
			uint64_t ignore;
			read(gGPIOEventFD, &ignore, sizeof(ignore));
		}

		FskMutexAcquire(gLinuxDigitalPins->mutex);
			for (walker = (linuxDigital)gLinuxDigitalPins->list; NULL != walker; walker = *(linuxDigital *)walker) {
				linuxDigital w = (linuxDigital)(((char *)walker) - offsetof(linuxDigitalRecord, next));
				int i;
				struct pollfd *fdp = &fds[1];

				for (i = 1; i < fdsInUse; i++, fdp++) {
					if ((fdp->fd == w->file) && (fdp->revents & POLLPRI)){
						char buffer[6];

						lseek(w->file, 0, SEEK_SET);
						read(w->file, buffer, sizeof(buffer) - 1);

						if (w->triggeredCallback)
							(w->triggeredCallback)((FskPinDigital)w, w->triggeredRefCon);
						break;
					}
				}
			}
		FskMutexRelease(gLinuxDigitalPins->mutex);
	}

	FskMemPtrDispose(fds);
}

void gpioThreadQuit(void)
{
	uint64_t one = 1;

	gGPIOThreadQuit = true;
	write(gGPIOEventFD, &one, sizeof(one));
	FskThreadJoin(gGPIOThread);
	gGPIOThread = NULL;
	gGPIOThreadQuit = false;
	close(gGPIOEventFD);
	gGPIOEventFD = -1;
}

/*
	Extension
*/

FskExport(FskErr) FskPinDigitalLinux_fskLoad(FskLibrary library)
{
	return FskExtensionInstall(kFskExtensionPinDigital, &gLinuxPinDigital);
}

FskExport(FskErr) FskPinDigitalLinux_fskUnload(FskLibrary library)
{
	return FskExtensionUninstall(kFskExtensionPinDigital, &gLinuxPinDigital);
}
