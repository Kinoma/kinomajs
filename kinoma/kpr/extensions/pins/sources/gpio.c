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
#ifdef USEGPIO

#include "FskECMAScript.h"
#include "kprPins.h"
#include "FskMemory.h"
#include "FskThread.h"

#include <poll.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>

// ideally these would be protected with a mutex because they are accessed across threads
static FskListMutex gGPIOPollers;		// use a mutex list here...
static int gGPIOPollersSeed = 0;
static int gGPIOEventFD = -1;
static FskThread gGPIOThread;
static Boolean gGPIOThreadQuit = false;

static FskErr FskGPIONew(FskGPIO *gpioOut, int pin, char *pinName, GPIOdirection direction);
static void gpioPollerThreadCallback(void *a0, void *a1, void *a2, void *a3);
static GPIOdirection stringToDirection(xsMachine *the, const char *direction, FskGPIO gpio);

static void gpioThreadQuit(void);
static void gpioThread(void *param);

static FskErr FskGPIONew(FskGPIO *gpioOut, int pin, char *pinName, GPIOdirection direction)
{
	FskErr err = kFskErrNone;
	FskGPIO gpio = NULL;

    err = FskMemPtrNewClear(sizeof(FskGPIORecord), (FskMemPtr *)&gpio);
	BAIL_IF_ERR(err);

	gpio->pinNum = pin;
	gpio->realPin = FskHardwarePinsMux(pin, kFskHardwarePinGPIO);
    gpio->thread = FskThreadGetCurrent();

	if (gpio->realPin < 0)
        BAIL(kFskErrInvalidParameter);

    err = FskGPIOPlatformInit(gpio);
	BAIL_IF_ERR(err);

    if (undefined != direction) {
        err = FskGPIOPlatformSetDirection(gpio, direction);
        BAIL_IF_ERR(err);
    }

bail:
	if (err && gpio) {
        FskGPIOPlatformDispose(gpio);
        FskMemPtrDisposeAt(&gpio);
	}

    *gpioOut = gpio;

	return err;
}

void xs_gpio(void *gpio)
{
    if (gpio) {
        FskListMutexRemove(gGPIOPollers, gpio);
		gGPIOPollersSeed++;
		if (gGPIOThread) {
			uint64_t one = 1;
			UInt32 count = FskListMutexCount(gGPIOPollers);
			write(gGPIOEventFD, &one, sizeof(one));
			if (0 == count)
				gpioThreadQuit();
		}

        FskGPIOPlatformDispose(gpio);
        FskMemPtrDispose(gpio);
    }
}

void xs_gpio_init(xsMachine* the)
{
    FskErr err;
    FskGPIO gpio;
    SInt32 pin = 0;
	GPIOdirection dir;
    char *pinName = NULL;

	if (NULL == gGPIOPollers) {
		err = FskListMutexNew(&gGPIOPollers, "gpio read thread");
		xsThrowIfFskErr(err);
	}

    xsVars(1);

    if ((gpio = xsGetHostData(xsThis)))
        xsThrowDiagnosticIfFskErr(kFskErrOperationFailed, "Digital pin %d already initialized.", (int)gpio->pinNum);

    dir = stringToDirection(the, xsToString(xsGet(xsThis, xsID("_direction"))), gpio);

    xsVar(0) = xsGet(xsThis, xsID("_pin"));
    if (xsStringType == xsTypeOf(xsVar(0)))
        pinName = xsToString(xsVar(0));
    else
        pin = xsToInteger(xsVar(0));

    err = FskGPIONew(&gpio, pin, pinName, dir);
    xsThrowDiagnosticIfFskErr(err, "Digital pin %d initialization failed with error %s.", pin, FskInstrumentationGetErrorString(err));

    xsSetHostData(xsThis, gpio);
}

void xs_gpio_write(xsMachine* the)
{
    FskGPIO gpio = xsGetHostData(xsThis);
    if (gpio) {
        SInt32 value = xsToInteger(xsArg(0));
        FskErr err = FskGPIOPlatformWrite(gpio, value ? 1 : 0);
        xsThrowDiagnosticIfFskErr(err, "Digital pin %d write error %s.", (int)gpio->pinNum, FskInstrumentationGetErrorString(err));
    }
}

void xs_gpio_read(xsMachine* the)
{
    FskGPIO gpio = xsGetHostData(xsThis);
    if (gpio) {
		int value;
		if (gpio->poller && (-1 != gpio->pollerValue))
			value = gpio->pollerValue;
		else
			value = FskGPIOPlatformRead(gpio);
        if ((0 == value) || (1 == value))
            xsResult = xsInteger(value);
        else
            xsThrowDiagnosticIfFskErr((FskErr)value, "Digital pin %d read error %s.", (int)gpio->pinNum, FskInstrumentationGetErrorString((FskErr)value));
    }
}

