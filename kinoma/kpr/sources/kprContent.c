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

#include "FskEvent.h"
#include "FskPort.h"
#include "FskUtilities.h"

#include "kprSkin.h"
#include "kprStyle.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprLayer.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprTransition.h"
#include "kprUtilities.h"

/* COORDINATES */

void KprCoordinatesSet(KprCoordinates coordinates,
	UInt16 horizontal, SInt32 left, SInt32 width, SInt32 right,
	UInt16 vertical, SInt32 top, SInt32 height, SInt32 bottom)
{
	coordinates->horizontal = horizontal;
	coordinates->left = horizontal & kprLeft ? left : 0;
	coordinates->width = horizontal & kprWidth ? width : 0;
	coordinates->right = horizontal & kprRight ? right : 0;
	coordinates->vertical = vertical;
	coordinates->top = vertical & kprTop ? top : 0;
	coordinates->height = vertical & kprHeight ? height : 0;
	coordinates->bottom = vertical & kprBottom ? bottom : 0;
}

void xsSlotToKprCoordinates(xsMachine *the, xsSlot* slot, KprCoordinates coordinates)
{
	coordinates->horizontal = kprCenter;
	coordinates->vertical = kprMiddle;
	if (xsTest(*slot)) {
		xsEnterSandbox();
		if (xsFindInteger(*slot, xsID_left, &coordinates->left))
			coordinates->horizontal |= kprLeft;
		else
			coordinates->left = 0;
		if (xsFindInteger(*slot, xsID_width, &coordinates->width))
			coordinates->horizontal |= kprWidth;
		else
			coordinates->width = 0;
		if (xsFindInteger(*slot, xsID_right, &coordinates->right))
			coordinates->horizontal |= kprRight;
		else
			coordinates->right = 0;
		if (xsFindInteger(*slot, xsID_top, &coordinates->top))
			coordinates->vertical |= kprTop;
		else
			coordinates->top = 0;
		if (xsFindInteger(*slot, xsID_height, &coordinates->height))
			coordinates->vertical |= kprHeight;
		else
			coordinates->height = 0;
		if (xsFindInteger(*slot, xsID_bottom, &coordinates->bottom))
			coordinates->vertical |= kprBottom;
		else
			coordinates->bottom = 0;
		xsLeaveSandbox();
	}
	else {
		coordinates->left = 0;
		coordinates->width = 0;
		coordinates->right = 0;
		coordinates->top = 0;
		coordinates->height = 0;
		coordinates->bottom = 0;
	}
}

void xsSlotFromKprCoordinates(xsMachine *the, xsSlot* slot, KprCoordinates coordinates)
{
	UInt16 horizontal = coordinates->horizontal;
	UInt16 vertical = coordinates->vertical;
	*slot = xsNewInstanceOf(xsObjectPrototype);
	if (horizontal & kprLeft)
		xsNewHostProperty(*slot, xsID_left, xsInteger(coordinates->left), xsDefault, xsDontScript);
	if (horizontal & kprWidth)
		xsNewHostProperty(*slot, xsID_width, xsInteger(coordinates->width), xsDefault, xsDontScript);
	if (horizontal & kprRight)
		xsNewHostProperty(*slot, xsID_right, xsInteger(coordinates->right), xsDefault, xsDontScript);
	if (vertical & kprTop)
		xsNewHostProperty(*slot, xsID_top, xsInteger(coordinates->top), xsDefault, xsDontScript);
	if (vertical & kprHeight)
		xsNewHostProperty(*slot, xsID_height, xsInteger(coordinates->height), xsDefault, xsDontScript);
	if (vertical & kprBottom)
		xsNewHostProperty(*slot, xsID_bottom, xsInteger(coordinates->bottom), xsDefault, xsDontScript);
}

void KprMarginsApply(KprMargins margins, FskRectangle src, FskRectangle dst)
{
	SInt32 dl, dt, dr, db;
	dl = margins->left;
	dt = margins->top;
	dr = margins->right;
	db = margins->bottom;
	FskRectangleSet(dst, src->x + dl, src->y + dt, src->width - dl - dr, src->height - dt - db);
}

/* CONTENT */

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprContentInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprContent", FskInstrumentationOffset(KprContentRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprContentDispatchRecord = {
	"content",
	KprContentActivated,
	KprContentAdded,
	KprContentCascade,
	KprContentDispose,
	KprContentDraw,
	KprContentFitHorizontally,
	KprContentFitVertically,
	KprContentGetBitmap,
	KprContentHit,
	KprContentIdle,
	KprContentInvalidated,
	KprContentLayered,
	KprContentMark,
	KprContentMeasureHorizontally,
	KprContentMeasureVertically,
	KprContentPlace,
	KprContentPlaced,
	KprContentPredict,
	KprContentReflowing,
	KprContentRemoving,
	KprContentSetWindow,
	KprContentShowing,
	KprContentShown,
	KprContentUpdate
};

