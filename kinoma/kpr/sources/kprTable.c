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
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprSkin.h"
#include "kprShell.h"
#include "kprTable.h"

#include <math.h>

/* VERTICAL TABLE */

static void KprColumnFitVertically(void* it);
static void KprColumnMeasureVertically(void* it);
static void KprColumnReflowing(void* it, UInt32 flags);
static void KprColumnRemoving(void* it, KprContent content);
static void KprLineFitHorizontally(void* it);
static void KprLineMeasureHorizontally(void* it);
static void KprLineReflowing(void* it, UInt32 flags);
static void KprLineRemoving(void* it, KprContent content);

/* COLUMN */

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprColumnInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprColumn", FskInstrumentationOffset(KprColumnRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprColumnDispatchRecord = {
	"column",
	KprContainerActivated,
	KprContainerAdded,
	KprContainerCascade,
	KprContainerDispose,
	KprContentDraw,
	KprContainerFitHorizontally,
	KprColumnFitVertically,
	KprContentGetBitmap,
	KprContainerHit,
	KprContentIdle,
	KprContainerInvalidated,
	KprContainerLayered,
	KprContainerMark,
	KprContainerMeasureHorizontally,
	KprColumnMeasureVertically,
	KprContainerPlace,
	KprContainerPlaced,
	KprContainerPredict,
	KprColumnReflowing,
	KprColumnRemoving,
	KprContainerSetWindow,
	KprContainerShowing,
	KprContainerShown,
	KprContainerUpdate
};

