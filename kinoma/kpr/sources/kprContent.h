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
#ifndef __KPRCONTENT__
#define __KPRCONTENT__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

FskAPI(void) KprMarginsApply(KprMargins margins, FskRectangle src, FskRectangle dst);

FskAPI(void) KprCoordinatesSet(KprCoordinates coordinates,
	UInt16 horizontal, SInt32 left, SInt32 width, SInt32 right,
	UInt16 vertical, SInt32 top, SInt32 height, SInt32 bottom);

typedef void (*KprContentActivatedProc)(void* it, Boolean activateIt);
typedef void (*KprContentAddedProc)(void* it, KprContent content);
typedef void (*KprContentCascadeProc)(void* it, KprStyle style);
typedef void (*KprContentDisposeProc)(void* it);
typedef void (*KprContentDrawProc)(void* it, FskPort port, FskRectangle area);
typedef void (*KprContentFitProc)(void* it);
typedef FskBitmap (*KprContentGetBitmapProc)(void* it, FskPort port, Boolean* owned);
typedef KprContent (*KprContentHitProc)(void* it, SInt32 x, SInt32 y);
typedef void (*KprContentIdleProc)(void* it, double ticks);
typedef void (*KprContentInvalidatedProc)(void* it, FskRectangle area);
typedef void (*KprContentLayeredProc)(void* it, KprLayer layer, Boolean layerIt);
typedef void (*KprContentMarkProc)(void* it, xsMarkRoot markRoot);
typedef void (*KprContentMeasureProc)(void* it);
typedef void (*KprContentPlaceProc)(void* it);
typedef void (*KprContentPlacedProc)(void* it);
typedef void (*KprContentPredictProc)(void* it, FskRectangle area);
typedef void (*KprContentReflowingProc)(void* it, UInt32 flags);
typedef void (*KprContentRemovingProc)(void* it, KprContent content);
typedef void (*KprContentSetWindowProc)(void* it, KprShell shell, KprStyle style);
typedef void (*KprContentShowingProc)(void* it, Boolean showIt);
typedef void (*KprContentShownProc)(void* it, Boolean showIt);
typedef void (*KprContentUpdateProc)(void* it, FskPort port, FskRectangle area);

struct KprDispatchStruct {
	char* type;
	KprContentActivatedProc activated;
	KprContentAddedProc added;
	KprContentCascadeProc cascade;
	KprContentDisposeProc dispose;
	KprContentDrawProc draw;
	KprContentFitProc fitHorizontally;
	KprContentFitProc fitVertically;
	KprContentGetBitmapProc getBitmap;
	KprContentHitProc hit;
	KprContentIdleProc idle;
	KprContentInvalidatedProc invalidated;
	KprContentLayeredProc layered;
	KprContentMarkProc mark;
	KprContentMeasureProc measureHorizontally;
	KprContentMeasureProc measureVertically;
	KprContentPlaceProc place;
	KprContentPlacedProc placed;
	KprContentPredictProc predict;
	KprContentReflowingProc reflowing;
	KprContentRemovingProc removing;
	KprContentSetWindowProc setWindow;
	KprContentShowingProc showing;
	KprContentShownProc shown;
	KprContentUpdateProc update;
};

struct KprContentStruct {
	KprSlotPart;
	KprContentPart;
};

FskAPI(FskErr) KprContentNew(KprContent *self, KprCoordinates coordinates, KprSkin skin, KprStyle style);
FskAPI(void) KprContentCancel(KprContent self);
FskAPI(void) KprContentClose(KprContent self);
FskAPI(void) KprContentComplete(KprMessage message, void* it);
FskAPI(Boolean) KprContentFindFocus(KprContent self, UInt32 axis, SInt32 delta, KprContent current);
FskAPI(void) KprContentFromWindowCoordinates(KprContent self, SInt32 x0, SInt32 y0, SInt32 *x1, SInt32 *y1);
FskAPI(KprStyle) KprContentGetStyle(KprContent self);
FskAPI(SInt32) KprContentIndex(KprContent self);
FskAPI(void) KprContentInitialize(KprContent self, KprCoordinates coordinates, KprSkin skin, KprStyle style);
FskAPI(void) KprContentInvalidate(KprContent self);
FskAPI(Boolean) KprContentIsFocus(KprContent self);
FskAPI(Boolean) KprContentIsShown(KprContent self);
FskAPI(void) KprContentMoveBy(KprContent self, SInt32 dx, SInt32 dy);
FskAPI(void) KprContentPlacing(KprContent self);
FskAPI(void) KprContentReflow(KprContent self, UInt32 flags);
FskAPI(void) KprContentSetBehavior(KprContent self, KprBehavior behavior);
FskAPI(void) KprContentSetCoordinates(void* it, KprCoordinates coordinates);
FskAPI(void) KprContentSetDuration(KprContent self, double duration);
FskAPI(void) KprContentSetFraction(KprContent self, double fraction);
FskAPI(void) KprContentSetInterval(KprContent self, double interval);
FskAPI(void) KprContentSetSkin(KprContent self, KprSkin skin);
FskAPI(void) KprContentSetStyle(KprContent self, KprStyle style);
FskAPI(void) KprContentSetTime(KprContent self, double time);
FskAPI(void) KprContentShow(KprContent self, Boolean showIt);
FskAPI(void) KprContentSizeBy(KprContent self, SInt32 dx, SInt32 dy);
FskAPI(void) KprContentStart(KprContent self);
FskAPI(void) KprContentStop(KprContent self);
FskAPI(void) KprContentToWindowCoordinates(KprContent self, SInt32 x0, SInt32 y0, SInt32 *x1, SInt32 *y1);

