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

// ideally these would be protected with a mutex because they are accessed across threads
static FskGPIO gGPIOPollers;
static FskTimeCallBack gGPIOPoller;

static FskErr FskGPIONew(FskGPIO *gpioOut, int pin, char *pinName, GPIOdirection direction);
static void gpioPoller(FskTimeCallBack callback, const FskTime time, void *param);
static void gpioPollerThreadCallback(void *a0, void *a1, void *a2, void *a3);
static GPIOdirection stringToDirection(xsMachine *the, const char *direction, FskGPIO gpio);

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
        FskListRemove(&gGPIOPollers, gpio);
        if ((NULL == gGPIOPollers) && (NULL != gGPIOPoller)) {
            FskTimeCallbackDispose(gGPIOPoller);
            gGPIOPoller = NULL;
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

    xsVars(1);

    if (xsGetHostData(xsThis))
        xsThrowDiagnosticIfFskErr(kFskErrOperationFailed, "Digital pin %d already initialized.", (int)gpio->pinNum);

    dir = stringToDirection(the, xsToString(xsGet(xsThis, xsID("_direction"))), gpio);

    xsVar(0) = xsGet(xsThis, xsID("_pin"));
    if (xsStringType == xsTypeOf(xsVar(0)))
        pinName = xsToString(xsVar(0));
    else
        pin = xsToInteger(xsVar(0));

    err = FskGPIONew(&gpio, pin, pinName, dir);
    xsThrowDiagnosticIfFskErr(err, "Digital pin %d initialization failed with error %s.", (int)gpio->pinNum, FskInstrumentationGetErrorString(err));

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
    	int value = FskGPIOPlatformRead(gpio);
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
        if (in != gpio->direction)
            xsThrowDiagnosticIfFskErr(kFskErrUnimplemented, "Digital pin %d cannot repeat on output pin", (int)gpio->pinNum);

        if (gpio->poller)
            FskListRemove(&gGPIOPollers, gpio);
        
        gpio->poller = (xsTest(xsArg(0))) ? xsGetHostData(xsArg(0)) : NULL;

        if (gpio->poller) {
            FskListAppend(&gGPIOPollers, gpio);
            gpio->pollerValue = -1;     // won't match
            if (NULL == gGPIOPoller) {
                FskTimeCallbackNew(&gGPIOPoller);
                FskTimeCallbackScheduleNextRun(gGPIOPoller, gpioPoller, NULL);
            }
        }
        else if ((NULL == gGPIOPollers) && (NULL != gGPIOPoller)) {
            FskTimeCallbackDispose(gGPIOPoller);
            gGPIOPoller = NULL;
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

static void gpioPoller(FskTimeCallBack callback, const FskTime time, void *param)
{
    FskGPIO walker;
    FskThread thread = FskThreadGetCurrent();

    for (walker = gGPIOPollers; NULL != walker; walker = walker->next) {
        int value = FskGPIOPlatformRead(walker);
        if (value == walker->pollerValue)
            continue;

        walker->pollerValue = value;

        if (thread == walker->thread)
            KprPinsPollerRun(walker->poller);
        else
            FskThreadPostCallback(walker->thread, gpioPollerThreadCallback, walker, NULL, NULL, NULL);
    }

    FskTimeCallbackScheduleFuture(gGPIOPoller, 0, 33, gpioPoller, NULL);
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

    xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "Digital pin %d must specify direction", (int)gpio->pinNum);
}

#endif /* USEGPIO */
