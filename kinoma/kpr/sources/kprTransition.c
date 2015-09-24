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
#include <math.h>

#define __FSKPORT_PRIV__
#include "FskPort.h"
#include "FskText.h"
#include "FskTextConvert.h"
#include "FskUtilities.h"

#include "kprBehavior.h"
#include "kprContent.h"
#include "kprShell.h"
#include "kprTransition.h"

static void KprTransitionBegin(KprTransition self);
static void KprTransitionEnd(KprTransition self);
static void KprTransitionIdle(void* it, double interval);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprTransitionInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprTransition", FskInstrumentationOffset(KprTransitionRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprTransitionDispatchRecord = {
	"transition",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	KprTransitionIdle,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
    NULL
};

void KprTransitionLink(KprTransition self, KprContainer container)
{
	KprTransition* address = &container->transition;
	while (*address)
		address = &((*address)->next);
	*address = self;
	self->container = container;
	self->shell = container->shell;
	FskInstrumentedItemSetOwner(self, container);
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedTransitionLink, &self->slot);
	fxRemember(self->the, &self->slot);
	if (container->transition == self) {
		kprDelegateTransitionBeginning(container);
		KprTransitionBegin(self);
	}
}

void KprTransitionUnlink(KprTransition self, KprContainer container)
{
	if (self) {
		if (self->next)
			KprTransitionUnlink(self->next, container);
		else
			container->transition = NULL;
        if (self->shell) {
            KprShellStopIdling(self->shell, self);
            self->shell = NULL;
        }
		self->container = NULL;
		FskInstrumentedItemClearOwner(self);
		FskInstrumentedItemSendMessageDebug(self, kprInstrumentedTransitionUnlink, &self->slot);
		fxForget(self->the, &self->slot);
	}
}

/* IMPLEMENTATION */

void KprTransitionBegin(KprTransition self)
{
	self->time = -1;
	{
		xsBeginHostSandboxCode(self->the, self->code);
		{
			xsVars(3);
			xsVar(0) = xsAccess(self->slot);
			xsVar(1) = xsGet(xsVar(0), xsID_onBegin);
			xsVar(2) = xsGet(xsVar(0), xsID_parameters);
			(void)xsCall2(xsVar(1), xsID_apply, xsVar(0), xsVar(2));
			(void)xsCall1(xsVar(0), xsID_onStep, xsNumber(0));
		}
		xsEndHostSandboxCode();
	}
	self->shell = self->container->shell;
	KprShellStartIdling(self->shell, self);
}

void KprTransitionEnd(KprTransition self)
{
	KprShellStopIdling(self->shell, self);
	self->shell = NULL;
	{
		xsBeginHostSandboxCode(self->the, self->code);
		{
			xsVars(3);
			xsVar(0) = xsAccess(self->slot);
			xsVar(1) = xsGet(xsVar(0), xsID_onEnd);
			xsVar(2) = xsGet(xsVar(0), xsID_parameters);
			(void)xsCall2(xsVar(1), xsID_apply, xsVar(0), xsVar(2));
			xsDelete(xsVar(0), xsID_parameters);
		}
		xsEndHostSandboxCode();
	}
}

void KprTransitionIdle(void* it, double interval)
{
    KprTransition self = it;
	if (self->time < 0)
		self->time = 0;
	else if (self->time < self->duration) {
		self->time += interval;
		if (self->time >= self->duration)
			self->time = self->duration;
		{
			xsBeginHostSandboxCode(self->the, self->code);
			xsVars(1);
			xsVar(0) = xsAccess(self->slot);
			(void)xsCall1(xsVar(0), xsID_onStep, xsNumber(self->time / self->duration));
			xsEndHostSandboxCode();
		}
	}
	else {
		KprContainer container = self->container;
		KprTransition transition = container->transition = self->next;
		KprTransitionEnd(self);
		self->container = NULL;
		FskInstrumentedItemClearOwner(self);
		FskInstrumentedItemSendMessageDebug(self, kprInstrumentedTransitionUnlink, &self->slot);
		fxForget(self->the, &self->slot);
		if (transition)
			KprTransitionBegin(transition);
		else
			kprDelegateTransitionEnded(container);
	}
}

