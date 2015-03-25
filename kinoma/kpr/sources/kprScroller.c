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
#define __FSKPORT_PRIV__
#include "FskPort.h"
#include "FskText.h"
#include "FskTextConvert.h"
#include "FskUtilities.h"

#include "kprSkin.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprShell.h"
#include "kprUtilities.h"

#include "kprScroller.h"

static void KprScrollerFitHorizontally(void* it);
static void KprScrollerFitVertically(void* it);
static KprContent KprScrollerHit(void* it, SInt32 x, SInt32 y);
static void KprScrollerMeasureHorizontally(void* it);
static void KprScrollerMeasureVertically(void* it);
static void KprScrollerPlace(void* it);
static void KprScrollerPredict(void* it, FskRectangle area);
static void KprScrollerReflowing(void* it, UInt32 flags);
static void KprScrollerUpdate(void* it, FskPort port, FskRectangle area);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprScrollerInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprScroller", FskInstrumentationOffset(KprScrollerRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprScrollerDispatchRecord = {
	"scroller",
	KprContainerActivated,
	KprContainerAdded,
	KprContainerCascade,
	KprContainerDispose,
	KprContentDraw,
	KprScrollerFitHorizontally,
	KprScrollerFitVertically,
	KprContentGetBitmap,
	KprScrollerHit,
	KprContentIdle,
	KprContainerInvalidated,
	KprContainerLayered,
	KprContainerMark,
	KprScrollerMeasureHorizontally,
	KprScrollerMeasureVertically,
	KprScrollerPlace,
	KprContainerPlaced,
	KprScrollerPredict,
	KprScrollerReflowing,
	KprContainerRemoving,
	KprContainerSetWindow,
	KprContainerShowing,
	KprContainerShown,
	KprScrollerUpdate
};

