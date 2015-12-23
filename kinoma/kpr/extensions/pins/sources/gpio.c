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

#include "FskECMAScript.h"
#include "kprPins.h"
#include "FskMemory.h"
#include "FskThread.h"

static FskPinsDigitalDirection stringToDirection(xsMachine *the, const char *direction, FskPinDigital gpio);

FskTimeCallBack gDigitalPoller;

typedef struct {
	void			*next;
	FskPinDigital	gpio;
	KprPinsPoller	poller;
	FskThread		thread;
	Boolean			value;
	Boolean			usesCallback;
} digitalRepeatRecord, *digitalRepeat;

static digitalRepeat gDigitalRepeats;

static void digitalAddRepeat(FskPinDigital gpio, KprPinsPoller poller);
static void digitalRemoveRepeat(FskPinDigital gpio);
static void digitalRepeatPoll(FskTimeCallBack callback, const FskTime time, void *param);
static void digitalRepeatPollerThreadCallback(void *a0, void *a1, void *a2, void *a3);
static void digitalRepeatTriggered(FskPinDigital pin, void *refCon);

void xs_gpio(void *gpio)
{
    if (gpio) {
		digitalRemoveRepeat(gpio);
		FskPinDigitalDispose((FskPinDigital)gpio);
    }
}

void xs_gpio_init(xsMachine* the)
{
    FskErr err;
    FskPinDigital gpio;
    SInt32 pin = 0;
	FskPinsDigitalDirection dir;
    char *pinName = NULL;

    xsVars(1);

	gpio = xsGetHostData(xsThis);
    if (gpio)
        xsThrowDiagnosticIfFskErr(kFskErrOperationFailed, "Digital pin %d already initialized.", (int)pin);

    dir = stringToDirection(the, xsToString(xsGet(xsThis, xsID("_direction"))), gpio);

    xsVar(0) = xsGet(xsThis, xsID("_pin"));
    if (xsStringType == xsTypeOf(xsVar(0)))
        pinName = xsToString(xsVar(0));
    else
        pin = xsToInteger(xsVar(0));

	err = FskPinDigitalNew(&gpio, pin, pinName, dir);
    xsThrowDiagnosticIfFskErr(err, "Digital pin %d initialization failed with error %s.", pin, FskInstrumentationGetErrorString(err));

    xsSetHostData(xsThis, gpio);
}

void xs_gpio_write(xsMachine* the)
{
    FskPinDigital gpio = xsGetHostData(xsThis);
	Boolean value = xsToInteger(xsArg(0)) ? 1 : 0;
	FskErr err;

	xsThrowIfNULL(gpio);

	err = FskPinDigitalWrite(gpio, value);
	xsThrowDiagnosticIfFskErr(err, "Digital pin %d write error %s.", (int)-1 /* gpio->pinNum */, FskInstrumentationGetErrorString(err));
}

void xs_gpio_read(xsMachine* the)
{
    FskPinDigital gpio = xsGetHostData(xsThis);
	Boolean value;
	FskErr err;

	xsThrowIfNULL(gpio);

	err = FskPinDigitalRead(gpio, &value);
	xsThrowDiagnosticIfFskErr(err, "Digital pin %d read error %s.", (int)-1 /* gpio->pinNum */, FskInstrumentationGetErrorString((FskErr)value));
	xsResult = xsInteger(value);
}

void digitalAddRepeat(FskPinDigital gpio, KprPinsPoller poller)
{
	digitalRepeat repeat, walker;
	FskErr err;
	SInt32 usesCallbackCount;

	if (kFskErrNone != FskMemPtrNewClear(sizeof(digitalRepeatRecord), &repeat))
		return;

	repeat->gpio = gpio;
	repeat->poller = poller;
	repeat->thread = FskThreadGetCurrent();
	FskPinDigitalRead(gpio, &repeat->value);
	FskListAppend(&gDigitalRepeats, repeat);
	FskThreadPostCallback(repeat->thread, digitalRepeatPollerThreadCallback, poller, NULL, NULL, NULL);	// callback to set initial condition

	err = FskPinDigitalRepeat(gpio, digitalRepeatTriggered, repeat);
	if (kFskErrUnimplemented != err) {
		repeat->usesCallback = false;
		if (kFskErrNone != err)
			FskListRemove(&gDigitalRepeats, repeat);
		return;
	}

	repeat->usesCallback = true;

	for (walker = gDigitalRepeats, usesCallbackCount = 0; NULL != walker; walker = walker->next)
		if (walker->usesCallback)
			usesCallbackCount += 1;

	if (1 == usesCallbackCount) {
		FskTimeCallbackNew(&gDigitalPoller);
		FskTimeCallbackScheduleFuture(gDigitalPoller, 0, 33, digitalRepeatPoll, NULL);
	}
}