FskErr KprContentNew(KprContent *it, KprCoordinates coordinates, KprSkin skin, KprStyle style)
{
	FskErr err = kFskErrNone;
	KprContent self;
	bailIfError(FskMemPtrNewClear(sizeof(KprContentRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprContentInstrumentation);
	self->dispatch = &KprContentDispatchRecord;
	self->flags = kprVisible;
	KprContentInitialize((KprContent)self, coordinates, skin, style);
bail:
	return err;
}

void KprContentCancel(KprContent self) 
{
	if (self->flags & kprMessaging) {
		FskList messages = gShell->messages;
		KprMessage message = FskListGetNext(messages, NULL);
		while (message) {
			if (message->request.target == self)
				KprMessageCancel(message);
			message = FskListGetNext(messages, message);
		}
		self->flags &= ~kprMessaging;
	}
}

void KprContentClose(KprContent self)
{
	FskInstrumentedItemSendMessageNormal(self, kprInstrumentedContentClose, self);
	if (self->the) {
		self->container = NULL;
		FskInstrumentedItemClearOwner(self);
		self->previous = NULL;
		self->next = NULL;
	}
	else {
		KprContent current = (self->flags & kprContainer) ? ((KprContainer)self)->first : NULL;
		while (current) {
			KprContent next = current->next;
			KprContentClose(current);
			current = next;
		}
		(*self->dispatch->dispose)(self);
	}
}

void KprContentComplete(KprMessage message, void* it)
{
    KprContent self = it;
	FskList messages = gShell->messages;
    kprDelegateComplete(self, message);
	message = FskListGetNext(messages, NULL);
	while (message) {
		if (message->request.target == self)
			return;
		message = FskListGetNext(messages, message);
	}
	self->flags &= ~kprMessaging;
}

Boolean KprContentFindFocus(KprContent self, UInt32 axis, SInt32 delta, KprContent current)
{
	if (self->shell) {
		if (self->flags & kprContainer) {
			if (delta > 0) {
				KprContent content;
				if (current) {
					if (axis && ((self->flags & (kprHorizontal | kprVertical)) == axis))
						content = current->next;
					else
						content = NULL;
				}
				else
					content = ((KprContainer)self)->first;
				while (content) {
					if (KprContentFindFocus(content, axis, delta, NULL))
						return true;
					content = content->next;
				}
			}
			else {
				KprContent content;
				if (current) {
					if (axis && ((self->flags & (kprHorizontal | kprVertical)) == axis))
						content = current->previous;
					else
						content = NULL;
				}
				else
					content = ((KprContainer)self)->last;
				while (content) {
					if (KprContentFindFocus(content, axis, delta, NULL))
						return true;
					content = content->previous;
				}
			}
		}
		if (current) {
			if (self->container)
				return KprContentFindFocus((KprContent)self->container, axis, delta, self);
			if (!axis)
				return KprContentFindFocus(self, axis, delta, NULL);
		}
		if (self->flags & kprFocusable) {
			KprShellSetFocus(self->shell, self);
			return true;
		}
	}	
	return false;
}

void KprContentFromWindowCoordinates(KprContent self, SInt32 x0, SInt32 y0, SInt32 *x1, SInt32 *y1)
{
	KprContainer container = self->container;
	x0 -= self->bounds.x;
	y0 -= self->bounds.y;
	while (container) {
		x0 -= container->bounds.x;
		y0 -= container->bounds.y;
		container = container->container;
	}
	*x1 = x0;
	*y1 = y0;
}

KprStyle KprContentGetStyle(KprContent self)
{
	if (self->flags & kprHasOwnStyle)
		return KprStyleUncascade(self->style);
	return NULL;
}

SInt32 KprContentIndex(KprContent self)
{
	KprContainer container = self->container;
	if (container) {
		SInt32 count = 0;
		KprContent content = container->first;
		while (content) {
			if (content == self)
				return count;
			count++;
			content = content->next;
		}
	}
	return -1;
}

void KprContentInitialize(KprContent self, KprCoordinates coordinates, KprSkin skin, KprStyle style)
{
	self->coordinates = *coordinates;
	if (skin) {
		self->skin = skin;
		KprAssetBind(skin);
	}
	if (style) {
		self->style = style;
		KprAssetBind(style);
		self->flags |= kprHasOwnStyle;
	}
	self->interval = 1;
}

void KprContentInvalidate(KprContent self)
{
	(*self->dispatch->invalidated)(self, NULL);
}

Boolean KprContentIsFocus(KprContent self)
{
	return (self->shell && (self->shell->focus == self));
}

Boolean KprContentIsShown(KprContent self)
{
	if (self->shell == (KprShell)self)
		return true;
	if ((self->flags & kprVisible) && self->container)
		return KprContentIsShown((KprContent)self->container);
	return false;
}

void KprContentMoveBy(KprContent self, SInt32 dx, SInt32 dy)
{
	UInt16 horizontal = self->coordinates.horizontal & kprLeftRightWidth;
	UInt16 vertical = self->coordinates.vertical & kprTopBottomHeight;
	if ((horizontal == kprLeftRight) || (horizontal == kprWidth)) dx = 0;
	if ((vertical == kprTopBottom) || (vertical == kprHeight)) dy = 0;
	if (dx || dy) {
		KprContainer container = self->container;
		if (container)
			KprContentInvalidate(self);
		if (horizontal & kprLeft)
			self->coordinates.left += dx;
		else
			self->coordinates.right -= dx;
		if (vertical & kprTop)
			self->coordinates.top += dy;
		else
			self->coordinates.bottom -= dy;
		if (container) {
			UInt32 flags = 0;
			if (dx) flags |= kprXChanged;
			if (dy) flags |= kprYChanged;
			KprContentReflow(self, flags);
		}
	}
}

void KprContentPlacing(KprContent self)
{
	if (!(self->flags & kprPlaced)) {
		KprContainer container = self->container;
		while (container) {
			if (container->flags & kprContentsPlaced)
				break;
			container->flags |= kprContentsPlaced;
			container = container->container;
		}
		self->flags |= kprPlaced;
	}
}

void KprContentReflow(KprContent self, UInt32 flags)
{
	KprContainer container = self->container;
	self->flags |= flags;
	if (flags & (kprPositionChanged | kprSizeChanged))
		self->flags &= ~kprPlaced;
	if (container)
		(*container->dispatch->reflowing)(container, flags);
	else if (self->shell)
		(*self->dispatch->reflowing)(self, flags);
}

void KprContentSetBehavior(KprContent self, KprBehavior behavior)
{
	kprDelegateDispose(self);
	self->behavior = behavior;
}

void KprContentSetCoordinates(void* it, KprCoordinates coordinates) 
{
	KprContent self = it;
	UInt16 horizontal = self->coordinates.horizontal;
	UInt16 vertical = self->coordinates.vertical;
	Boolean flag;
	if (horizontal != coordinates->horizontal) 
		flag = true;
	else if (vertical != coordinates->vertical)
		flag = true;
	else if ((horizontal & kprLeft) && (self->coordinates.left != coordinates->left))
		flag = true;
	else if ((horizontal & kprRight) && (self->coordinates.right != coordinates->right))
		flag = true;
	else if ((horizontal & kprWidth) && (self->coordinates.width != coordinates->width))
		flag = true;
	else if ((vertical & kprTop) && (self->coordinates.top != coordinates->top))
		flag = true;
	else if ((vertical & kprBottom) && (self->coordinates.bottom != coordinates->bottom))
		flag = true;
	else if ((vertical & kprHeight) && (self->coordinates.height != coordinates->height))
		flag = true;
	else
		flag = false;
	if (flag) {
		KprContainer container = self->container;
		if (container)
			KprContentInvalidate(self);
		self->coordinates = *coordinates;
		if (container)
			KprContentReflow(self, kprSizeChanged);
	}
}

void KprContentSetDuration(KprContent self, double duration)
{
	if (duration < 0)
		duration = 0;
	self->duration = duration;
	KprContentSetTime(self, self->time);
}

void KprContentSetFraction(KprContent self, double fraction)
{
	double duration = self->duration;
	if (duration)
		KprContentSetTime(self, duration * fraction);
}

void KprContentSetInterval(KprContent self, double interval)
{
	if (interval < 1)
		interval = 1;
	self->interval = interval;
}

void KprContentSetSkin(KprContent self, KprSkin skin)
{
	KprAssetUnbind(self->skin);
	self->skin = skin;
	KprAssetBind(self->skin);
	KprContentReflow(self, kprSizeChanged);
}

void KprContentSetStyle(KprContent self, KprStyle style)
{
	KprAssetUnbind(self->style);
	self->style = style;
	if (style)
		self->flags |= kprHasOwnStyle;
	else
		self->flags &= ~kprHasOwnStyle;
	KprAssetBind(self->style);
	if (self->shell)
		(*self->dispatch->cascade)(self, self->container->style);
}

void KprContentSetTime(KprContent self, double time)
{
	double duration = self->duration;
	if (time < 0)
		time = 0;
	if (duration && (time > duration))
		time = duration;
	if (self->time != time) {
		self->time = time;
		kprDelegateTimeChanged(self);
	}
}

void KprContentShow(KprContent self, Boolean showIt)
{
	Boolean visible = (self->flags & kprVisible) ? true : false;
	if (visible != showIt) {
		Boolean flag = self->container ? KprContentIsShown((KprContent)self->container) : false;
		if (flag) 
			(*self->dispatch->showing)(self, showIt);
		if (showIt) {
			self->flags |= kprVisible;
			KprContentInvalidate(self);
		}
		else {
			KprContentInvalidate(self);
			self->flags &= ~kprVisible;
		}
		if (flag) 
			(*self->dispatch->shown)(self, showIt);
	}
}

void KprContentSizeBy(KprContent self, SInt32 dx, SInt32 dy)
{
	if (!(self->coordinates.horizontal & kprWidth)) dx = 0;
	if (!(self->coordinates.vertical & kprHeight)) dy = 0;
	if (dx || dy) {
		KprContainer container = self->container;
		if (container)
			KprContentInvalidate(self);
		self->coordinates.width += dx;
		self->coordinates.height += dy;
		if (container) {
			UInt32 flags = 0;
			if (dx) flags |= kprWidthChanged;
			if (dy) flags |= kprHeightChanged;
			KprContentReflow(self, flags);
		}
	}
}

void KprContentStart(KprContent self)
{
	if (!(self->flags & kprIdling)) {
		self->flags |= kprIdling;
		if (self->shell)
			KprShellStartIdling(self->shell, self);
	}
}

void KprContentStop(KprContent self)
{
	if (self->flags & kprIdling) {
		self->flags &= ~kprIdling;
		if (self->shell)
			KprShellStopIdling(self->shell, self);
	}
}

void KprContentToWindowCoordinates(KprContent self, SInt32 x0, SInt32 y0, SInt32 *x1, SInt32 *y1)
{
	KprContainer container = self->container;
	x0 += self->bounds.x;
	y0 += self->bounds.y;
	while (container) {
		x0 += container->bounds.x;
		y0 += container->bounds.y;
		container = container->container;
	}
	*x1 = x0;
	*y1 = y0;
}

/* CONTENT DISPATCH */

void KprContentActivated(void* it UNUSED, Boolean activateIt UNUSED) 
{
}

void KprContentAdded(void* it UNUSED, KprContent content UNUSED) 
{
	ASSERT(0);
}

void KprContentCascade(void* it, KprStyle style) 
{
	KprContent self = it;
	KprAssetUnbind(self->style);
	if (self->flags & kprHasOwnStyle)
		self->style = KprStyleCascade(KprStyleUncascade(self->style), style);
	else
		self->style = style;
	KprAssetBind(self->style);
}

void KprContentDispose(void* it) 
{
	KprContent self = it;
	KprContentCancel(self);
	kprDelegateDispose(self);
	FskMemPtrDispose(self->name);
	KprAssetUnbind(self->style);
	KprAssetUnbind(self->skin);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

void KprContentDraw(void* it, FskPort port, FskRectangle area UNUSED) 
{
	KprContent self = it;
	if (self->skin) {
		FskRectangleRecord bounds;
		FskRectangleSet(&bounds, 0, 0, self->bounds.width, self->bounds.height);
		if (bounds.width && bounds.height)
			KprSkinFill(self->skin, port, &bounds, self->variant, self->state, self->coordinates.horizontal, self->coordinates.vertical);
	}
}

void KprContentFitHorizontally(void* it) 
{
	KprContent self = it;
	if ((self->coordinates.horizontal & kprLeftRight) != kprLeftRight)
		self->bounds.width = self->coordinates.width;
	self->flags &= ~(kprWidthChanged | kprContentsHorizontallyChanged);
}

void KprContentFitVertically(void* it) 
{
	KprContent self = it;
	if ((self->coordinates.vertical & kprTopBottom) != kprTopBottom)
		self->bounds.height = self->coordinates.height;
	self->flags &= ~(kprHeightChanged | kprContentsVerticallyChanged);
}

FskBitmap KprContentGetBitmap(void* it, FskPort port, Boolean* owned)
{
	FskErr err = kFskErrNone;
	KprContent self = it;
	KprLayer layer = NULL;
	KprCoordinatesRecord coordinates = {0, 0, 0, 0, 0, 0, 0, 0};
	FskBitmap bitmap = NULL;
#if FSKBITMAP_OPENGL
	UInt32 flags = (port && FskBitmapIsOpenGLDestinationAccelerated(port->bits)) ? 0 : kprNoAcceleration;
#else
	UInt32 flags = 0;
#endif
	*owned = false;
	bailIfError(KprLayerNew(&layer, &coordinates, flags));
	if (self->shell) {
		KprLayerCapture(layer, self, &bitmap);
	}
	else {
		KprContainerAdd((KprContainer)gShell, self);
		KprShellAdjust(gShell);
		KprLayerCapture(layer, self, &bitmap);
		KprContainerRemove((KprContainer)gShell, self);
	}
bail:
	return bitmap;
}

KprContent KprContentHit(void* it, SInt32 x, SInt32 y) 
{
	KprContent self = it;
	if (self->flags & kprActive) {
		if ((0 <= x) && (0 <= y) && (x < self->bounds.width) && (y < self->bounds.height))
			return self;
	}
	return NULL;
}

void KprContentIdle(void* it, double interval)
{
	KprContent self = it;
	KprContentSetTime(self, self->time + interval);
	if (self->duration && (self->time == self->duration)) {
		KprContentStop(self);
		kprDelegateFinished(self);
	}
}

void KprContentInvalidated(void* it, FskRectangle area UNUSED) 
{
	KprContent self = it;
	if (self->flags & kprVisible) {
		KprContainer container = self->container;
		if (container) {
			FskRectangleRecord bounds;
			FskRectangleSet(&bounds, self->bounds.x, self->bounds.y, self->bounds.width, self->bounds.height);
			if (self->skin)
				KprSkinExtend(self->skin, &bounds);
			(*container->dispatch->invalidated)(container, &bounds);
		}
	}
}

void KprContentLayered(void* it UNUSED, KprLayer layer UNUSED, Boolean layerIt UNUSED)
{
}

void KprContentMark(void* it, xsMarkRoot markRoot)
{
	KprContent self = it;
	kprDelegateMark(self, markRoot);
}

void KprContentMeasureHorizontally(void* it) 
{
	KprContent self = it;
	UInt16 horizontal = self->coordinates.horizontal;
	if (!(horizontal & kprWidth)) {
		KprSkin skin = self->skin;
		if (skin) {
			FskRectangleRecord bounds;
			KprSkinMeasure(skin, &bounds);
			self->coordinates.width = bounds.width;
		}
		else
			self->coordinates.width = 0;
	}
}

void KprContentMeasureVertically(void* it) 
{
	KprContent self = it;
	UInt16 vertical = self->coordinates.vertical;
	if (!(vertical & kprHeight)) {
		KprSkin skin = self->skin;
		if (skin) {
			FskRectangleRecord bounds;
			KprSkinMeasure(skin, &bounds);
			self->coordinates.height = bounds.height;
		}
		else
			self->coordinates.height = 0;
	}
}

void KprContentPlace(void* it UNUSED) 
{
}

void KprContentPlaceHorizontally(KprContent self, SInt32 containerWidth) 
{
	KprCoordinates coordinates = &self->coordinates;
	UInt16 horizontal = coordinates->horizontal;
	if ((horizontal & kprLeftRight) == kprLeftRight) {
		if ((coordinates->left == 0) && (coordinates->right == 0))
			self->bounds.x = ((containerWidth - self->bounds.width + 1) >> 1);
		else
			self->bounds.x = ((containerWidth - self->bounds.width) * coordinates->left) / (coordinates->left + coordinates->right);
	}
	else if (horizontal & kprLeft)
		self->bounds.x = coordinates->left;
	else if (horizontal & kprRight)
		self->bounds.x = containerWidth - self->bounds.width - coordinates->right;
	else
		self->bounds.x = ((containerWidth - self->bounds.width + 1) >> 1);
	self->flags &= ~kprXChanged;
	KprContentPlacing(self);
}

void KprContentPlaceVertically(KprContent self, SInt32 containerHeight) 
{
	KprCoordinates coordinates = &self->coordinates;
	UInt16 vertical = coordinates->vertical;
	SInt32 height = self->bounds.height;
	if ((vertical & kprTopBottom) == kprTopBottom) {
		if ((coordinates->top == 0) && (coordinates->bottom == 0))
			self->bounds.y = ((containerHeight - height + 1) >> 1);
		else
			self->bounds.y = ((containerHeight - height) * coordinates->top) / (coordinates->top + coordinates->bottom);
	}
	else if (vertical & kprTop)
		self->bounds.y = coordinates->top;
	else if (vertical & kprBottom)
		self->bounds.y = containerHeight - height - coordinates->bottom;
	else
		self->bounds.y = ((containerHeight - height + 1) >> 1);
	self->flags &= ~kprYChanged;
	KprContentPlacing(self);
}

void KprContentPlaced(void* it) 
{
	KprContent self = it;
	if (self->flags & kprPlaced) {
		self->flags &= ~kprPlaced;
		KprContentInvalidate(self);
	}
	if (self->flags & kprDisplaying) {
		self->flags &= ~kprDisplaying;
		kprDelegateDisplaying(self);
	}
}

void KprContentPredict(void* it UNUSED, FskRectangle area UNUSED) 
{
}

void KprContentReflowing(void* it UNUSED, UInt32 flags UNUSED)
{
	ASSERT(0);
}

void KprContentRemoving(void* it UNUSED, KprContent content UNUSED) 
{
	ASSERT(0);
}

void KprContentSetWindow(void* it, KprShell shell, KprStyle style) 
{
	KprContent self = it;
	if (self->shell && !shell) {
		if (self->shell->focus == self)
			self->shell->focus = (KprContent)self->shell;
		if (self->flags & kprIdling)
			KprShellStopIdling(self->shell, self);
		if (KprContentChainContains(&self->shell->touchChain, it))
			KprContentChainRemove(&self->shell->touchChain, it);
		KprContentInvalidate(self);
		kprDelegateUndisplayed(self);
	}
	if (!self->shell && shell) {
		if (self->flags & kprIdling)
			KprShellStartIdling(shell, self);
		self->flags |= kprDisplaying | kprDisplayed;
	}
	self->shell = shell;
	if (style) {
		if (self->flags & kprHasOwnStyle) {
			KprAssetUnbind(self->style);
			self->style = KprStyleCascade(self->style, style);
		}
		else
			self->style = style;
		KprAssetBind(self->style);
	}
	else {
		KprAssetUnbind(self->style);
		if (self->flags & kprHasOwnStyle) {
			self->style = KprStyleUncascade(self->style);
			KprAssetBind(self->style);
		}
		else
			self->style = NULL;
	}
}

void KprContentShowing(void* it, Boolean showIt UNUSED) 
{
	KprContent self = it;
	KprContentInvalidate(self);
}

void KprContentShown(void* it, Boolean showIt UNUSED) 
{
	KprContent self = it;
	KprContentInvalidate(self);
}

void KprContentUpdate(void* it, FskPort port, FskRectangle area) 
{
	KprContent self = it;
	if ((self->flags & kprVisible) && FskRectangleIsIntersectionNotEmpty(KprBounds(self), area)) {
        FskPortOffsetOrigin(port, self->bounds.x, self->bounds.y);
		FskRectangleOffset(area, -self->bounds.x, -self->bounds.y);
		(*self->dispatch->draw)(self, port, area);
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

/* CONTENT ECMASCRIPT */

void KPR_content(void *it)
{
	if (it) {
		KprContent self = it;
		kprVolatileDestructor(KPR_content);
		if (((KprShell)self != self->shell) && !self->container) {
			KprContentClose(self);
		}
	}
}

void KPR_content_marker(void* it, xsMarkRoot markRoot)
{
	if (it) {
		KprContent self = it;
		(*self->dispatch->mark)(it, markRoot);
	}
}

static xsHostHooks KPR_content_hostHooks = {
	KPR_content,
	KPR_content_marker
};

void KPR_content_patch(xsMachine *the)
{
	xsResult = xsGet(xsGet(xsGlobal, xsID_KPR), xsID_content);
	xsSetHostHooks(xsResult, &KPR_content_hostHooks);
}

void KPR_Content(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprCoordinatesRecord coordinates;
	KprSkin skin = NULL;
	KprStyle style = NULL;
	KprContent self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	if ((c > 1) && xsTest(xsArg(1)))
		skin = kprGetHostData(xsArg(1), skin, skin);
	if ((c > 2) && xsTest(xsArg(2)))
		style = kprGetHostData(xsArg(2), skin, style);
	KprContentNew(&self, &coordinates, skin, style);
	kprContentConstructor(KPR_Content);
}

void KPR_content_get_active(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprActive) ? xsTrue : xsFalse;
}

void KPR_content_get_backgroundTouch(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprBackgroundTouch) ? xsTrue : xsFalse;
}

void KPR_content_get_behavior(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	KprBehavior behavior = self->behavior;
	if (behavior)
		xsResult = behavior->slot;
}

void KPR_content_get_bounds(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	SInt32 x, y;
	if (self->shell) {
		KprShellAdjust(self->shell);
		KprContentToWindowCoordinates(self, 0, 0, &x, &y);
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsResult, xsID_x, xsInteger(x), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_y, xsInteger(y), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_width, xsInteger(self->bounds.width), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_height, xsInteger(self->bounds.height), xsDefault, xsDontScript);
	}
}

void KPR_content_get_container(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = kprContentGetter(self->container);
}

void KPR_content_get_coordinates(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsSlotFromKprCoordinates(the, &xsResult, &self->coordinates);
}

void KPR_content_get_duration(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->duration);
}

void KPR_content_get_exclusiveTouch(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprExclusiveTouch) ? xsTrue : xsFalse;
}