FskErr KprColumnNew(KprColumn* it,  KprCoordinates coordinates, KprSkin skin, KprStyle style)
{
	FskErr err = kFskErrNone;
	KprColumn self;
	bailIfError(FskMemPtrNewClear(sizeof(KprColumnRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprColumnInstrumentation);
	self->dispatch = &KprColumnDispatchRecord;
	self->flags = kprContainer | kprVertical | kprVisible;
	KprContentInitialize((KprContent)self, coordinates, skin, style);
bail:
	return err;
}

/* COLUMN DISPATCH */

void KprColumnFitVertically(void* it) 
{
	KprColumn self = it;
	SInt32 portion = self->portion, sum = self->sum, y = 0;
	KprContent content = self->first;
	KprContentFitVertically(it);
	if (portion) {
		SInt32 slop = self->bounds.height - sum;
		SInt32 step = slop / portion;
		while (content) {
			FskRectangle bounds = &content->bounds;
			KprCoordinates coordinates = &content->coordinates;
			if ((coordinates->vertical & kprTopBottom) == kprTopBottom) {
				portion--;
				if (portion) {
					bounds->height = step + coordinates->height;
					slop -= step;
				}
				else
					bounds->height = slop + coordinates->height;
			}
			(*content->dispatch->fitVertically)(content);
			y += coordinates->top;
			bounds->y = y;
			y += bounds->height + coordinates->bottom;
			content->flags &= ~kprYChanged;
			KprContentPlacing((KprContent)self);
			content = content->next;
		}
	}
	else {
		while (content) {
			FskRectangle bounds = &content->bounds;
			KprCoordinates coordinates = &content->coordinates;
			(*content->dispatch->fitVertically)(content);
			y += coordinates->top;
			bounds->y = y;
			y += bounds->height + coordinates->bottom;
			content->flags &= ~kprYChanged;
			KprContentPlacing((KprContent)self);
			content = content->next;
		}
	}
	self->flags |= kprContentsPlaced;
}

void KprColumnMeasureVertically(void* it) 
{
	KprColumn self = it;
	SInt32 portion = 0, sum = 0;
	KprContent content = self->first;
	UInt16 vertical = self->coordinates.vertical;
	while (content) {
		KprCoordinates coordinates = &content->coordinates;
		if ((coordinates->vertical & kprTopBottom) == kprTopBottom)
			portion++;
		(*content->dispatch->measureVertically)(content);
		sum += coordinates->height + coordinates->top + coordinates->bottom;
		content = content->next;
	}
	self->portion = portion;
	self->sum = sum;
	if (!(vertical & kprHeight))
		self->coordinates.height = sum;
}

void KprColumnReflowing(void* it, UInt32 flags)
{
	KprContainer self = it;
	KprContainer container = self->container;
	if (flags & kprHorizontallyChanged) {
		if( (self->coordinates.horizontal & kprLeftRightWidth) < kprLeftRight)
			self->flags |= kprWidthChanged;
		else
			self->flags |= kprContentsHorizontallyChanged;
	}
	else if (flags & kprContentsHorizontallyChanged)
		self->flags |= kprContentsHorizontallyChanged;
	if (flags & kprVerticallyChanged)
		self->flags |= kprHeightChanged;
	else if (flags & kprContentsVerticallyChanged)
		self->flags |= kprContentsVerticallyChanged;
	self->flags |= kprContentsPlaced;
	if (container)
		(*container->dispatch->reflowing)(container, self->flags);
}

void KprColumnRemoving(void* it, KprContent content) 
{
	KprContainer self = it;
	if (self->shell) {
		UInt32 flags = kprHeightChanged;
		if ((self->coordinates.horizontal & kprLeftRightWidth) < kprLeftRight)
			flags |= kprWidthChanged;
		KprContentReflow(it, flags);
		(*content->dispatch->setWindow)(content, NULL, NULL);
	}
}

/* LINE */

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprLineInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprLine", FskInstrumentationOffset(KprLineRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprLineDispatchRecord = {
	"line",
	KprContainerActivated,
	KprContainerAdded,
	KprContainerCascade,
	KprContainerDispose,
	KprContentDraw,
	KprLineFitHorizontally,
	KprContainerFitVertically,
	KprContentGetBitmap,
	KprContainerHit,
	KprContentIdle,
	KprContainerInvalidated,
	KprContainerLayered,
	KprContainerMark,
	KprLineMeasureHorizontally,
	KprContainerMeasureVertically,
	KprContainerPlace,
	KprContainerPlaced,
	KprContainerPredict,
	KprLineReflowing,
	KprLineRemoving,
	KprContainerSetWindow,
	KprContainerShowing,
	KprContainerShown,
	KprContainerUpdate
};

FskErr KprLineNew(KprLine* it,  KprCoordinates coordinates, KprSkin skin, KprStyle style)
{
	FskErr err = kFskErrNone;
	KprLine self;
	bailIfError(FskMemPtrNewClear(sizeof(KprLineRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprLineInstrumentation);
	self->dispatch = &KprLineDispatchRecord;
	self->flags = kprContainer | kprHorizontal | kprVisible;
	KprContentInitialize((KprContent)self, coordinates, skin, style);
bail:
	return err;
}

/* LINE DISPATCH */

void KprLineFitHorizontally(void* it) 
{
	KprLine self = it;
	SInt32 portion = self->portion, sum = self->sum, x = 0;
	KprContent content = self->first;
	KprContentFitHorizontally(it);
	if (portion) {
		SInt32 slop = self->bounds.width - sum;
		SInt32 step = slop / portion;
		while (content) {
			FskRectangle bounds = &content->bounds;
			KprCoordinates coordinates = &content->coordinates;
			if ((coordinates->horizontal & kprLeftRight) == kprLeftRight) {
				portion--;
				if (portion) {
					bounds->width = step + coordinates->width;
					slop -= step;
				}
				else
					bounds->width = slop + coordinates->width;
			}
			(*content->dispatch->fitHorizontally)(content);
			x += coordinates->left;
			bounds->x = x;
			x += bounds->width + coordinates->right;
			content->flags &= ~kprXChanged;
			KprContentPlacing((KprContent)self);
			content = content->next;
		}
	}
	else {
		while (content) {
			FskRectangle bounds = &content->bounds;
			KprCoordinates coordinates = &content->coordinates;
			(*content->dispatch->fitHorizontally)(content);
			x += coordinates->left;
			bounds->x = x;
			x += bounds->width + coordinates->right;
			content->flags &= ~kprXChanged;
			KprContentPlacing((KprContent)self);
			content = content->next;
		}
	}
	self->flags |= kprContentsPlaced;
}

void KprLineMeasureHorizontally(void* it) 
{
	KprLine self = it;
	SInt32 portion = 0, sum = 0;
	KprContent content = self->first;
	UInt16 horizontal = self->coordinates.horizontal;
	while (content) {
		KprCoordinates coordinates = &content->coordinates;
		if ((coordinates->horizontal & kprLeftRight) == kprLeftRight)
			portion++;
		(*content->dispatch->measureHorizontally)(content);
		sum += coordinates->width + coordinates->left + coordinates->right;
		content = content->next;
	}
	self->portion = portion;
	self->sum = sum;
	if (!(horizontal & kprWidth))
		self->coordinates.width = sum;
}

void KprLineReflowing(void* it, UInt32 flags)
{
	KprContainer self = it;
	KprContainer container = self->container;
	if (flags & kprHorizontallyChanged)
		self->flags |= kprWidthChanged;
	else if (flags & kprContentsHorizontallyChanged)
		self->flags |= kprContentsHorizontallyChanged;
	if (flags & kprVerticallyChanged) {
		if ((self->coordinates.vertical & kprTopBottomHeight) < kprTopBottom)
			self->flags |= kprHeightChanged;
		else
			self->flags |= kprContentsVerticallyChanged;
	}
	else if (flags & kprContentsVerticallyChanged)
		self->flags |= kprContentsVerticallyChanged;
	self->flags |= kprContentsPlaced;
	if (container)
		(*container->dispatch->reflowing)(container, self->flags);
}

void KprLineRemoving(void* it, KprContent content) 
{
	KprContainer self = it;
	if (self->shell) {
		UInt32 flags = kprWidthChanged;
		if ((self->coordinates.vertical & kprTopBottomHeight) < kprTopBottom)
			flags |= kprHeightChanged;
		KprContentReflow(it, flags);
		(*content->dispatch->setWindow)(content, NULL, NULL);
	}
}

/* ECMASCRIPT */

void KPR_Column(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprCoordinatesRecord coordinates;
	KprSkin skin = NULL;
	KprStyle style = NULL;
	KprColumn self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	if ((c > 1) && xsTest(xsArg(1)))
		skin = kprGetHostData(xsArg(1), skin, skin);
	if ((c > 2) && xsTest(xsArg(2)))
		style = kprGetHostData(xsArg(2), skin, style);
	xsThrowIfFskErr(KprColumnNew(&self, &coordinates, skin, style));
	kprContentConstructor(KPR_Column);
}

void KPR_Line(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprCoordinatesRecord coordinates;
	KprSkin skin = NULL;
	KprStyle style = NULL;
	KprLine self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	if ((c > 1) && xsTest(xsArg(1)))
		skin = kprGetHostData(xsArg(1), skin, skin);
	if ((c > 2) && xsTest(xsArg(2)))
		style = kprGetHostData(xsArg(2), skin, style);
	xsThrowIfFskErr(KprLineNew(&self, &coordinates, skin, style));
	kprContentConstructor(KPR_Line);
}