void xs_gpio_repeat(xsMachine* the)
{
    FskGPIO gpio = xsGetHostData(xsThis);

    if (gpio) {
		UInt32 count;

        if (in != gpio->direction)
            xsThrowDiagnosticIfFskErr(kFskErrUnimplemented, "Digital pin %d cannot repeat on output pin", (int)gpio->pinNum);

        if (gpio->poller) {
            FskListMutexRemove(gGPIOPollers, gpio);
			gGPIOPollersSeed++;
		}

        gpio->poller = (xsTest(xsArg(0))) ? xsGetHostData(xsArg(0)) : NULL;

        if (gpio->poller) {
            gpio->pollerValue = -1;     // won't match
            FskListMutexAppend(gGPIOPollers, gpio);
			gGPIOPollersSeed++;
        }

		count = gGPIOPollers ? FskListMutexCount(gGPIOPollers) : 0;
		if (0 == count) {
			if (gGPIOThread)
				gpioThreadQuit();
		}
		else {
			if (NULL == gGPIOThread) {
				gGPIOEventFD = eventfd(0, EFD_NONBLOCK);
				if (gGPIOEventFD >= 0)
					FskThreadCreate(&gGPIOThread, gpioThread, kFskThreadFlagsDefault, NULL, "gpio thread");
			}
			else {
				uint64_t one = 1;
				write(gGPIOEventFD, &one, sizeof(one));
			}
		}
    }
}

void xs_gpio_get_direction(xsMachine* the)
{
    FskGPIO gpio = xsGetHostData(xsThis);
    if (gpio) {
        GPIOdirection direction;
        FskGPIOPlatformGetDirection(gpio, &direction);
        if (in == direction)
            xsResult = xsString("in");      //@@ input - to match constructor
        else if (out == direction)
            xsResult = xsString("out");     //@@ output - to match constructor
        else
            xsResult = xsString("undefined");
    }
}

void xs_gpio_set_direction(xsMachine* the)
{
    FskGPIO gpio = xsGetHostData(xsThis);
    FskErr err;
    GPIOdirection dir = stringToDirection(the, xsToString(xsArg(0)), gpio);

    err = FskGPIOPlatformSetDirection(gpio, dir);
    xsThrowIfFskErr(err);
}

void xs_gpio_close(xsMachine* the)
{
    xs_gpio(xsGetHostData(xsThis));
    xsSetHostData(xsThis, NULL);
}

static void gpioPollerThreadCallback(void *a0, void *a1, void *a2, void *a3)
{
    FskGPIO gpio = a0;
    KprPinsPollerRun(gpio->poller);
}

GPIOdirection stringToDirection(xsMachine *the, const char *direction, FskGPIO gpio)
{
	if (0 == FskStrCompare(direction, "input"))
		return in;
	else if (0 == FskStrCompare(direction, "output"))
		return out;
	else if (0 == FskStrCompare(direction, "undefined"))
		return undefined;

    xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "Digital pin %d must specify direction as input / output / undefined, not %s", (int)gpio->pinNum, direction);

	return undefined;		// never reach here, but compiler wants to see a return.
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

void gpioThread(void *param)
{
	int seed = -1;
	struct pollfd *fds = NULL;
	int fdsInUse = 0;
	int pollersCount = 0;

	while (!gGPIOThreadQuit) {
		int result;
		FskGPIO walker;

		FskMutexAcquire(gGPIOPollers->mutex);
			if (seed != gGPIOPollersSeed) {
				FskErr err;

				pollersCount = (int)FskListMutexCount(gGPIOPollers);
				err = FskMemPtrRealloc((pollersCount + 1) * sizeof(struct pollfd), &fds);
				if (err) {
					FskMutexRelease(gGPIOPollers->mutex);
					break;
				}
				seed = gGPIOPollersSeed;
				
				fds[0].fd = gGPIOEventFD;
				fds[0].events = POLLIN | POLLERR | POLLHUP;
				fdsInUse = 1;

				for (walker = (FskGPIO)gGPIOPollers->list; NULL != walker; walker = walker->next) {
					if (walker->canInterrupt) {
						fds[fdsInUse].fd = FskGPIOPlatformGetFD(walker);
						fds[fdsInUse].events = POLLPRI;
						fdsInUse += 1;
					}
				}
			}
		FskMutexRelease(gGPIOPollers->mutex);

		result = poll(fds, fdsInUse, (fdsInUse == (pollersCount + 1)) ? -1 : 33);

		if (fds[0].revents & POLLIN) {
			uint64_t ignore;
			read(gGPIOEventFD, &ignore, sizeof(ignore));
		}

		for (walker = (FskGPIO)gGPIOPollers->list; NULL != walker; walker = walker->next) {
			Boolean doUpdate = false;

			if (walker->canInterrupt) {
				int i, fd = FskGPIOPlatformGetFD(walker);
				struct pollfd *fdp = &fds[1];
				for (i = 1; i < fdsInUse; i++, fdp++) {
					if ((fdp->fd == fd) && (fdp->revents & POLLPRI)){
						doUpdate = true;
						break;
					}
				}
			}
			else
				doUpdate = true;

			if (doUpdate) {
				GPIOvalue value = FskGPIOPlatformRead(walker);
				if (value == walker->pollerValue)
					continue;
				walker->pollerValue = value;
				FskThreadPostCallback(walker->thread, gpioPollerThreadCallback, walker, NULL, NULL, NULL);
			}
		}
	}

	FskMemPtrDispose(fds);
}


#endif /* USEGPIO */