void KPR_content_get_focused(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = xsBoolean(KprContentIsFocus(self));
}

void KPR_content_get_fraction(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	if (self->duration)
		xsResult = xsNumber(self->time / self->duration);
}

void KPR_content_get_index(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = xsInteger(KprContentIndex(self));
}

void KPR_content_get_interval(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->interval);
}

void KPR_content_get_multipleTouch(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprMultipleTouch) ? xsTrue : xsFalse;
}

void KPR_content_get_name(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	if (self->name)
		xsResult = xsString(self->name);
}

void KPR_content_get_next(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = kprContentGetter(self->next);
}

void KPR_content_get_offset(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	if (self->shell) {
		KprShellAdjust(self->shell);
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsResult, xsID_x, xsInteger(self->bounds.x), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_y, xsInteger(self->bounds.y), xsDefault, xsDontScript);
	}
}

void KPR_content_get_position(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	SInt32 x, y;
	if (self->shell) {
		KprShellAdjust(self->shell);
		KprContentToWindowCoordinates(self, 0, 0, &x, &y);
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsResult, xsID_x, xsInteger(x), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_y, xsInteger(y), xsDefault, xsDontScript);
	}
}

void KPR_content_get_previous(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = kprContentGetter(self->previous);
}

void KPR_content_get_running(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprIdling) ? xsTrue : xsFalse;
}

void KPR_content_get_size(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	if (self->shell) {
		KprShellAdjust(self->shell);
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsResult, xsID_width, xsInteger(self->bounds.width), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID_height, xsInteger(self->bounds.height), xsDefault, xsDontScript);
	}
}

void KPR_content_get_skin(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = kprVolatileGetter(self->skin, xsID_skin);
}

void KPR_content_get_state(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->state);
}