FskErr KprScrollerNew(KprScroller* it,  KprCoordinates coordinates, KprSkin skin, KprStyle style)
{
	FskErr err = kFskErrNone;
	KprScroller self;
	bailIfError(FskMemPtrNewClear(sizeof(KprScrollerRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprScrollerInstrumentation);
	self->dispatch = &KprScrollerDispatchRecord;
	self->flags = kprContainer | kprVisible | kprScrolled | kprBackgroundTouch;
	KprContentInitialize((KprContent)self, coordinates, skin, style);
bail:
	return err;
}

void KprScrollerConstraint(KprScroller self, FskPoint delta) 
{
	KprContent content = self->first;
	if (content) {
		FskRectangle bounds = &content->bounds;
		KprCoordinates coordinates = &content->coordinates;
		UInt16 horizontal = coordinates->horizontal;
		UInt16 vertical = coordinates->vertical;
		SInt32 min, max, c, d;
		if (self->bounds.width < bounds->width) {
			if (self->flags & kprLooping) {
				c = 0;
				d = c - self->delta.x;
				d %= bounds->width;
				delta->x = c - d;
			}
			else {
				min = self->bounds.width - bounds->width;
				max = 0;
				if (horizontal & kprLeft)
					c = 0;
				else if (horizontal & kprRight)
					c = self->bounds.width - bounds->width;
				else
					c = (self->bounds.width - bounds->width + 1) >> 1;
				d = c - self->delta.x;
				if (d < min)
					d = min;
				if (d > max)
					d = max;
				delta->x = c - d;
			}
		}
		else
			delta->x = 0;
		if (self->bounds.height < bounds->height) {
			if (self->flags & kprLooping) {
				c = 0;
				d = c - self->delta.y;
				d %= bounds->height;
				if (d > 0)
					d -= bounds->height;
				delta->y = 0 - d;
			}
			else {
				min = self->bounds.height - bounds->height;
				max = 0;
				if (vertical & kprTop)
					c = 0;
				else if (vertical & kprBottom)
					c = self->bounds.height - bounds->height;
				else
					c = ((self->bounds.height - bounds->height + 1) >> 1);
				d = c - self->delta.y;
				if (d < min)
					d = min;
				if (d > max)
					d = max;
				delta->y = c - d;
			}
		}
		else
			delta->y = 0;
	}
	else {
		delta->x = 0;
		delta->y = 0;
	}
}

void KprScrollerLoop(KprScroller self, Boolean loopIt)
{
	KprContent content = self->first;
	if (loopIt)
		self->flags |= kprLooping;
	else
		self->flags &= ~kprLooping;
	if (content)
		KprContentReflow(content, kprPositionChanged);
}

void KprScrollerPredictBy(KprScroller self, SInt32 dx, SInt32 dy) 
{
	KprContent content = self->first;
	if (content) {
		FskRectangleRecord area = *KprBounds(self);
		if ((content->coordinates.horizontal & kprLeftRightWidth) == kprLeftRight) dx = 0;
		if ((content->coordinates.vertical & kprTopBottomHeight) == kprTopBottom) dy = 0;
		content->bounds.x -= dx;
		content->bounds.y -= dy;
		(*self->dispatch->predict)(self, &area);
		content->bounds.x += dx;
		content->bounds.y += dy;
	}
}

void KprScrollerPredictTo(KprScroller self, SInt32 x, SInt32 y) 
{
	KprScrollerPredictBy(self, x - self->delta.x, y - self->delta.y);
}

void KprScrollerReveal(KprScroller self, FskRectangle bounds) 
{
	SInt32 start, stop, min, max, x, y;
	start = bounds->y - self->delta.y;
	stop = start + bounds->height;
	min = 0;
	max = self->bounds.height;
	if (stop > max) {
		if ((start - (stop - max)) < min)
			y = start - min;
		else
			y = stop - max;
	}
	else if (start < min)
		y = start - min;
	else
		y = 0;
	start = bounds->x - self->delta.x;
	stop = start + bounds->width;
	min = 0;
	max = self->bounds.width;
	if (stop > max) {
		if ((start - (stop - max)) < min)
			x = start - min;
		else
			x = stop - max;
	}
	else if (start < min)
		x = start - min;
	else
		x = 0;
	if (x || y)
		KprScrollerScrollBy(self, x, y);
}

void KprScrollerScrollBy(KprScroller self, SInt32 dx, SInt32 dy) 
{
	KprContent content = self->first;
	UInt32 flags = 0;
	if (content) {
		if ((content->coordinates.horizontal & kprLeftRightWidth) == kprLeftRight) dx = 0;
		if ((content->coordinates.vertical & kprTopBottomHeight) == kprTopBottom) dy = 0;
		if (self->flags & kprClip)
			KprContentInvalidate((KprContent)self);
		else
			KprContentInvalidate(content);
	}
	if (dx) {
		flags |= kprXChanged;
		self->delta.x += dx;
	}
	if (dy) {
		flags |= kprYChanged;
		self->delta.y += dy;
	}
	if (content && flags)
		KprContentReflow(content, kprPositionChanged);
}

void KprScrollerScrollTo(KprScroller self, SInt32 x, SInt32 y) 
{
	KprContent content = self->first;
	UInt32 flags = 0;
	if (content) {
		if ((content->coordinates.horizontal & kprLeftRightWidth) == kprLeftRight) x = 0;
		if ((content->coordinates.vertical & kprTopBottomHeight) == kprTopBottom) y = 0;
		if (self->flags & kprClip)
			KprContentInvalidate((KprContent)self);
		else
			KprContentInvalidate(content);
	}
	if (self->delta.x != x) {
		flags |= kprXChanged;
		self->delta.x = x;
	}
	if (self->delta.y != y) {
		flags |= kprYChanged;
		self->delta.y = y;
	}
	if (content && flags)
		KprContentReflow(content, flags);
}

/* SCROLLER DISPATCH */

void KprScrollerFitHorizontally(void* it) 
{
	KprScroller self = it;
	KprContent content = self->first;
	KprContainerFitHorizontally(it);
	if (content)
		self->flags |= kprXScrolled;
}

void KprScrollerFitVertically(void* it) 
{
	KprScroller self = it;
	KprContent content = self->first;
	KprContainerFitVertically(it);
	if (content)
		self->flags |= kprYScrolled;
}

KprContent KprScrollerHit(void* it, SInt32 x, SInt32 y) 
{
	KprContainer self = it;
	KprContent content;
	KprContent result;
	if (self->flags & kprVisible) {
		Boolean hit = ((0 <= x) && (0 <= y) && (x < self->bounds.width) && (y < self->bounds.height));
		if (!hit && (self->flags & kprClip))
			return NULL;
		content = self->last;
		while (content) {
			result = (*content->dispatch->hit)(content, x - content->bounds.x, y - content->bounds.y);
			if (result)
				return result;
			content = content->previous;
		}
		if (hit && (self->flags & kprActive))
			return (KprContent)self;
	}
	return NULL;
}

void KprScrollerMeasureHorizontally(void* it) 
{
	KprScroller self = it;
	KprContent content = self->first;
	UInt16 horizontal = self->coordinates.horizontal;
	if (content) {
		KprContainerMeasureHorizontally(it);
		if (!(horizontal & kprWidth)) 
			self->coordinates.width = content->coordinates.width;
	}
}

void KprScrollerMeasureVertically(void* it) 
{
	KprScroller self = it;
	KprContent content = self->first;
	UInt16 vertical = self->coordinates.vertical;
	if (content) {
		KprContainerMeasureVertically(it);
		if (!(vertical & kprHeight))
			self->coordinates.height = content->coordinates.height;
	}
}

void KprScrollerPlace(void* it) 
{
	KprScroller self = it;
	KprContent content = self->first;
	if (self->flags & kprScrolled) {
		if (content) {
            if (content->flags & kprPlaced) {
                if (!(self->flags & kprTracking) || (self->flags & kprLooping))
               		KprScrollerConstraint(self, &self->delta);
                if (self->flags & kprXScrolled)
                    content->bounds.x -= self->delta.x;
                if (self->flags & kprYScrolled)
                    content->bounds.y -= self->delta.y;
                kprDelegateScrolled(self);
                while (content) {
                    kprDelegateScrolled(content);
                    content = content->next;
                }
                self->flags &= ~kprScrolled;
            }
        }
        else
            self->flags &= ~kprScrolled;
	}
	KprContainerPlace(it);
}

void KprScrollerPredict(void* it, FskRectangle area) 
{
	KprContainer self = it;
	if (self->flags & kprVisible) {
		KprContent content = self->first;
		if (content) {
			FskRectangleRecord clip = *area;
			FskRectangleOffset(&clip, -self->bounds.x, -self->bounds.y);
			if (self->flags & kprClip) {
				FskRectangleRecord bounds;
				FskRectangleSet(&bounds, 0, 0, self->bounds.width, self->bounds.height);
				FskRectangleIntersect(&clip, &bounds, &clip);
			}
			(*content->dispatch->predict)(content, &clip);
			if (self->flags & kprLooping) {
				content = self->first;
				if (content->bounds.width > self->bounds.width)
					content->bounds.x += content->bounds.width;
				if (content->bounds.height > self->bounds.height)
					content->bounds.y += content->bounds.height;
				(*content->dispatch->predict)(content, &clip);
				if (content->bounds.width > self->bounds.width)
					content->bounds.x -= content->bounds.width;
				if (content->bounds.height > self->bounds.height)
					content->bounds.y -= content->bounds.height;
			}
			while (content) {
				(*content->dispatch->predict)(content, &clip);
				content = content->next;
			}
		}
	}
}

void KprScrollerReflowing(void* it, UInt32 flags)
{
	KprContainer self = it;
	KprContent content = self->first;
	if (content) {
		if (flags & kprHorizontallyChanged)
			self->flags |= kprXScrolled;
		if (flags & kprVerticallyChanged)
			self->flags |= kprYScrolled;
	}
	KprContainerReflowing(it, flags);
}


void KprScrollerUpdate(void* it, FskPort port, FskRectangle area) 
{
	KprContainer self = it;
	if (self->flags & kprVisible) {
		KprContent content = self->first;
		FskRectangleRecord oldClip, newClip;
		FskPortOffsetOrigin(port, self->bounds.x, self->bounds.y);
		FskRectangleOffset(area, -self->bounds.x, -self->bounds.y);
		(*self->dispatch->draw)(self, port, area);
		if (content) {
			FskRectangleRecord bounds;
			FskRectangleSet(&bounds, 0, 0, self->bounds.width, self->bounds.height);
			if (self->flags & kprClip) {
				FskPortGetClipRectangle(port, &oldClip);
				FskRectangleIntersect(&oldClip, &bounds, &newClip);
				if (FskRectangleIsIntersectionNotEmpty(&newClip, area)) {
					FskPortSetClipRectangle(port, &newClip);
					FskRectangleIntersect(area, &newClip, &bounds);
					(*content->dispatch->update)(content, port, &bounds);
					if (self->flags & kprLooping) {
						content = self->first;
						if (content->bounds.width > self->bounds.width)
							content->bounds.x += content->bounds.width;
						if (content->bounds.height > self->bounds.height)
							content->bounds.y += content->bounds.height;
						(*content->dispatch->update)(content, port, area);
						if (content->bounds.width > self->bounds.width)
							content->bounds.x -= content->bounds.width;
						if (content->bounds.height > self->bounds.height)
							content->bounds.y -= content->bounds.height;
					}
					content = content->next;
					while (content) {
						(*content->dispatch->update)(content, port, &bounds);
						content = content->next;
					}
					FskPortSetClipRectangle(port, &oldClip);
				}
			}
			else {
				(*content->dispatch->update)(content, port, area);
				if (self->flags & kprLooping) {
					content = self->first;
					if (content->bounds.width > self->bounds.width)
						content->bounds.x += content->bounds.width;
					if (content->bounds.height > self->bounds.height)
						content->bounds.y += content->bounds.height;
					(*content->dispatch->update)(content, port, area);
					if (content->bounds.width > self->bounds.width)
						content->bounds.x -= content->bounds.width;
					if (content->bounds.height > self->bounds.height)
						content->bounds.y -= content->bounds.height;
				}
				content = content->next;
				while (content) {
					(*content->dispatch->update)(content, port, area);
					content = content->next;
				}
			}
		}
		FskRectangleOffset(area, self->bounds.x, self->bounds.y);
		FskPortOffsetOrigin(port, -self->bounds.x, -self->bounds.y);
	}
	if (self->flags & kprDisplayed) {
		if (self->shell->port == port) {
			self->flags &= ~kprDisplayed;
			kprDelegateDisplayed(self);
		}
	}
}

/* ECMASCRIPT */

void KPR_Scroller(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprCoordinatesRecord coordinates;
	KprSkin skin = NULL;
	KprStyle style = NULL;
	KprScroller self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	if ((c > 1) && xsTest(xsArg(1)))
		skin = kprGetHostData(xsArg(1), skin, skin);
	if ((c > 2) && xsTest(xsArg(2)))
		style = kprGetHostData(xsArg(2), skin, style);
	xsThrowIfFskErr(KprScrollerNew(&self, &coordinates, skin, style));
	kprContentConstructor(KPR_Scroller);
}

void KPR_scroller_get_content(xsMachine *the)
{
	KprScroller self = xsGetHostData(xsThis);
	xsResult = kprContentGetter(self->first);
}

void KPR_scroller_get_constraint(xsMachine *the)
{
	KprScroller self = xsGetHostData(xsThis);
	FskPointRecord delta;
	KprScrollerConstraint(self, &delta);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_x, xsInteger(delta.x), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsInteger(delta.y), xsDefault, xsDontScript);
}

void KPR_scroller_get_loop(xsMachine *the)
{
	KprScroller self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprLooping) ? xsTrue : xsFalse;
}