/* ECMASCRIPT */

void KPR_transition(void *it)
{
	if (it) {
		KprTransition self = it;
        if (self->shell) {
            KprShellStopIdling(self->shell, self);
            self->shell = NULL;
        }
        if (self->container) { // delete machine
            FskInstrumentedItemClearOwner(self);
            self->container->transition = NULL;
        }
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

void KPR_Transition(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprTransition self;
	xsNumberValue duration = 250;
	if ((c > 0) && xsTest(xsArg(0)))
		duration = xsToNumber(xsArg(0));
	xsAssert(duration > 0);
	xsThrowIfFskErr(FskMemPtrNewClear(sizeof(KprTransitionRecord), &self));
	FskInstrumentedItemNew(self, NULL, &KprTransitionInstrumentation);
	self->dispatch = &KprTransitionDispatchRecord;
	self->duration = duration;
	self->interval = 1;
	self->the = the;
	self->slot = xsThis;
	self->code = the->code;
	xsSetHostData(xsThis, self);
}

void KPR_transition_get_duration(xsMachine *the)
{
	KprTransition self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->duration);
}

void KPR_transition_set_duration(xsMachine *the)
{
	KprTransition self = xsGetHostData(xsThis);
	xsNumberValue duration = xsToNumber(xsArg(0));
	xsAssert(duration > 0);
	self->duration = duration;
}
	
#define PI 3.14159265358979323846

void Math_quadEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(fraction * fraction);
}

void Math_quadEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(-1 * fraction * (fraction - 2));
}

void Math_quadEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	fraction *= 2;
	if (fraction < 1)
		xsResult = xsNumber(fraction * fraction / 2);
	else {
		fraction -= 1;
		xsResult = xsNumber((fraction * (fraction - 2) - 1) / -2);
	}
}

void Math_cubicEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(fraction * fraction * fraction);
}

void Math_cubicEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	double f = fraction - 1;
	xsResult = xsNumber(f * f * f + 1);
}

void Math_cubicEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	fraction *= 2;
	if (fraction < 1)
		xsResult = xsNumber(fraction * fraction * fraction / 2);
	else {
		fraction -= 2;
		xsResult = xsNumber((fraction * fraction * fraction + 2) / 2);
	}
}

void Math_quartEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(fraction * fraction * fraction * fraction);
}

void Math_quartEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	double f = fraction - 1;
	xsResult = xsNumber(-1 * (f * f * f * f - 1));
}

void Math_quartEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	fraction *= 2;
	if (fraction < 1)
		xsResult = xsNumber(fraction * fraction * fraction * fraction / 2);
	else {
		fraction -= 2;
		xsResult = xsNumber((fraction * fraction * fraction * fraction - 2) / -2);
	}
}

void Math_quintEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(fraction * fraction * fraction * fraction * fraction);
}

void Math_quintEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	double f = fraction - 1;
	xsResult = xsNumber(f * f * f * f * f + 1);
}

void Math_quintEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	fraction *= 2;
	if (fraction < 1)
		xsResult = xsNumber(fraction * fraction * fraction * fraction * fraction / 2);
	else {
		fraction -= 2;
		xsResult = xsNumber((fraction * fraction * fraction * fraction * fraction + 2) / 2);
	}
}

void Math_sineEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(-1 * cos(fraction * (PI / 2)) + 1);
}

void Math_sineEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(sin(fraction * (PI / 2)));
}

void Math_sineEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(-1/2 * (cos(PI * fraction) - 1));
}

void Math_circularEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(-1 * (sqrt(1 - fraction * fraction) - 1));
}

void Math_circularEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	fraction -= 1;
	xsResult = xsNumber(sqrt(1 - fraction * fraction));
}