void KPR_content_get_style(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	KprStyle style = KprContentGetStyle(self);
	xsResult = kprVolatileGetter(style, xsID_style);
}

void KPR_content_get_time(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->time);
}

void KPR_content_get_variant(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->variant);
}

void KPR_content_get_visible(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprVisible) ? xsTrue : xsFalse;
}

void KPR_content_get_x(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	SInt32 x, y;
	if (self->shell) {
		KprShellAdjust(self->shell);
		KprContentToWindowCoordinates(self, 0, 0, &x, &y);
		xsResult = xsInteger(x);
	}
}

void KPR_content_get_y(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	SInt32 x, y;
	if (self->shell) {
		KprShellAdjust(self->shell);
		KprContentToWindowCoordinates(self, 0, 0, &x, &y);
		xsResult = xsInteger(y);
	}
}

void KPR_content_get_width(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	if (self->coordinates.horizontal & kprWidth)
		xsResult = xsInteger(self->coordinates.width);
	else if (self->shell) {
		KprShellAdjust(self->shell);
		xsResult = xsInteger(self->bounds.width);
	}
}

void KPR_content_get_height(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	if (self->coordinates.vertical & kprHeight)
		xsResult = xsInteger(self->coordinates.height);
	else if (self->shell) {
		KprShellAdjust(self->shell);
		xsResult = xsInteger(self->bounds.height);
	}
}

void KPR_content_set_active(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprActive;
	else
		self->flags &= ~kprActive;
	kprDelegateStateChanged(self);
}

void KPR_content_set_backgroundTouch(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprBackgroundTouch;
	else
		self->flags &= ~kprBackgroundTouch;
}

void KPR_content_set_behavior(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	KprBehavior behavior = NULL;
	if (xsTest(xsArg(0))) {
		if (xsIsInstanceOf(xsArg(0), xsObjectPrototype))
			xsThrowIfFskErr(KprScriptBehaviorNew(&behavior, self, the, &xsArg(0)));
		else {
			char* name = xsToString(xsArg(0));
			if (!FskStrCompare(name, "dragBar"))
				xsThrowIfFskErr(KprDragBarBehaviorNew(&behavior, self));
			else if (!FskStrCompare(name, "growBox"))
				xsThrowIfFskErr(KprGrowBoxBehaviorNew(&behavior, self));
		}
	}
	KprContentSetBehavior(self, behavior);
}

void KPR_content_set_coordinates(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	KprCoordinatesRecord coordinates;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	KprContentSetCoordinates(self, &coordinates);
}

void KPR_content_set_duration(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	KprContentSetDuration(self, xsToNumber(xsArg(0)));
}

void KPR_content_set_exclusiveTouch(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprExclusiveTouch;
	else
		self->flags &= ~kprExclusiveTouch;
}

void KPR_content_set_fraction(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	KprContentSetFraction(self, xsToNumber(xsArg(0)));
}

void KPR_content_set_interval(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	KprContentSetInterval(self, xsToNumber(xsArg(0)));
}

void KPR_content_set_multipleTouch(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0)))
		self->flags |= kprMultipleTouch;
	else
		self->flags &= ~kprMultipleTouch;
}

void KPR_content_set_name(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	FskMemPtrDispose(self->name);
	self->name = NULL;
	if (xsTest(xsArg(0))) {
		self->name = FskStrDoCopy(xsToString(xsArg(0)));	
		xsThrowIfNULL(self->name);
	}
}

void KPR_content_set_offset(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	SInt32 x = 0, y = 0;
	if (self->shell) {
		KprShellAdjust(self->shell);
		xsEnterSandbox();
		if (xsFindInteger(xsArg(0), xsID_x, &x))
			x -= self->bounds.x;
		if (xsFindInteger(xsArg(0), xsID_y, &y))
			y -= self->bounds.y;
		xsLeaveSandbox();
		KprContentMoveBy(self, x, y);
	}
}

void KPR_content_set_position(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	SInt32 x0, y0, x1 = 0, y1 = 0;
	if (self->shell) {
		KprShellAdjust(self->shell);
		KprContentToWindowCoordinates(self, 0, 0, &x0, &y0);
		xsEnterSandbox();
		if (xsFindInteger(xsArg(0), xsID_x, &x1))
			x1 -= x0;
		if (xsFindInteger(xsArg(0), xsID_y, &y1))
			y1 -= y0;
		xsLeaveSandbox();
		KprContentMoveBy(self, x1, y1);
	}
}

void KPR_content_set_size(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	SInt32 width = 0, height = 0;
	if (self->shell) {
		KprShellAdjust(self->shell);
		xsEnterSandbox();
		if (xsFindInteger(xsArg(0), xsID_width, &width))
			width -= self->bounds.width;
		if (xsFindInteger(xsArg(0), xsID_height, &height))
			height -= self->bounds.height;
		xsLeaveSandbox();
		KprContentSizeBy(self, width, height);
	}
}

void KPR_content_set_skin(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	KprSkin skin = NULL;
	if (xsTest(xsArg(0)))
		skin = kprGetHostData(xsArg(0), skin, skin);
	KprContentSetSkin(self, skin);
}

void KPR_content_set_state(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	double state = xsToNumber(xsArg(0));
	if (self->state != state) {
		self->state = state;
		KprContentInvalidate(self);
	}
}

void KPR_content_set_style(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	KprStyle style = NULL;
	if (xsTest(xsArg(0)))
		style = kprGetHostData(xsArg(0), style, style);
	KprContentSetStyle(self, style);
}

void KPR_content_set_time(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	KprContentSetTime(self, xsToNumber(xsArg(0)));
}

void KPR_content_set_variant(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	SInt32 variant = xsToInteger(xsArg(0)); 
	if (self->variant != variant) {
		self->variant = variant;
		KprContentInvalidate(self);
	}
}

void KPR_content_set_visible(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	KprContentShow(self, xsTest(xsArg(0)));
}

void KPR_content_set_x(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	SInt32 x, y;
	if (self->shell) {
		KprShellAdjust(self->shell);
		KprContentToWindowCoordinates(self, 0, 0, &x, &y);
		KprContentMoveBy(self, xsToInteger(xsArg(0)) - x, 0);
	}
}

void KPR_content_set_y(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	SInt32 x, y;
	if (self->shell) {
		KprShellAdjust(self->shell);
		KprContentToWindowCoordinates(self, 0, 0, &x, &y);
		KprContentMoveBy(self, 0, xsToInteger(xsArg(0)) - y);
	}
}

void KPR_content_set_width(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	if (self->shell) {
		KprShellAdjust(self->shell);
		KprContentSizeBy(self, xsToInteger(xsArg(0)) - self->bounds.width, 0);
	}
}

void KPR_content_set_height(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	if (self->shell) {
		KprShellAdjust(self->shell);
		KprContentSizeBy(self, 0, xsToInteger(xsArg(0)) - self->bounds.height);
	}
}

void KPR_content_adjust(xsMachine *the)
{
	KprContent self = kprGetHostData(xsThis, this, content);
	if (self->container)
		KprContentReflow(self, kprSizeChanged);
}

void KPR_content_bubble(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	KprContent context = xsGetContext(the);
	KprContent content = kprGetHostData(xsThis, this, content);
	xsIndex id = xsID(xsToString(xsArg(0)));
	xsVars(3);
	xsVar(1) = xsNew1(xsGlobal, xsID_Array, xsNumber(c));
	for (i = 1; i < c; i++)
		xsSetAt(xsVar(1), xsInteger(i), xsArg(i));
	while (content) {
		KprScriptBehavior self = KprContentGetScriptBehavior(content);
		if (self) {
			xsVar(0) = xsAccess(self->slot);
			if (xsFindResult(xsVar(0), id)) {
				xsVar(2) = kprContentGetter(content);
				xsSetAt(xsVar(1), xsInteger(0), xsVar(2));
				if (xsTest(xsCall2(xsResult, xsID_apply, xsVar(0), xsVar(1)))) {
					xsResult = xsVar(2);
					break;
				}
			}
		}
		if (content == context)
			break;
		content = (KprContent)content->container;
	}
}

void KPR_content_cancel(xsMachine *the)
{
	KprContent self = kprGetHostData(xsThis, this, content);
	KprContentCancel(self);
}

void KPR_content_captureTouch(xsMachine *the)
{
	KprContent self = kprGetHostData(xsThis, this, content);
	if (self->shell){
		xsIntegerValue id = xsToInteger(xsArg(0));
		xsIntegerValue x = xsToInteger(xsArg(1));
		xsIntegerValue y = xsToInteger(xsArg(2));
		xsNumberValue ticks = xsToNumber(xsArg(2));
		KprShellCaptureTouch(self->shell, self, id, x, y, ticks);
	}
}

void KPR_content_delegate(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	KprContent content = kprGetHostData(xsThis, this, content);
	KprScriptBehavior self = KprContentGetScriptBehavior(content);
	xsVars(2);
	if (self) {
		xsIndex id = xsID(xsToString(xsArg(0)));
		xsVar(0) = xsAccess(self->slot);
		if (xsFindResult(xsVar(0), id)) {
			xsVar(1) = xsNew1(xsGlobal, xsID_Array, xsNumber(c));
			xsSetAt(xsVar(1), xsInteger(0), xsThis);
			for (i = 1; i < c; i++)
				xsSetAt(xsVar(1), xsInteger(i), xsArg(i));
			xsResult = xsCall2(xsResult, xsID_apply, xsVar(0), xsVar(1));
		}
	}
}

static Boolean KPR_content_distributeAux(xsMachine *the, KprContent container, xsIndex id)
{
	KprScriptBehavior self;
	if ((container->flags & (kprContainer | kprHost)) == kprContainer) {
		KprContent content = ((KprContainer)container)->first;
		while (content) {
			if (KPR_content_distributeAux(the, content, id))
				return true;
			content = content->next;
		}
	}
	self = KprContentGetScriptBehavior(container);
	if (self) {
		xsVar(0) = xsAccess(self->slot);
		if (xsFindResult(xsVar(0), id)) {
			xsVar(2) = kprContentGetter(container);
			xsSetAt(xsVar(1), xsInteger(0), xsVar(2));
			xsResult = xsCall2(xsResult, xsID_apply, xsVar(0), xsVar(1));
			if (xsTest(xsResult))
				return true;
		}
	}
	return false;
}