void KPR_scroller_get_scroll(xsMachine *the)
{
	KprScroller self = xsGetHostData(xsThis);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_x, xsInteger(self->delta.x), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsInteger(self->delta.y), xsDefault, xsDontScript);
}

void KPR_scroller_get_tracking(xsMachine *the)
{
	KprScroller self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprTracking) ? xsTrue : xsFalse;
}

void KPR_scroller_set_content(xsMachine *the)
{
	KprScroller self = xsGetHostData(xsThis);
	KprContent content = NULL;
	if (xsTest(xsArg(0))) {
		content = kprGetHostData(xsArg(0), it, content);
		xsAssert(content->container == NULL);
	}
	if (self->first) {
		if (content)
			KprContainerReplace((KprContainer)self, self->first, content);
		else
			KprContainerRemove((KprContainer)self, self->first);
	}
	else if (content)
		KprContainerAdd((KprContainer)self, content);
}

void KPR_scroller_set_loop(xsMachine *the)
{
	KprScroller self = kprGetHostData(xsThis, this, scroller);
	KprScrollerLoop(self, xsToBoolean(xsArg(0)));
}

void KPR_scroller_set_scroll(xsMachine *the)
{
	KprScroller self = xsGetHostData(xsThis);
	SInt32 x = 0, y = 0;
	xsEnterSandbox();
	if (xsFindInteger(xsArg(0), xsID_x, &x))
		x -= self->delta.x;
	if (xsFindInteger(xsArg(0), xsID_y, &y))
		y -= self->delta.y;
	xsLeaveSandbox();
	KprScrollerScrollBy(self, x, y);
}