void digitalRemoveRepeat(FskPinDigital gpio)
{
	digitalRepeat walker = gDigitalRepeats;
	SInt32 usesCallbackCount;

	for (walker = gDigitalRepeats, usesCallbackCount = 0; NULL != walker; walker = walker->next) {
		if (walker->gpio == gpio)
			break;
		if (walker->usesCallback)
			usesCallbackCount += 1;
	}
	if (!walker) return;

	FskPinDigitalRepeat(gpio, NULL, NULL);
	FskListRemove(&gDigitalRepeats, walker);
	FskMemPtrDispose(walker);

	if (0 == usesCallbackCount) {
		FskTimeCallbackDispose(gDigitalPoller);
		gDigitalPoller = NULL;
	}
}

void digitalRepeatPoll(FskTimeCallBack callback, const FskTime time, void *param)
{
	digitalRepeat walker;

	for (walker = gDigitalRepeats; NULL != walker; walker = walker->next) {
		Boolean value;

		if (!walker->usesCallback)
			continue;

		if (kFskErrNone != FskPinDigitalRead(walker->gpio, &value))
			continue;

		if (value == walker->value)
			continue;

		walker->value = value;
		KprPinsPollerRun(walker->poller);
	}

	FskTimeCallbackScheduleFuture(gDigitalPoller, 0, 33, digitalRepeatPoll, NULL);
}

void digitalRepeatTriggered(FskPinDigital pin, void *refCon)
{
	digitalRepeat repeat = (digitalRepeat)refCon;
	FskThreadPostCallback(repeat->thread, digitalRepeatPollerThreadCallback, repeat->poller, NULL, NULL, NULL);
}

void digitalRepeatPollerThreadCallback(void *a0, void *a1, void *a2, void *a3)
{
    KprPinsPollerRun((KprPinsPoller)a0);
}

void xs_gpio_repeat(xsMachine* the)
{
    FskPinDigital gpio = xsGetHostData(xsThis);
	FskPinsDigitalDirection direction = kFskPinDigitalDirectionError;
	KprPinsPoller poller;

	if (!gpio) return;

	FskPinDigitalGetDirection(gpio, &direction);
	if (kFskPinDigitalDirectionIn != direction)
		xsThrowDiagnosticIfFskErr(kFskErrUnimplemented, "Digital pin %d cannot repeat on output pin", (int)-1 /* gpio->pinNum */);

	digitalRemoveRepeat(gpio);

	poller = (xsTest(xsArg(0))) ? xsGetHostData(xsArg(0)) : NULL;
	if (poller)
		digitalAddRepeat(gpio, poller);
}

void xs_gpio_get_direction(xsMachine* the)
{
    FskPinDigital gpio = xsGetHostData(xsThis);
	FskPinsDigitalDirection direction;
	FskErr err;

	xsThrowIfNULL(gpio);

	err = FskPinDigitalGetDirection(gpio, &direction);
	xsThrowIfFskErr(err);

	if (kFskPinDigitalDirectionIn == direction)
		xsResult = xsString("in");      //@@ input - to match constructor
	else if (kFskPinDigitalDirectionOut == direction)
		xsResult = xsString("out");     //@@ output - to match constructor
	else
		xsResult = xsString("undefined");
}

void xs_gpio_set_direction(xsMachine* the)
{
    FskPinDigital gpio = xsGetHostData(xsThis);
	FskPinsDigitalDirection dir = stringToDirection(the, xsToString(xsArg(0)), gpio);
	FskErr err;

	xsThrowIfNULL(gpio);

	err = FskPinDigitalSetDirection(gpio, dir);
	xsThrowIfFskErr(err);
}

void xs_gpio_close(xsMachine* the)
{
    xs_gpio(xsGetHostData(xsThis));
    xsSetHostData(xsThis, NULL);
}

FskPinsDigitalDirection stringToDirection(xsMachine *the, const char *direction, FskPinDigital gpio)
{
	if (0 == FskStrCompare(direction, "input"))
		return kFskPinDigitalDirectionIn;
	else if (0 == FskStrCompare(direction, "output"))
		return kFskPinDigitalDirectionOut;
	else if (0 == FskStrCompare(direction, "undefined"))
		return kFskPinDigitalDirectionUndefined;

    xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "Digital pin %d must specify direction as input / output / undefined, not %s", (int)-1 /* gpio->pinNum */, direction);

	return kFskPinDigitalDirectionError;		// never reach here, but compiler wants to see a return.
}