void KPR_content_distribute(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	KprContent content = kprGetHostData(xsThis, this, content);
	xsIndex id = xsID(xsToString(xsArg(0)));
	xsVars(3);
	xsVar(1) = xsNew1(xsGlobal, xsID_Array, xsNumber(c));
	for (i = 1; i < c; i++)
		xsSetAt(xsVar(1), xsInteger(i), xsArg(i));
	(void)KPR_content_distributeAux(the, content, id);
}


void KPR_content_focus(xsMachine *the)
{
	KprContent self = kprGetHostData(xsThis, this, content);
	if (self->shell) {
		KprContent context = xsGetContext(the);
		KprContent content = self->shell->focus;
		while (content) {
			if (content == context)
				break;
			content = (KprContent)content->container;
		}
		if (content)
			xsResult = kprContentGetter(self->shell->focus);
		KprShellSetFocus(self->shell, self);
	}
}

void KPR_content_hit(xsMachine *the)
{
	KprContent self = kprGetHostData(xsThis, this, content);
	KprContent result = NULL;
	if (self->shell) {
		SInt32 x = xsToInteger(xsArg(0));
		SInt32 y = xsToInteger(xsArg(1));
		KprContentFromWindowCoordinates(self, x, y, &x, &y);
		result = (*self->dispatch->hit)(self, x, y);
	}
	xsResult = kprContentGetter(result);
}

void KPR_content_invoke(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprContent self = kprGetHostData(xsThis, this, content);
	KprMessage message = kprGetHostData(xsArg(0), message, message);
	if ((c > 1) && xsTest(xsArg(1))) {
		KprMessageScriptTargetSet(message, the, &xsArg(1));
		xsThrowIfFskErr(KprMessageInvoke(message, KprContentComplete, NULL, self));
		self->flags |= kprMessaging;
	}
	else
		xsThrowIfFskErr(KprMessageInvoke(message, NULL, NULL, NULL));
}

void KPR_content_measure(xsMachine *the)
{
	KprContent self = kprGetHostData(xsThis, this, content);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID_width, xsInteger(self->coordinates.width), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_height, xsInteger(self->coordinates.height), xsDefault, xsDontScript);
}

void KPR_content_moveBy(xsMachine *the)
{
	KprContent self = kprGetHostData(xsThis, this, content);
	xsIntegerValue x = xsToInteger(xsArg(0));
	xsIntegerValue y = xsToInteger(xsArg(1));
	KprContentMoveBy(self, x, y);
}

void KPR_content_peek(xsMachine *the)
{
}

void KPR_content_sizeBy(xsMachine *the)
{
	KprContent self = kprGetHostData(xsThis, this, content);
	xsIntegerValue x = xsToInteger(xsArg(0));
	xsIntegerValue y = xsToInteger(xsArg(1));
	KprContentSizeBy(self, x, y);
}

void KPR_content_start(xsMachine *the)
{
	KprContent self = kprGetHostData(xsThis, this, content);
	KprContentStart(self);
}

void KPR_content_stop(xsMachine *the)
{
	KprContent self = kprGetHostData(xsThis, this, content);
	KprContentStop(self);
}

static void KPR_content_waitCancel(KprMessage message, void* it)
{
	FskTimeCallBack callback = it;
	FskTimeCallbackDispose(callback);
}

static void KPR_content_waitComplete(FskTimeCallBack callback UNUSED, const FskTime time UNUSED, void *it)
{
	KprMessage message = it;
	KprMessageResume(message);
	FskTimeCallbackDispose(callback);
}

void KPR_content_wait(xsMachine *the)
{
	KprContent self = kprGetHostData(xsThis, this, content);
	KprMessage message;
	FskTimeCallBack callback;
	FskTimeRecord when;
	xsThrowIfFskErr(KprMessageNew(&message, "xkpr://wait"));
	xsResult = xsString("TEXT");
	KprMessageScriptTargetSet(message, the, &xsResult);
	xsThrowIfFskErr(KprMessageInvoke(message, KprContentComplete, NULL, self));
	self->flags |= kprMessaging;
	FskTimeCallbackNew(&callback);
	KprMessageSuspend(message, KPR_content_waitCancel, NULL, callback);
	FskTimeGetNow(&when);
	FskTimeAddMS(&when, xsToInteger(xsArg(0)));
	FskTimeCallbackSet(callback, &when, KPR_content_waitComplete, message);
}

void KPR_Content_combineCoordinates(xsMachine *the)
{
#define kprCombineCoordinate(_ID, _DIRECTION, _FLAG, _COORDINATE) \
	if (xsFindResult(xsArg(1), _ID)) { \
		if (xsTypeOf(xsResult) == xsUndefinedType) { \
			coordinates._DIRECTION &= ~_FLAG; \
			coordinates._COORDINATE = 0; \
		} \
		else { \
			coordinates._DIRECTION |= _FLAG; \
			coordinates._COORDINATE = xsToInteger(xsResult); \
		} \
	}
	KprCoordinatesRecord coordinates;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	xsEnterSandbox();
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		kprCombineCoordinate(xsID_left, horizontal, kprLeft, left);
		kprCombineCoordinate(xsID_width, horizontal, kprWidth, width);
		kprCombineCoordinate(xsID_right, horizontal, kprRight, right);
		kprCombineCoordinate(xsID_top, vertical, kprTop, top);
		kprCombineCoordinate(xsID_height, vertical, kprHeight, height);
		kprCombineCoordinate(xsID_bottom, vertical, kprBottom, bottom);
		xsLeaveSandbox();
	}
	xsLeaveSandbox();
	xsSlotFromKprCoordinates(the, &xsResult, &coordinates);
}

void KPR_Content_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("Content"));
	xsNewHostProperty(xsResult, xsID("combineCoordinates"), xsNewHostFunction(KPR_Content_combineCoordinates, 2), xsDefault, xsDontScript);
}

/* CONTAINER */

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprContainerInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprContainer", FskInstrumentationOffset(KprContainerRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprContainerDispatchRecord = {
	"container",
	KprContainerActivated,
	KprContainerAdded,
	KprContainerCascade,
	KprContainerDispose,
	KprContentDraw,
	KprContainerFitHorizontally,
	KprContainerFitVertically,
	KprContentGetBitmap,
	KprContainerHit,
	KprContentIdle,
	KprContainerInvalidated,
	KprContainerLayered,
	KprContainerMark,
	KprContainerMeasureHorizontally,
	KprContainerMeasureVertically,
	KprContainerPlace,
	KprContainerPlaced,
	KprContainerPredict,
	KprContainerReflowing,
	KprContainerRemoving,
	KprContainerSetWindow,
	KprContainerShowing,
	KprContainerShown,
	KprContainerUpdate
};