extern void KprContentActivated(void* it, Boolean activateIt);
extern void KprContentAdded(void* it, KprContent content);
extern void KprContentCascade(void* it, KprStyle style);
extern void KprContentDispose(void* it);
extern void KprContentDraw(void* it, FskPort port, FskRectangle area);
extern void KprContentFitHorizontally(void* it);
extern void KprContentFitVertically(void* it);
extern FskBitmap KprContentGetBitmap(void* it, FskPort port, Boolean* owned);
extern KprContent KprContentHit(void* it, SInt32 x, SInt32 y);
extern void KprContentIdle(void* it, double ticks);
extern void KprContentLayered(void* it, KprLayer layer, Boolean layerIt);
extern void KprContentInvalidated(void* it, FskRectangle area);
extern void KprContentMark(void* it, xsMarkRoot markRoot);
extern void KprContentMeasureHorizontally(void* it);
extern void KprContentMeasureVertically(void* it);
extern void KprContentPlace(void* it);
extern void KprContentPlaced(void* it);
extern void KprContentPlaceHorizontally(KprContent self, SInt32 containerWidth);
extern void KprContentPlaceVertically(KprContent self, SInt32 containerHeight);
extern void KprContentPredict(void* it, FskRectangle area);
extern void KprContentReflowing(void* it, UInt32 flags);
extern void KprContentRemoving(void* it, KprContent content);
extern void KprContentSetWindow(void* it, KprShell shell, KprStyle style);
extern void KprContentShowing(void* it, Boolean showIt);
extern void KprContentShown(void* it, Boolean showIt);
extern void KprContentUpdate(void* it, FskPort port, FskRectangle area);


struct KprContainerStruct {
	KprSlotPart;
	KprContentPart;
	KprContainerPart;
};

FskAPI(FskErr) KprContainerNew(KprContainer *self, KprCoordinates coordinates, KprSkin skin, KprStyle style);
FskAPI(void) KprContainerAdd(KprContainer self, KprContent content);
FskAPI(KprContent) KprContainerContent(KprContainer self, SInt32 index);
FskAPI(SInt32) KprContainerCount(KprContainer self);
FskAPI(void) KprContainerEmpty(KprContainer self, SInt32 start, SInt32 stop);
FskAPI(void) KprContainerInsert(KprContainer self, KprContent content, KprContent before);
FskAPI(void) KprContainerRemove(KprContainer self, KprContent content);
FskAPI(void) KprContainerReplace(KprContainer self, KprContent current, KprContent content);
FskAPI(void) KprContainerSetTransition(KprContainer self, KprTransition transition);
FskAPI(void) KprContainerSwap(KprContainer self, KprContent content0, KprContent content1);

extern void KprContainerActivated(void* it, Boolean activateIt);
extern void KprContainerAdded(void* it, KprContent content);
extern void KprContainerCascade(void* it, KprStyle style);
extern void KprContainerDispose(void* it);
extern void KprContainerFitHorizontally(void* it);
extern void KprContainerFitVertically(void* it);
extern KprContent KprContainerHit(void* it, SInt32 x, SInt32 y);
extern void KprContentIdle(void* it, double interval);
extern void KprContainerLayered(void* it, KprLayer layer, Boolean layerIt);
extern void KprContainerInvalidated(void* it, FskRectangle area) ;
extern void KprContainerMark(void* it, xsMarkRoot markRoot);
extern void KprContainerMeasureHorizontally(void* it);
extern void KprContainerMeasureVertically(void* it);
extern void KprContainerPlace(void* it);
extern void KprContainerPlaced(void* it);
extern void KprContainerPlaceHorizontally(void* it);
extern void KprContainerPlaceVertically(void* it);
extern void KprContainerPredict(void* it, FskRectangle area);
extern void KprContainerRemoving(void* it, KprContent content);
extern void KprContainerReflowing(void* it, UInt32 flags);
extern void KprContainerSetWindow(void* it, KprShell shell, KprStyle style);
extern void KprContainerShowing(void* it, Boolean showIt);
extern void KprContainerShown(void* it, Boolean showIt);
extern void KprContainerUpdate(void* it, FskPort port, FskRectangle area);


struct KprLayoutStruct {
	KprSlotPart;
	KprContentPart;
	KprContainerPart;
};

FskAPI(FskErr) KprLayoutNew(KprLayout *self, KprCoordinates coordinates, KprSkin skin, KprStyle style);


extern FskErr KprContentChainAppend(KprContentChain chain, void* it, UInt32 size, KprContentLink *result);
extern Boolean KprContentChainContains(KprContentChain chain, void* it);
extern KprContentLink KprContentChainGetFirst(KprContentChain chain);
extern KprContentLink KprContentChainGetNext(KprContentChain chain);
extern Boolean KprContentChainIsEmpty(KprContentChain chain);
extern FskErr KprContentChainPrepend(KprContentChain chain, void* it, UInt32 size, KprContentLink *result);
extern FskErr KprContentChainRemove(KprContentChain chain, void* it);

#define KprBounds(CONTENT) (&((CONTENT)->bounds))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