void Math_circularEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	fraction *= 2;
	if (fraction < 1)
		xsResult = xsNumber((sqrt(1 - fraction * fraction) - 1) / -2);
	else {
		fraction -= 2;
		xsResult = xsNumber((sqrt(1 - fraction * fraction) + 1) / 2);
	}
}

void Math_exponentialEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber((fraction == 0) ? 0 : pow(2, 10 * (fraction - 1)));
}

void Math_exponetialEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber((fraction == 1) ? 1 : (-pow(2, -10 * fraction) + 1));
}

void Math_exponentialEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	if (fraction == 0) 
		xsResult = xsNumber(0);
	else if (fraction == 1) 
		xsResult = xsNumber(1);
	else {
		fraction *= 2;
		if (fraction < 1)
			xsResult = xsNumber(pow(2, 10 * (fraction - 1)) / 2);
		else
			xsResult = xsNumber((-pow(2, -10 * --fraction) + 2) / 2);
	}
}

void Math_backEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	double s = xsTest(xsArg(1)) ? xsToNumber(xsArg(1)) : 1.70158;
	xsResult = xsNumber(fraction * fraction * ((s + 1) * fraction - s));
}

void Math_backEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	double s = xsTest(xsArg(1)) ? xsToNumber(xsArg(1)) : 1.70158;
	fraction -= 1;
	xsResult = xsNumber(fraction * fraction * ((s + 1) * fraction + s) + 1);
}

void Math_backEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	double s = xsTest(xsArg(1)) ? xsToNumber(xsArg(1)) : 1.70158;
	s *= 1.525;
	fraction *= 2;
	if (fraction < 1)
		xsResult = xsNumber((fraction * fraction * (s + 1) * fraction - s) / 2);
	else {
		fraction -= 2;
		xsResult = xsNumber((fraction * fraction * ((s + 1) * fraction + s) + 2) / 2);
	}
}

static double bounce(double fraction)
{
	if (fraction < (1 / 2.75))
		fraction = 7.5625 * fraction * fraction;
	else if (fraction < (2 / 2.75)) {
		fraction -= (1.5 / 2.75);
		fraction = 7.5625 * fraction * fraction + 0.75;
	}
	else if (fraction < (2.5 / 2.75)) {
		fraction -= (2.25 / 2.75);
		fraction = 7.5625 * fraction * fraction + 0.9375;
	}
	else {
		fraction -= (2.625 / 2.75);
		fraction = 7.5625 * fraction * fraction + 0.984375;
	}
	return fraction;
}

void Math_bounceEaseIn(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(1 - bounce(1 - fraction));
}

void Math_bounceEaseOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	xsResult = xsNumber(bounce(fraction));
}

void Math_bounceEaseInOut(xsMachine* the)
{
	double fraction = xsToNumber(xsArg(0));
	if (fraction < 0.5)
		xsResult = xsNumber((1 - bounce(1 - (fraction * 2))) / 2);
	else
		xsResult = xsNumber(bounce(fraction * 2 - 1) / 2 + 0.5);
}

void Math_elasticEaseIn(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	double fraction = xsToNumber(xsArg(0));
	if (fraction == 0)	
		xsResult = xsNumber(0);
	else if (fraction == 1) 
		xsResult = xsNumber(1);
	else {
		double a = (c > 1) ? xsToNumber(xsArg(1)) : 0;
		double p = (c > 2) ? xsToNumber(xsArg(2)) : 0;
		double s;
		if (!p) 
			p = 0.3;
		if (a < 1) {
			a = 1;
			s = p / 4;
		}
		else 
			s = p / (2 * PI) * asin(1 / a);
		fraction -= 1;
		xsResult = xsNumber(-(a * pow(2, 10 * fraction) * sin( (fraction - s) * (2 * PI) / p )));
	}
}