FskErr KprContainerNew(KprContainer *it, KprCoordinates coordinates, KprSkin skin, KprStyle style)
{
	FskErr err = kFskErrNone;
	KprContainer self;
	bailIfError(FskMemPtrNewClear(sizeof(KprContainerRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprContainerInstrumentation);
	self->dispatch = &KprContainerDispatchRecord;
	self->flags = kprContainer | kprVisible;
	KprContentInitialize((KprContent)self, coordinates, skin, style);
bail:
	return err;
}

void KprContainerAdd(KprContainer self, KprContent content)
{
	ASSERT(self != NULL);
	ASSERT(content != NULL);
	ASSERT(content->container == NULL);
	ASSERT(content->previous == NULL);
	ASSERT(content->next == NULL);
	if (self->first) {
		content->previous = self->last;
		self->last->next = content;
	}
	else
		self->first = content;
	self->last = content;
	content->container = self;
	FskInstrumentedItemSetOwner(content, self);
	(*self->dispatch->added)(self, content);
}

KprContent KprContainerContent(KprContainer self, SInt32 index)
{
	SInt32 count = 0;
	KprContent content = self->first;
	while (content) {
		if (count == index)
			return content;
		count++;
		content = content->next;
	}
	return NULL;
}

SInt32 KprContainerCount(KprContainer self)
{
	KprContent content = self->first;
	SInt32 count = 0;
	while (content) {
		count++;
		content = content->next;
	}
	return count;
}

void KprContainerEmpty(KprContainer self, SInt32 start, SInt32 stop)
{
	KprContent content, previous, next;
	SInt32 length, index;
	ASSERT(self != NULL);
	length = KprContainerCount(self);
	if (start < 0) {
		start += length;
		if (start < 0)
			start = 0;
	}
	else if (start > length)
		start = length;
	if (stop <= 0) {
		stop += length;
		if (stop < 0)
			stop = 0;
	}
	else if (stop > length)
		stop = length;
	if (start < stop) {
		content = self->first;
		index = 0;
		while (index < start) {
			content = content->next;
			index++;
		}
		while (index < stop) {
			(*self->dispatch->removing)(self, content);
			content = content->next;
			index++;
		}
		previous = NULL;
		next = NULL;
		content = self->first;
		index = 0;
		while (index < start) {
			previous = content;
			content = content->next;
			index++;
		}
		while (index < stop) {
			next = content->next;
			KprContentClose(content);
			content = next;
			index++;
		}
		if (previous)
			previous->next = next;
		else
			self->first = next;
		if (next)
			next->previous = previous;
		else
			self->last = previous;
	}
}

void KprContainerInsert(KprContainer self, KprContent content, KprContent before)
{
	KprContent previous;
	ASSERT(self != NULL);
	ASSERT(content != NULL);
	ASSERT(content->container == NULL);
	ASSERT(content->previous == NULL);
	ASSERT(content->next == NULL);
	ASSERT(before != NULL);
	ASSERT(before->container == self);
	previous = before->previous;
	before->previous = content;
	content->next = before;
	content->previous = previous;
	if (previous)
		previous->next = content;
	else
		self->first = content;
	content->container = self;
	FskInstrumentedItemSetOwner(content, self);
	(*self->dispatch->added)(self, content);
}

void KprContainerRemove(KprContainer self, KprContent content)
{
	KprContent next, previous;
	ASSERT(self != NULL);
	ASSERT(content != NULL);
	ASSERT(content->container == self);
	(*self->dispatch->removing)(self, content);
	previous = content->previous;
	next = content->next;
	if (previous)
		previous->next = next;
	else
		self->first = next;
	if (next)
		next->previous = previous;
	else
		self->last = previous;
	KprContentClose(content);
}

void KprContainerReplace(KprContainer self, KprContent content, KprContent by)
{
	KprContent next, previous;
	ASSERT(self != NULL);
	ASSERT(content != NULL);
	ASSERT(content->container == self);
	ASSERT(by != NULL);
	ASSERT(by->container == NULL);
	ASSERT(by->previous == NULL);
	ASSERT(by->next == NULL);
	(*self->dispatch->removing)(self, content);
	previous = content->previous;
	next = content->next;
	if (previous)
		previous->next = by;
	else
		self->first = by;
	if (next)
		next->previous = by;
	else
		self->last = by;
	KprContentClose(content);
	by->container = self;
	FskInstrumentedItemSetOwner(by, self);
	by->previous = previous;
	by->next = next;
	(*self->dispatch->added)(self, by);
}

void KprContainerSwap(KprContainer self, KprContent content0, KprContent content1)
{
	KprContent next0, next1, previous0, previous1;
	ASSERT(self != NULL);
	ASSERT(content0 != NULL);
	ASSERT(content0->container == self);
	ASSERT(content1 != NULL);
	ASSERT(content1->container == self);
	KprContentInvalidate(content0);
	KprContentInvalidate(content1);
	previous0 = content0->previous;
	next0 = content0->next;
	previous1 = content1->previous;
	next1 = content1->next;
	if (!previous0) {
		self->first = content1;
		content1->previous = NULL;
	}
	else if (previous0 != content1) {
		previous0->next = content1;
		content1->previous = previous0;
	}
	else
		content1->previous = content0;
	if (!next0)	{
		self->last = content1;
		content1->next = NULL;
	}
	else if (next0 != content1)	{
		next0->previous = content1;
		content1->next = next0;
	}
	else
		content1->next = content0;
	if (!previous1)	{
		self->first = content0;
		content0->previous = NULL;
	}
	else if (previous1 != content0)	{
		previous1->next = content0;
		content0->previous = previous1;
	}
	else
		content0->previous = content1;
	if (!next1)	{
		self->last = content0;
		content0->next = NULL;
	}
	else if (next1 != content0)	{
		next1->previous = content0;
		content0->next = next1;
	}
	else
		content0->next = content1;
	KprContentReflow((KprContent)self, kprContentsChanged);
}

/* CONTAINER DISPATCH */

void KprContainerActivated(void* it, Boolean activateIt) 
{
	KprContainer self = it;
	KprContent content = self->first;
	while (content) {
		(*content->dispatch->activated)(content, activateIt);
		content = content->next;
	}
}

void KprContainerAdded(void* it, KprContent content) 
{
	KprContainer self = it;
	if (self->shell) {
		(*content->dispatch->setWindow)(content, self->shell, self->style);
		KprContentReflow(content, kprSizeChanged);
	}
	else {
		content->flags |= kprSizeChanged;
		self->flags |= kprContentsPlaced;
	}
}

void KprContainerCascade(void* it, KprStyle style) 
{
	KprContainer self = it;
	KprContent content = self->first;
	KprContentCascade(it, style);
	while (content) {
		(*content->dispatch->cascade)(content, self->style);
		content = content->next;
	}
}

void KprContainerDispose(void* it) 
{
	KprContainer self = it;
	KprTransitionUnlink(self->transition, self);
	KprContentDispose(it);
}

void KprContainerFitHorizontally(void* it) 
{
	KprContainer self = it;
	KprContent content = self->first;
	SInt32 width;
	KprContentFitHorizontally(it);
	width = self->bounds.width;
	while (content) {
		FskRectangle bounds = &content->bounds;
		KprCoordinates coordinates = &content->coordinates;
		if ((coordinates->horizontal & kprLeftRight) == kprLeftRight)
			bounds->width = width - coordinates->left - coordinates->right;
		(*content->dispatch->fitHorizontally)(content);
		KprContentPlaceHorizontally(content, width);
		content = content->next;
	}
	self->flags |= kprContentsPlaced;
}

void KprContainerFitVertically(void* it) 
{
	KprContainer self = it;
	KprContent content = self->first;
	SInt32 height;
	KprContentFitVertically(it);
	height = self->bounds.height;
	while (content) {
		FskRectangle bounds = &content->bounds;
		KprCoordinates coordinates = &content->coordinates;
		if ((coordinates->vertical & kprTopBottom) == kprTopBottom)
			bounds->height = height - coordinates->top - coordinates->bottom;
		(*content->dispatch->fitVertically)(content);
		KprContentPlaceVertically(content, height);
		content = content->next;
	}
	self->flags |= kprContentsPlaced;
}

KprContent KprContainerHit(void* it, SInt32 x, SInt32 y) 
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

void KprContainerInvalidated(void* it, FskRectangle area) 
{
	if (area) {
		KprContainer self = it;
		if (self->flags & kprVisible) {
			KprContainer container = self->container;
			if (container && !FskRectangleIsEmpty(area)) {
				FskRectangleOffset(area, self->bounds.x, self->bounds.y);
				if (self->flags & kprClip)
					FskRectangleIntersect(area, KprBounds(self), area);
				(*container->dispatch->invalidated)(container, area);
			}
		}
	}
	else
		KprContentInvalidated(it, area);
}

void KprContainerLayered(void* it, KprLayer layer, Boolean layerIt)
{
	KprContainer self = it;
	KprContent content = self->first;
	KprContentLayered(it, layer, layerIt);
	while (content) {
		(*content->dispatch->layered)(content, layer, layerIt);
		content = content->next;
	}
}

void KprContainerMark(void* it, xsMarkRoot markRoot)
{
	KprContainer self = it;
	KprContent content = self->first;
	KprContentMark(it, markRoot);
	while (content) {
		(*content->dispatch->mark)(content, markRoot);
		content = content->next;
	}
}

void KprContainerMeasureHorizontally(void* it) 
{
	KprContainer self = it;
	SInt32 max = 0, width;
	KprContent content = self->first;
	UInt16 horizontal = self->coordinates.horizontal;
	KprContentMeasureHorizontally(it);
	max = self->coordinates.width;
	while (content) {
		KprCoordinates coordinates = &content->coordinates;
		(*content->dispatch->measureHorizontally)(content);
		width = coordinates->width + coordinates->left + coordinates->right;
		if (max < width)
			max = width;
		content = content->next;
	}
	if (!(horizontal & kprWidth))
		self->coordinates.width = max;
}

void KprContainerMeasureVertically(void* it) 
{
	KprContainer self = it;
	SInt32 max = 0, height;
	KprContent content = self->first;
	UInt16 vertical = self->coordinates.vertical;
	KprContentMeasureVertically(it);
	max = self->coordinates.height;
	while (content) {
		KprCoordinates coordinates = &content->coordinates;
		(*content->dispatch->measureVertically)(content);
		height = coordinates->height + coordinates->top + coordinates->bottom;
		if (max < height)
			max = height;
		content = content->next;
	}
	if (!(vertical & kprHeight))
		self->coordinates.height = max;
}

void KprContainerPlace(void* it) 
{
	KprContainer self = it;
	KprContent content = self->first;
	self->flags &= ~kprContentsPlaced;
	while (content) {
		if (content->flags & kprPlaced) {
			(*content->dispatch->placed)(content);
		}
		if (content->flags & kprContentsPlaced) {
			(*content->dispatch->place)(content);
		}
		content = content->next;
	}
}

void KprContainerPlaced(void* it) 
{
//	KprContainer self = it;
	KprContentPlaced(it);
    /*if (self->flags & kprContentsPlaced)
	{
        KprContent content = self->first;
        while (content) {
            (*content->dispatch->placed)(content);
            content = content->next;
        }
      self->flags &= ~kprContentsPlaced;
	}*/
}

void KprContainerPlaceHorizontally(void* it) 
{
	KprContainer self = it;
	KprContent content = self->first;
	self->flags &= ~kprContentsHorizontallyChanged;
	while (content) {
		if (content->flags & kprWidthChanged) {
			KprContentInvalidate(content);
			(*content->dispatch->measureHorizontally)(content);
			if ((content->coordinates.horizontal & kprLeftRight) == kprLeftRight)
				content->bounds.width = self->bounds.width - content->coordinates.left - content->coordinates.right;
			(*content->dispatch->fitHorizontally)(content);
			content->flags |= kprXChanged;
		}
		else if (content->flags & kprContentsHorizontallyChanged) 
			KprContainerPlaceHorizontally(content);
		if (content->flags & kprXChanged)
			KprContentPlaceHorizontally(content, self->bounds.width);
		content = content->next;
	}
}

void KprContainerPlaceVertically(void* it) 
{
	KprContainer self = it;
	KprContent content = self->first;
	self->flags &= ~kprContentsVerticallyChanged;
	while (content) {
		if (content->flags & kprHeightChanged) {
			KprContentInvalidate(content);
			(*content->dispatch->measureVertically)(content);
			if ((content->coordinates.vertical & kprTopBottom) == kprTopBottom)
				content->bounds.height = self->bounds.height - content->coordinates.top - content->coordinates.bottom;
			(*content->dispatch->fitVertically)(content);
			content->flags |= kprYChanged;
		}
		else if (content->flags & kprContentsVerticallyChanged) 
			KprContainerPlaceVertically(content);
		if (content->flags & kprYChanged)
			KprContentPlaceVertically(content, self->bounds.height);
		content = content->next;
	}
}

void KprContainerPredict(void* it, FskRectangle area) 
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
			while (content) {
				(*content->dispatch->predict)(content, &clip);
				content = content->next;
			}
		}
	}
}