void KPR_scroller_set_tracking(xsMachine *the)
{
	KprScroller self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprTracking;
	else
		self->flags &= ~kprTracking;
}

void KPR_scroller_reveal(xsMachine *the)
{
	KprScroller self = kprGetHostData(xsThis, this, scroller);
	FskRectangleRecord bounds;
	xsEnterSandbox();
	if (xsFindInteger(xsArg(0), xsID_x, &bounds.x))
		if (xsFindInteger(xsArg(0), xsID_y, &bounds.y))
			if (xsFindInteger(xsArg(0), xsID_width, &bounds.width))
				if (xsFindInteger(xsArg(0), xsID_height, &bounds.height))
					KprScrollerReveal(self, &bounds);
	xsLeaveSandbox();
}

void KPR_scroller_predictBy(xsMachine *the)
{
	KprScroller self = kprGetHostData(xsThis, this, scroller);
	xsIntegerValue x = xsToInteger(xsArg(0));
	xsIntegerValue y = xsToInteger(xsArg(1));
	KprScrollerPredictBy(self, x, y);
}

void KPR_scroller_predictTo(xsMachine *the)
{
	KprScroller self = kprGetHostData(xsThis, this, scroller);
	xsIntegerValue x = xsToInteger(xsArg(0));
	xsIntegerValue y = xsToInteger(xsArg(1));
	KprScrollerPredictTo(self, x, y);
}

void KPR_scroller_scrollBy(xsMachine *the)
{
	KprScroller self = kprGetHostData(xsThis, this, scroller);
	xsIntegerValue x = xsToInteger(xsArg(0));
	xsIntegerValue y = xsToInteger(xsArg(1));
	KprScrollerScrollBy(self, x, y);
}

void KPR_scroller_scrollTo(xsMachine *the)
{
	KprScroller self = kprGetHostData(xsThis, this, scroller);
	xsIntegerValue x = xsToInteger(xsArg(0));
	xsIntegerValue y = xsToInteger(xsArg(1));
	KprScrollerScrollTo(self, x, y);
}