void Math_elasticEaseOut(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	double fraction = xsToNumber(xsArg(0));
	if (fraction == 0)	
		xsResult = xsNumber(0);
	else if (fraction == 1) 
		xsResult = xsNumber(1);
	else {
		double a = (c > 1) ? xsToNumber(xsArg(1)) : 0;
		double p = (c > 2) ? xsToNumber(xsArg(2)) : 0;
		double s;
		if (!p) 
			p = 0.3;
		if (a < 1) { 
			a = 1;
			s = p / 4;
		} 
		else
			s = p / (2 * PI) * asin(1 / a);
		xsResult = xsNumber(a * pow(2, -10 * fraction ) * sin((fraction - s) * (2 * PI) / p ) + 1);
	}
}

void Math_elasticEaseInOut(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	double fraction = xsToNumber(xsArg(0));
	if (fraction == 0) 
		xsResult = xsNumber(0);
	else if (fraction == 1) 
		xsResult = xsNumber(1);
	else {
		double a = (c > 1) ? xsToNumber(xsArg(1)) : 0;
		double p = (c > 2) ? xsToNumber(xsArg(2)) : 0;
		double s;
		fraction *= 2;
		if (!p)
			p = 0.45;
		if (a < 1) { 
			a = 1;
			s = p / 4;
		} 
		else 
			s = p / (2 * PI) * asin(1 / a);
		fraction -= 1;
		if (fraction < 0)
			xsResult = xsNumber((a * pow(2, 10 * fraction) * sin((fraction - s) * (2 * PI) / p )) / -2);
		else	
			xsResult = xsNumber(a * pow(2, -10 * fraction) * sin((fraction - s) * 2 * PI / p ) / 2 + 1);
	}
}

void Math_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("Math"));
	xsNewHostProperty(xsResult, xsID("backEaseIn"), xsNewHostFunction(Math_backEaseIn, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("backEaseInOut"), xsNewHostFunction(Math_backEaseInOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("backEaseOut"), xsNewHostFunction(Math_backEaseOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("bounceEaseIn"), xsNewHostFunction(Math_bounceEaseIn, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("bounceEaseInOut"), xsNewHostFunction(Math_bounceEaseInOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("bounceEaseOut"), xsNewHostFunction(Math_bounceEaseOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("circularEaseIn"), xsNewHostFunction(Math_circularEaseIn, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("circularEaseInOut"), xsNewHostFunction(Math_circularEaseInOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("circularEaseOut"), xsNewHostFunction(Math_circularEaseOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("cubicEaseIn"), xsNewHostFunction(Math_cubicEaseIn, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("cubicEaseInOut"), xsNewHostFunction(Math_cubicEaseInOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("cubicEaseOut"), xsNewHostFunction(Math_cubicEaseOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("elasticEaseIn"), xsNewHostFunction(Math_elasticEaseIn, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("elasticEaseInOut"), xsNewHostFunction(Math_elasticEaseInOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("elasticEaseOut"), xsNewHostFunction(Math_elasticEaseOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("exponentialEaseIn"), xsNewHostFunction(Math_exponentialEaseIn, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("exponentialEaseInOut"), xsNewHostFunction(Math_exponentialEaseInOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("exponetialEaseOut"), xsNewHostFunction(Math_exponetialEaseOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("quadEaseIn"), xsNewHostFunction(Math_quadEaseIn, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("quadEaseInOut"), xsNewHostFunction(Math_quadEaseInOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("quadEaseOut"), xsNewHostFunction(Math_quadEaseOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("quartEaseIn"), xsNewHostFunction(Math_quartEaseIn, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("quartEaseInOut"), xsNewHostFunction(Math_quartEaseInOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("quartEaseOut"), xsNewHostFunction(Math_quartEaseOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("quintEaseIn"), xsNewHostFunction(Math_quintEaseIn, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("quintEaseInOut"), xsNewHostFunction(Math_quintEaseInOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("quintEaseOut"), xsNewHostFunction(Math_quintEaseOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("sineEaseIn"), xsNewHostFunction(Math_sineEaseIn, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("sineEaseInOut"), xsNewHostFunction(Math_sineEaseInOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsResult, xsID("sineEaseOut"), xsNewHostFunction(Math_sineEaseOut, 1), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
}