void KprContainerReflowing(void* it, UInt32 flags)
{
	KprContainer self = it;
	KprContainer container = self->container;
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedContainerReflowing, &flags);
	if (flags & kprHorizontallyChanged) {
		if ((self->coordinates.horizontal & kprLeftRightWidth) < kprLeftRight)
			self->flags |= kprWidthChanged;
		else
			self->flags |= kprContentsHorizontallyChanged;
	}
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

void KprContainerRemoving(void* it, KprContent content) 
{
	KprContainer self = it;
	if (self->shell) {
		UInt32 flags = 0;
		if ((self->coordinates.horizontal & kprLeftRightWidth) < kprLeftRight)
			flags |= kprWidthChanged;
		if ((self->coordinates.vertical & kprTopBottomHeight) < kprTopBottom)
			flags |= kprHeightChanged;
		if (flags)
			KprContentReflow(it, flags);
		else
			KprContentInvalidate(content);
		(*content->dispatch->setWindow)(content, NULL, NULL);
	}
}

void KprContainerSetWindow(void* it, KprShell shell, KprStyle style) 
{
	KprContainer self = it;
	KprContent content = self->first;
	KprContentSetWindow(it, shell, style);
	if (!shell)
		KprTransitionUnlink(self->transition, self);
	if (style)
		style = self->style;
	while (content) {
		(*content->dispatch->setWindow)(content, shell, style);
		content = content->next;
	}
}

void KprContainerShowing(void* it, Boolean showIt) 
{
	KprContainer self = it;
	KprContent content = self->first;
	while (content) {
		if (content->flags & kprVisible)
			(*content->dispatch->showing)(content, showIt);
		content = content->next;
	}
	KprContentInvalidate((KprContent)self);
}

void KprContainerShown(void* it, Boolean showIt) 
{
	KprContainer self = it;
	KprContent content = self->first;
	while (content) {
		if (content->flags & kprVisible)
			(*content->dispatch->shown)(content, showIt);
		content = content->next;
	}
	KprContentInvalidate((KprContent)self);
}

void KprContainerUpdate(void* it, FskPort port, FskRectangle area) 
{
	KprContainer self = it;
	if (self->flags & kprVisible) {
		KprContent content = self->first;
		if (FskRectangleIsIntersectionNotEmpty(KprBounds(self), area)) {
			FskPortOffsetOrigin(port, self->bounds.x, self->bounds.y);
			FskRectangleOffset(area, -self->bounds.x, -self->bounds.y);
			if (!FskRectangleContainsRectangle(&self->hole, area))
				(*self->dispatch->draw)(self, port, area);
			if (self->flags & kprClip) {
				FskRectangleRecord bounds, oldClip, newClip;
				FskRectangleSet(&bounds, 0, 0, self->bounds.width, self->bounds.height);
				FskPortGetClipRectangle(port, &oldClip);
				FskRectangleIntersect(&oldClip, &bounds, &newClip);
				FskPortSetClipRectangle(port, &newClip);
				FskRectangleIntersect(area, &newClip, &bounds);
				while (content) {
					(*content->dispatch->update)(content, port, &bounds);
					content = content->next;
				}
				FskPortSetClipRectangle(port, &oldClip);
			}
			else {
				while (content) {
					(*content->dispatch->update)(content, port, area);
					content = content->next;
				}
			}
			FskRectangleOffset(area, self->bounds.x, self->bounds.y);
			FskPortOffsetOrigin(port, -self->bounds.x, -self->bounds.y);
		}
		else if (!(self->flags & kprClip)) {
            FskPortOffsetOrigin(port, self->bounds.x, self->bounds.y);
			FskRectangleOffset(area, -self->bounds.x, -self->bounds.y);
			while (content) {
				(*content->dispatch->update)(content, port, area);
				content = content->next;
			}
			FskRectangleOffset(area, self->bounds.x, self->bounds.y);
            FskPortOffsetOrigin(port, -self->bounds.x, -self->bounds.y);
		}
	}
	if (self->flags & kprDisplayed) {
		if (self->shell && (self->shell->port == port)) {
			self->flags &= ~kprDisplayed;
			kprDelegateDisplayed(self);
		}
	}
}

/* CONTAINER ECMASCRIPT */

#ifdef XS6
static void KPR_container_iterator(xsMachine *the)
{
	xsVars(3);
	xsVar(0) = xsGet(xsFunction, xsID_iterator);
	xsVar(1) = xsGet(xsThis, xsID_first);
	xsVar(2) = xsNewInstanceOf(xsObjectPrototype);
	xsSet(xsVar(2), xsID_value, xsUndefined);
	xsSet(xsVar(2), xsID_done, xsTrue);
	xsResult = xsNewInstanceOf(xsVar(0));
	xsSet(xsResult, xsID_content, xsVar(1));
	xsSet(xsResult, xsID_result, xsVar(2));
}

static void KPR_container_iterator_next(xsMachine *the)
{
	xsVars(1);
	xsVar(0) = xsGet(xsThis, xsID_content);
	xsResult = xsGet(xsThis, xsID_result);
	if (xsTest(xsVar(0))) {
		xsSet(xsResult, xsID_value, xsVar(0));
		xsSet(xsResult, xsID_done, xsFalse);
		xsVar(0) = xsGet(xsVar(0) , xsID_next);
		xsSet(xsThis, xsID_content, xsVar(0));
	}
	else {
		xsSet(xsResult, xsID_value, xsUndefined);
		xsSet(xsResult, xsID_done, xsTrue);
	}
}
#endif

void KPR_Container_patch(xsMachine *the)
{
#ifdef XS6
	xsVars(5);
	xsVar(0) = xsGet(xsGet(xsGlobal, xsID_KPR), xsID_container);
	xsVar(1) = xsGet(xsGet(xsGlobal, xsID_Symbol), xsID_iterator);
	xsVar(2) = xsNewHostFunction(KPR_container_iterator, 0);
	xsVar(3) = xsNewInstanceOf(xsObjectPrototype);
	xsVar(4) = xsNewHostFunction(KPR_container_iterator_next, 0);
	xsSet(xsVar(3), xsID_next, xsVar(4));
	xsSet(xsVar(2), xsID_iterator, xsVar(3));
	xsSetAt(xsVar(0), xsVar(1), xsVar(2));
#endif
}

void KPR_Container(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprCoordinatesRecord coordinates;
	KprSkin skin = NULL;
	KprStyle style = NULL;
	KprContainer self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	if ((c > 1) && xsTest(xsArg(1)))
		skin = kprGetHostData(xsArg(1), skin, skin);
	if ((c > 2) && xsTest(xsArg(2)))
		style = kprGetHostData(xsArg(2), skin, style);
	KprContainerNew(&self, &coordinates, skin, style);
	kprContentConstructor(KPR_Container);
}

void KPR_container_get_clip(xsMachine *the)
{
	KprContainer self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprClip) ? xsTrue : xsFalse;
}

void KPR_container_get_first(xsMachine *the)
{
	KprContainer self = xsGetHostData(xsThis);
	xsResult = kprContentGetter(self->first);
}

void KPR_container_get_last(xsMachine *the)
{
	KprContainer self = xsGetHostData(xsThis);
	xsResult = kprContentGetter(self->last);
}

void KPR_container_get_length(xsMachine *the)
{
	KprContainer self = xsGetHostData(xsThis);
	xsResult = xsInteger(KprContainerCount(self));
}

void KPR_container_get_transitioning(xsMachine *the)
{
	KprContainer self = xsGetHostData(xsThis);
	xsResult = self->transition ? xsTrue : xsFalse;
}

void KPR_container_set_clip(xsMachine *the)
{
	KprContent self = xsGetHostData(xsThis);
	KprContentInvalidate((KprContent)self);
	if (xsTest(xsArg(0)))
		self->flags |= kprClip;
	else
		self->flags &= ~kprClip;
	KprContentInvalidate((KprContent)self);
}

void KPR_container_add(xsMachine *the)
{
	KprContainer self = kprGetHostData(xsThis, this, container);
	KprContent content = kprGetHostData(xsArg(0), content, content);
	xsAssert(content->container == NULL);
	xsAssert(content->previous == NULL);
	xsAssert(content->next == NULL);
	KprContainerAdd(self, content);
}

void KPR_container_content(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprContainer self = kprGetHostData(xsThis, this, container);
	KprContent content = NULL;
	if (self && (c > 0)) {
		if ((xsTypeOf(xsArg(0)) == xsIntegerType) || (xsTypeOf(xsArg(0)) == xsNumberType))
			content = KprContainerContent(self, xsToInteger(xsArg(0)));
		else {
			char* name = xsToString(xsArg(0));
			content = self->first;
			while (content) {
				if (content->name && !FskStrCompare(content->name, name))
					break;
				content = content->next;
			}
		}
	}
	if (content)
		xsResult = kprContentGetter(content);
}

void KPR_container_empty(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprContainer self = kprGetHostData(xsThis, this, container);
	xsIntegerValue start = 0;
	xsIntegerValue stop = 0;
	if ((c > 0) && xsTest(xsArg(0)))
		start = xsToInteger(xsArg(0));
	if ((c > 1) && xsTest(xsArg(1)))
		stop = xsToInteger(xsArg(1));
	KprContainerEmpty(self, start, stop);
}

void KPR_container_firstThat(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	KprContainer container = kprGetHostData(xsThis, this, container);
	KprContent content = container->first;
	xsVars(3);
	if (content) {
		xsIndex id = xsID(xsToString(xsArg(0)));
		xsVar(1) = xsNew1(xsGlobal, xsID_Array, xsNumber(c));
		for (i = 1; i < c; i++)
			xsSetAt(xsVar(1), xsInteger(i), xsArg(i));
		while (content) {
			KprScriptBehavior self = KprContentGetScriptBehavior(content);
			if (self) {
				xsVar(0) = xsAccess(self->slot);
				if (xsFindResult(xsVar(0), id)) {
					xsVar(2) = kprContentGetter(content);
					xsSetAt(xsVar(1), xsInteger(0), xsVar(2));
					xsResult = xsCall2(xsResult, xsID_apply, xsVar(0), xsVar(1));
					if (xsTest(xsResult))
						break;
				}
			}
			content = content->next;
		}
	}
}

void KPR_container_insert(xsMachine *the)
{
	KprContainer self = kprGetHostData(xsThis, this, container);
	KprContent content = kprGetHostData(xsArg(0), content, content);
	KprContent before = kprGetHostData(xsArg(1), before, content);
	xsAssert(content->container == NULL);
	xsAssert(content->previous == NULL);
	xsAssert(content->next == NULL);
	xsAssert(before->container == self);
	KprContainerInsert(self, content, before);
}

void KPR_container_lastThat(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	KprContainer container = kprGetHostData(xsThis, this, container);
	KprContent content = container->last;
	xsVars(3);
	if (content) {
		xsIndex id = xsID(xsToString(xsArg(0)));
		xsVar(1) = xsNew1(xsGlobal, xsID_Array, xsNumber(c));
		for (i = 1; i < c; i++)
			xsSetAt(xsVar(1), xsInteger(i), xsArg(i));
		while (content) {
			KprScriptBehavior self = KprContentGetScriptBehavior(content);
			if (self) {
				xsVar(0) = xsAccess(self->slot);
				if (xsFindResult(xsVar(0), id)) {
					xsVar(2) = kprContentGetter(content);
					xsSetAt(xsVar(1), xsInteger(0), xsVar(2));
					xsResult = xsCall2(xsResult, xsID_apply, xsVar(0), xsVar(1));
					if (xsTest(xsResult))
						break;
				}
			}
			content = content->previous;
		}
	}
}

void KPR_container_remove(xsMachine *the)
{
	KprContainer self = kprGetHostData(xsThis, this, container);
	KprContent content = kprGetHostData(xsArg(0), content, content);
	xsAssert(content->container == self);
	KprContainerRemove(self, content);
}

void KPR_container_replace(xsMachine *the)
{
	KprContainer self = kprGetHostData(xsThis, this, container);
	KprContent content = kprGetHostData(xsArg(0), content, content);
	KprContent by = kprGetHostData(xsArg(1), by, content);
	xsAssert(content->container == self);
	xsAssert(by->container == NULL);
	xsAssert(by->previous == NULL);
	xsAssert(by->next == NULL);
	KprContainerReplace(self, content, by);
}

void KPR_container_run(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	KprContainer self = kprGetHostData(xsThis, this, container);
	KprTransition transition = kprGetHostData(xsArg(0), transition, transition);
	if (self->shell) {
		xsAssert(transition->container == NULL);
		xsResult = xsNew1(xsGlobal, xsID_Array, xsNumber(c));
		xsSetAt(xsResult, xsInteger(0), xsThis);
		for (i = 1; i < c; i++)
			xsSetAt(xsResult, xsInteger(i), xsArg(i));
		xsSet(xsArg(0), xsID_parameters, xsResult);
		KprTransitionLink(transition, self);
	}
}

void KPR_container_swap(xsMachine *the)
{
	KprContainer self = kprGetHostData(xsThis, this, container);
	KprContent content0 = kprGetHostData(xsArg(0), content0, content);
	KprContent content1 = kprGetHostData(xsArg(1), content1, content);
	xsAssert(content0->container == self);
	xsAssert(content1->container == self);
	KprContainerSwap(self, content0, content1);
}

/* LAYOUT */

static void KprLayoutMeasureHorizontally(void* it);
static void KprLayoutMeasureVertically(void* it);
static void KprLayoutPlaced(void* it);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprLayoutInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprLayout", FskInstrumentationOffset(KprLayoutRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprLayoutDispatchRecord = {
	"layout",
	KprContainerActivated,
	KprContainerAdded,
	KprContainerCascade,
	KprContainerDispose,
	KprContentDraw,
	KprContainerFitHorizontally,
	KprContainerFitVertically,
	KprContentGetBitmap,
	KprContainerHit,
	KprContentIdle,
	KprContainerInvalidated,
	KprContainerLayered,
	KprContainerMark,
	KprLayoutMeasureHorizontally,
	KprLayoutMeasureVertically,
	KprContainerPlace,
	KprLayoutPlaced,
	KprContainerPredict,
	KprContainerReflowing,
	KprContainerRemoving,
	KprContainerSetWindow,
	KprContainerShowing,
	KprContainerShown,
	KprContainerUpdate
};

FskErr KprLayoutNew(KprLayout *it, KprCoordinates coordinates, KprSkin skin, KprStyle style)
{
	FskErr err = kFskErrNone;
	KprLayout self;
	bailIfError(FskMemPtrNewClear(sizeof(KprLayoutRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprLayoutInstrumentation);
	self->dispatch = &KprLayoutDispatchRecord;
	self->flags = kprContainer | kprVisible;
	KprContentInitialize((KprContent)self, coordinates, skin, style);
bail:
	return err;
}

void KprLayoutMeasureHorizontally(void* it) 
{
	KprLayout self = it;
	if ((self->coordinates.horizontal & kprLeftRight) == kprLeftRight)
		kprDelegateMeasureHorizontally(self, self->coordinates.width);
	KprContainerMeasureHorizontally(it);
	if ((self->coordinates.horizontal & kprLeftRight) != kprLeftRight)
		self->coordinates.width = kprDelegateMeasureHorizontally(self, self->coordinates.width);
}

void KprLayoutMeasureVertically(void* it) 
{
	KprLayout self = it;
	if ((self->coordinates.vertical & kprTopBottom) == kprTopBottom)
		kprDelegateMeasureVertically(self, self->coordinates.height);
	KprContainerMeasureVertically(it);
	if ((self->coordinates.vertical & kprTopBottom) != kprTopBottom)
		self->coordinates.height = kprDelegateMeasureVertically(self, self->coordinates.height);
}

void KprLayoutPlaced(void* it) 
{
	KprLayout self = it;
	if ((self->flags & kprPlaced) && !(self->flags & kprDisplaying))
		kprDelegateAdapt(self);
	KprContainerPlaced(it);
}

/* LAYOUT ECMASCRIPT */

void KPR_Layout(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprCoordinatesRecord coordinates;
	KprSkin skin = NULL;
	KprStyle style = NULL;
	KprLayout self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	if ((c > 1) && xsTest(xsArg(1)))
		skin = kprGetHostData(xsArg(1), skin, skin);
	if ((c > 2) && xsTest(xsArg(2)))
		style = kprGetHostData(xsArg(2), skin, style);
	KprLayoutNew(&self, &coordinates, skin, style);
	kprContentConstructor(KPR_Layout);
}

/* CHAIN */

FskErr KprContentChainAdd(KprContentChain chain, KprContent content, UInt32 size, KprContentLink *result) 
{
	FskErr err = kFskErrNone;
	KprContentLink *linkAddress = &chain->first, link;
	while ((link = *linkAddress)) {
		if (link->content == content)
			return kFskErrDuplicateElement;
		linkAddress = &link->next;
	}
	bailIfError(FskMemPtrNew(size, &link));
	link->next = NULL;
	*linkAddress = link;
	link->content = content;
	*result = link;
bail:
	return err;
}

FskErr KprContentChainAppend(KprContentChain chain, void* it, UInt32 size, KprContentLink *result) 
{
	FskErr err = kFskErrNone;
	KprContentLink *linkAddress = &chain->first, link;
	while ((link = *linkAddress)) {
		linkAddress = &link->next;
	}
	if (!size)
		size = sizeof(KprContentLinkRecord);
	bailIfError(FskMemPtrNewClear(size, &link));
	*linkAddress = link;
	link->content = it;
	if (result)
		*result = link;
bail:
	return err;
}

Boolean KprContentChainContains(KprContentChain chain, void* it) 
{
	KprContentLink link = chain->first;
	while (link) {
		if (link->content == it)
			return true;
		link = link->next;
	}
	return false;
}

KprContentLink KprContentChainGetFirst(KprContentChain chain) 
{
	KprContentLink link = chain->first;
	if (link)
		chain->next = link->next;
	return link;
}

KprContentLink KprContentChainGetNext(KprContentChain chain) 
{
	KprContentLink link = chain->next;
	if (link)
		chain->next = link->next;
	return link;
}

Boolean KprContentChainIsEmpty(KprContentChain chain) 
{
	return chain->first ? false : true;
}

FskErr KprContentChainPrepend(KprContentChain chain, void* it, UInt32 size, KprContentLink *result) 
{
	FskErr err = kFskErrNone;
	KprContentLink link = NULL;
	if (!size)
		size = sizeof(KprContentLinkRecord);
	else if (size < sizeof(KprContentLinkRecord))
		return kFskErrInvalidParameter;
	bailIfError(FskMemPtrNewClear(size, &link));
	link->next = chain->first;
	chain->first = link;
	link->content = it;
	if (result)
		*result = link;
bail:
	return err;
}

FskErr KprContentChainRemove(KprContentChain chain, void* it) 
{
	KprContentLink *linkAddress = &chain->first, link;
	while ((link = *linkAddress)) {
		if (link->content == it) {
			if (chain->next == link)
				chain->next = link->next;
			*linkAddress = link->next;
			FskMemPtrDispose(link);
		}
		else
			linkAddress = &link->next;
	}
	return kFskErrNone;
}

