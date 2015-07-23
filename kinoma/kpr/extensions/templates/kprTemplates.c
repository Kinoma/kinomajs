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
#include "kpr.h"
#include "kprBehavior.h"
#include "kprCanvas.h"
#include "kprContent.h"
#include "kprImage.h"
#include "kprLabel.h"
#include "kprLayer.h"
#include "kprMedia.h"
#include "kprPort.h"
#include "kprScroller.h"
#include "kprSkin.h"
#include "kprStyle.h"
#include "kprTable.h"
#include "kprText.h"

#include "FskExtensions.h"

FskExport(FskErr) kprTemplates_fskLoad(FskLibrary it);
FskExport(FskErr) kprTemplates_fskUnload(FskLibrary it);

FskExport(FskErr) kprTemplates_fskLoad(FskLibrary it UNUSED)
{
	return kFskErrNone;
}

FskExport(FskErr) kprTemplates_fskUnload(FskLibrary it UNUSED)
{
	return kFskErrNone;
}

static void xsArgsToKprCoordinates(xsMachine *the, xsIntegerValue c, KprCoordinates coordinates);
static void xsArgsToKprFlags(xsMachine *the, xsIntegerValue c, UInt32* flags);
static void xsArgsToKprSkinAndStyle(xsMachine *the, xsIntegerValue c, KprSkin* skin, KprStyle* style);
static void xsArgsToKprString(xsMachine *the, xsIntegerValue c, xsStringValue* text);
static void xsArgsToKprURL(xsMachine *the, xsIntegerValue c);

static void KPR_AspectDictionary(xsMachine* the, xsSlot* slot, KprContent self); 
static void KPR_BehaviorDictionary(xsMachine* the, xsSlot* slot, KprContent self, xsSlot* data, xsSlot* context); 
static void KPR_ContentDictionary(xsMachine* the, xsSlot* slot, KprContent self); 
static void KPR_ContainerDictionary(xsMachine* the, xsSlot* slot, KprContainer self); 
static void KPR_EffectDictionary(xsMachine* the, xsSlot* slot, KprLayer self);
static void KPR_LayerDictionary(xsMachine* the, xsSlot* slot, KprLayer self); 
static void KPR_LabelDictionary(xsMachine* the, xsSlot* slot, KprLabel self); 
static void KPR_ScrollerDictionary(xsMachine* the, xsSlot* slot, KprScroller self); 
static void KPR_TextDictionary(xsMachine* the, xsSlot* slot, KprText self);
static void KPR_URLDictionary(xsMachine* the, xsSlot* slot);

static void KPR_ContentPatch(xsMachine *the);
static void KPR_ContentTemplate(xsMachine *the);
static void KPR_Content_template(xsMachine *the);
static void KPR_ContainerPatch(xsMachine *the);
static void KPR_ContainerTemplate(xsMachine *the);
static void KPR_Container_template(xsMachine *the);
static void KPR_LayerPatch(xsMachine *the);
static void KPR_LayerTemplate(xsMachine *the);
static void KPR_Layer_template(xsMachine *the);
static void KPR_LayoutPatch(xsMachine *the);
static void KPR_LayoutTemplate(xsMachine *the);
static void KPR_Layout_template(xsMachine *the);
static void KPR_ScrollerPatch(xsMachine *the);
static void KPR_ScrollerTemplate(xsMachine *the);
static void KPR_Scroller_template(xsMachine *the);
static void KPR_ColumnPatch(xsMachine *the);
static void KPR_ColumnTemplate(xsMachine *the);
static void KPR_Column_template(xsMachine *the);
static void KPR_LinePatch(xsMachine *the);
static void KPR_LineTemplate(xsMachine *the);
static void KPR_Line_template(xsMachine *the);
static void KPR_LabelPatch(xsMachine *the);
static void KPR_LabelTemplate(xsMachine *the);
static void KPR_Label_template(xsMachine *the);
static void KPR_TextPatch(xsMachine *the);
static void KPR_TextTemplate(xsMachine *the);
static void KPR_Text_template(xsMachine *the);
static void KPR_PicturePatch(xsMachine *the);
static void KPR_PictureTemplate(xsMachine *the);
static void KPR_Picture_template(xsMachine *the);
static void KPR_ThumbnailPatch(xsMachine *the);
static void KPR_ThumbnailTemplate(xsMachine *the);
static void KPR_Thumbnail_template(xsMachine *the);
static void KPR_MediaPatch(xsMachine *the);
static void KPR_MediaTemplate(xsMachine *the);
static void KPR_Media_template(xsMachine *the);
static void KPR_PortPatch(xsMachine *the);
static void KPR_PortTemplate(xsMachine *the);
static void KPR_Port_template(xsMachine *the);
static void KPR_CanvasPatch(xsMachine *the);
static void KPR_CanvasTemplate(xsMachine *the);
static void KPR_Canvas_template(xsMachine *the);

static void KPR_SkinAspect(xsMachine *the, xsSlot* slot, UInt32* flags);
static void KPR_SkinBorders(xsMachine *the, xsSlot* slot, KprSkinData data);
static void KPR_SkinBounds(xsMachine *the, xsSlot* slot, KprSkinData data);
static void KPR_SkinFillColor(xsMachine *the, xsSlot* slot, KprSkinData data);
static void KPR_SkinFillColors(xsMachine *the, xsSlot* slot, KprSkinData data);
static void KPR_SkinMargins(xsMachine *the, xsSlot* slot, KprSkinData data);
static void KPR_SkinStates(xsMachine *the, xsSlot* slot, KprSkinData data);
static void KPR_SkinStrokeColor(xsMachine *the, xsSlot* slot, KprSkinData data);
static void KPR_SkinStrokeColors(xsMachine *the, xsSlot* slot, KprSkinData data);
static void KPR_SkinTiles(xsMachine *the, xsSlot* slot, KprSkinData data, UInt32* flags);
static void KPR_SkinVariants(xsMachine *the, xsSlot* slot, KprSkinData data);
static void KPR_SkinPatch(xsMachine *the);

static void KPR_StyleColor(xsMachine *the, xsSlot* slot, KprStyle self);
static void KPR_StyleHorizontal(xsMachine *the, xsSlot* slot, KprStyle self);
static void KPR_StyleVertical(xsMachine *the, xsSlot* slot, KprStyle self);
static void KPR_StylePatch(xsMachine *the);

static void KPR_Template(xsMachine *the);
static void KPR_template(xsMachine *the);

static void KPR_container_recurse(xsMachine* the);

static void KPR_Handler_bind(xsMachine *the);

static void KPR_Behavior_Template(xsMachine *the);
static void KPR_Behavior_template(xsMachine *the);

void xsArgsToKprCoordinates(xsMachine *the, xsIntegerValue c, KprCoordinates coordinates)
{
	if (c > 0)
		xsSlotToKprCoordinates(the, &xsArg(0), coordinates);
	else {
		coordinates->left = 0;
		coordinates->width = 0;
		coordinates->right = 0;
		coordinates->top = 0;
		coordinates->height = 0;
		coordinates->bottom = 0;
	}
}

void xsArgsToKprFlags(xsMachine *the, xsIntegerValue c, UInt32* flags)
{
	if ((c > 1) && xsTest(xsArg(1)))
		*flags |= kprNoAlpha;
	if ((c > 2) && xsTest(xsArg(2)))
		*flags |= kprNoAcceleration;
}

void xsArgsToKprSkinAndStyle(xsMachine *the, xsIntegerValue c, KprSkin* skin, KprStyle* style)
{
	if ((c > 1) && xsTest(xsArg(1)))
		*skin = kprGetHostData(xsArg(1), skin, skin);
	if ((c > 2) && xsTest(xsArg(2)))
		*style = kprGetHostData(xsArg(2), skin, style);
}

void xsArgsToKprString(xsMachine *the, xsIntegerValue c, xsStringValue* text)
{
	if ((c > 3) && xsTest(xsArg(3)))
		*text = xsToString(xsArg(3));
}

void xsArgsToKprURL(xsMachine *the, xsIntegerValue c)
{
	if (c > 2)
		xsCall2_noResult(xsThis, xsID_load, xsArg(1), xsArg(2));
	else if (c > 1)
		xsCall1_noResult(xsThis, xsID_load, xsArg(1));
}

void KPR_AspectDictionary(xsMachine* the, xsSlot* slot, KprContent self) 
{
	xsStringValue string;
	if (xsFindString(*slot, xsID_aspect, &string)) {
		if (!FskStrCompare(string, "fill"))
			self->flags |= kprImageFill;
		else if (!FskStrCompare(string, "fit"))
			self->flags |= kprImageFit;
		else if (!FskStrCompare(string, "stretch"))
			self->flags |= kprImageFill | kprImageFit;
	}
}

void KPR_BehaviorDictionary(xsMachine *the, xsSlot* slot, KprContent self, xsSlot* data, xsSlot* context)
{
	KprBehavior behavior = NULL;
	if (xsFindResult(*slot, xsID_anchor)) {
		if (xsTest(*data)) {
			xsSetAt(*data, xsResult, xsThis);
		}
	}
#ifdef XS6	
	if (xsFindResult(*slot, xsID_Behavior)) {
		if (xsIsInstanceOf(xsResult, xsFunctionPrototype)) {
			fxPush(xsThis);
			fxPush(*data);
			fxPush(*context);
			fxPushCount(the, 3);
			fxPush(xsResult);
			fxNew(the);
			xsResult = fxPop();
			xsThrowIfFskErr(KprScriptBehaviorNew(&behavior, self, the, &xsResult));
			KprContentSetBehavior(self, behavior);
		}
	}
	else 
#endif	
	if (xsFindResult(*slot, xsID_behavior)) {
		if (xsIsInstanceOf(xsResult, xsObjectPrototype)) {
			xsVar(0) = xsResult;
			if (xsFindResult(xsVar(0), xsID_onCreate)) {
				(void)xsCallFunction3(xsResult, xsVar(0), xsThis, *data, *context);
			}
			xsThrowIfFskErr(KprScriptBehaviorNew(&behavior, self, the, &xsVar(0)));
			KprContentSetBehavior(self, behavior);
		}
	}
}

void KPR_ContentDictionary(xsMachine* the, xsSlot* slot, KprContent self) 
{
	xsBooleanValue boolean;
	xsNumberValue number;
	xsStringValue string;
	if (xsFindBoolean(*slot, xsID_active, &boolean)) {
		if (boolean)
			self->flags |= kprActive;
		else
			self->flags &= ~kprActive;
	}
	if (xsFindBoolean(*slot, xsID_backgroundTouch, &boolean)) {
		if (boolean)
			self->flags |= kprBackgroundTouch;
		else
			self->flags &= ~kprBackgroundTouch;
	}
	if (xsFindNumber(*slot, xsID_duration, &number))
		KprContentSetDuration(self, number);
	if (xsFindBoolean(*slot, xsID_exclusiveTouch, &boolean)) {
		if (boolean)
			self->flags |= kprExclusiveTouch;
		else
			self->flags &= ~kprExclusiveTouch;
	}
	if (xsFindNumber(*slot, xsID_fraction, &number))
		KprContentSetFraction(self, number);
	if (xsFindNumber(*slot, xsID_interval, &number))
		KprContentSetInterval(self, number);
	if (xsFindBoolean(*slot, xsID_multipleTouch, &boolean)) {
		if (boolean)
			self->flags |= kprMultipleTouch;
		else
			self->flags &= ~kprMultipleTouch;
	}
	if (xsFindString(*slot, xsID_name, &string)) {
		self->name = FskStrDoCopy(string);	
		xsThrowIfNULL(self->name);
	}
	if (xsFindResult(*slot, xsID_skin)) {
		KprSkin skin = NULL;
		if (xsTest(xsResult))
			skin = kprGetHostData(xsResult, skin, skin);
		KprContentSetSkin(self, skin);
	}
	if (xsFindNumber(*slot, xsID_state, &number))
		self->state = number;
	if (xsFindResult(*slot, xsID_style)) {
		KprStyle style = NULL;
		if (xsTest(xsResult))
			style = kprGetHostData(xsResult, style, style);
		KprContentSetStyle(self, style);
	}
	if (xsFindNumber(*slot, xsID_time, &number))
		KprContentSetTime(self, number);
	if (xsFindNumber(*slot, xsID_variant, &number))
		self->variant = number;
	if (xsFindBoolean(*slot, xsID_visible, &boolean)) {
		if (boolean)
			self->flags |= kprVisible;
		else
			self->flags &= ~kprVisible;
	}
}

void KPR_ContainerDictionary(xsMachine* the, xsSlot* slot, KprContainer self) 
{
	xsBooleanValue boolean;
	if (xsFindBoolean(*slot, xsID_clip, &boolean)) {
		if (boolean)
			self->flags |= kprClip;
		else
			self->flags &= ~kprClip;
	}
	if (xsFindResult(*slot, xsID_contents)) {
		(void)xsCall1(xsThis, xsID_recurse, xsResult);
	}
}

void KPR_EffectDictionary(xsMachine* the, xsSlot* slot, KprLayer self) 
{
	if (xsFindResult(*slot, xsID_effect)) {
		KprEffect effect = NULL;
		if (xsTest(xsResult))
			effect = kprGetHostData(xsResult, effect, effect);
		KprLayerSetEffect(self, effect);
	}
}

void KPR_LayerDictionary(xsMachine* the, xsSlot* slot, KprLayer self) 
{
	xsBooleanValue boolean;
	if (xsFindBoolean(*slot, xsID_acceleration, &boolean)) {
		if (boolean)
			self->flags &= ~kprNoAcceleration;
		else
			self->flags |= kprNoAcceleration;
	}
	if (xsFindBoolean(*slot, xsID_alpha, &boolean)) {
		if (boolean)
			self->flags &= ~kprNoAlpha;
		else
			self->flags |= kprNoAlpha;
	}
	if (xsFindBoolean(*slot, xsID_blocking, &boolean)) {
		if (boolean)
			self->flags |= kprBlocking;
		else
			self->flags &= ~kprBlocking;
	}
}

void KPR_LabelDictionary(xsMachine* the, xsSlot* slot, KprLabel self) 
{
	xsBooleanValue boolean;
	xsStringValue string;
	if (xsFindBoolean(*slot, xsID_editable, &boolean)) {
		if (boolean)
			self->flags |= kprTextEditable | kprTextSelectable;
		else
			self->flags &= ~kprTextEditable;
	}
	if (xsFindBoolean(*slot, xsID_hidden, &boolean)) {
		if (boolean)
			self->flags |= kprTextHidden;
		else
			self->flags &= ~kprTextHidden;
	}
	if (xsFindBoolean(*slot, xsID_selectable, &boolean)) {
		if (boolean)
			self->flags |= kprTextSelectable;
		else
			self->flags &= ~(kprTextEditable | kprTextSelectable);
	}
	if (xsFindString(*slot, xsID_string, &string))
		KprLabelSetString(self, string);
}

void KPR_ScrollerDictionary(xsMachine* the, xsSlot* slot, KprScroller self) 
{
	xsBooleanValue boolean;
	if (xsFindBoolean(*slot, xsID_loop, &boolean)) {
		KprScrollerLoop(self, boolean);
	}
}

void KPR_TextDictionary(xsMachine* the, xsSlot* slot, KprText self) 
{
	xsBooleanValue boolean;
	xsStringValue string;
	if (xsFindBoolean(*slot, xsID_editable, &boolean)) {
		if (boolean)
			self->flags |= kprTextEditable | kprTextSelectable;
		else
			self->flags &= ~kprTextEditable;
	}
	if (xsFindBoolean(*slot, xsID_hidden, &boolean)) {
		if (boolean)
			self->flags |= kprTextHidden;
		else
			self->flags &= ~kprTextHidden;
	}
	if (xsFindBoolean(*slot, xsID_selectable, &boolean)) {
		if (boolean)
			self->flags |= kprTextSelectable;
		else
			self->flags &= ~(kprTextEditable | kprTextSelectable);
	}
	if (xsFindResult(*slot, xsID_blocks)) {
		(void)xsCall1(xsThis, xsID_format, xsResult);
	}
	else if (xsFindString(*slot, xsID_string, &string)) {
		KprTextBegin(self);
		KprTextConcatString(self, string);
		KprTextEnd(self);
	}
}

void KPR_URLDictionary(xsMachine* the, xsSlot* slot) 
{
	if (xsFindResult(*slot, xsID_mime))
		xsVar(0) = xsResult;
	else
		xsVar(0) = xsUndefined;
	if (xsFindResult(*slot, xsID_url))
		xsCall2_noResult(xsThis, xsID_load, xsResult, xsVar(0));
}

void KPR_ContentPatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		KprSkin skin = NULL;
		KprStyle style = NULL;
		KprContent self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsArgsToKprSkinAndStyle(the, c, &skin, &style);
		KprContentNew(&self, &coordinates, skin, style);
		kprContentConstructor(KPR_Content);
		if ((c == 1) && xsTest(xsArg(0))) {
            xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), self);
			KPR_BehaviorDictionary(the, &xsArg(0), self, &xsVar(1), &xsVar(1));
            xsLeaveSandbox();
		}
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_ContentTemplate(the);
	}
}

void KPR_ContentTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprContent self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprContentNew(&self, &coordinates, NULL, NULL);
	kprContentConstructor(KPR_Content);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), self);
		KPR_BehaviorDictionary(the, &xsArg(1), self, &xsArg(0), &xsArg(1));
		xsLeaveSandbox();
		xsResult = xsThis;
	}
}

void KPR_Content_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Content);
	KPR_template(the);
}

void KPR_ContainerPatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		KprSkin skin = NULL;
		KprStyle style = NULL;
		KprContainer self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsArgsToKprSkinAndStyle(the, c, &skin, &style);
		KprContainerNew(&self, &coordinates, skin, style);
		kprContentConstructor(KPR_Container);
		if ((c == 1) && xsTest(xsArg(0))) {
			xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), (KprContent)self);
			KPR_ContainerDictionary(the, &xsArg(0), self);
			KPR_BehaviorDictionary(the, &xsArg(0), (KprContent)self, &xsVar(1), &xsVar(1));
			xsLeaveSandbox();
		}
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_ContainerTemplate(the);
	}
}

void KPR_ContainerTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprContainer self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprContainerNew(&self, &coordinates, NULL, NULL);
	kprContentConstructor(KPR_Container);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_ContainerDictionary(the, &xsArg(1), self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_Container_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Container);
	KPR_template(the);
}

void KPR_LayerPatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		UInt32 flags = 0;
		KprLayer self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsArgsToKprFlags(the, c, &flags);
		KprLayerNew(&self, &coordinates, flags);
		kprContentConstructor(KPR_Layer);
		if ((c == 1) && xsTest(xsArg(0))) {
			xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), (KprContent)self);
			KPR_ContainerDictionary(the, &xsArg(0), (KprContainer)self);
			KPR_LayerDictionary(the, &xsArg(0), self);
			KPR_EffectDictionary(the, &xsArg(0), self);
			KPR_BehaviorDictionary(the, &xsArg(0), (KprContent)self, &xsVar(1), &xsVar(1));
			xsLeaveSandbox();
		}
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_LayerTemplate(the);
	}
}

void KPR_LayerTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprLayer self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprLayerNew(&self, &coordinates, 0);
	kprContentConstructor(KPR_Layer);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_ContainerDictionary(the, &xsArg(1), (KprContainer)self);
		KPR_LayerDictionary(the, &xsArg(1), self);
		KPR_EffectDictionary(the, &xsArg(1), self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_Layer_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Layer);
	KPR_template(the);
}

void KPR_LayoutPatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		KprSkin skin = NULL;
		KprStyle style = NULL;
		KprLayout self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsArgsToKprSkinAndStyle(the, c, &skin, &style);
		KprLayoutNew(&self, &coordinates, skin, style);
		kprContentConstructor(KPR_Layout);
		if ((c == 1) && xsTest(xsArg(0))) {
			xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), (KprContent)self);
			KPR_ContainerDictionary(the, &xsArg(0), (KprContainer)self);
			KPR_BehaviorDictionary(the, &xsArg(0), (KprContent)self, &xsVar(1), &xsVar(1));
			xsLeaveSandbox();
		}
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_LayoutTemplate(the);
	}
}
		
void KPR_LayoutTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprLayout self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprLayoutNew(&self, &coordinates, NULL, NULL);
	kprContentConstructor(KPR_Layout);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_ContainerDictionary(the, &xsArg(1), (KprContainer)self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_Layout_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Layout);
	KPR_template(the);
}

void KPR_ScrollerPatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		KprSkin skin = NULL;
		KprStyle style = NULL;
		KprScroller self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsArgsToKprSkinAndStyle(the, c, &skin, &style);
		xsThrowIfFskErr(KprScrollerNew(&self, &coordinates, skin, style));
		kprContentConstructor(KPR_Scroller);
		if ((c == 1) && xsTest(xsArg(0))) {
			xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), (KprContent)self);
			KPR_ContainerDictionary(the, &xsArg(0), (KprContainer)self);
			KPR_ScrollerDictionary(the, &xsArg(0), self);
			KPR_BehaviorDictionary(the, &xsArg(0), (KprContent)self, &xsVar(1), &xsVar(1));
			xsLeaveSandbox();
		}
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_ScrollerTemplate(the);
	}
}

void KPR_ScrollerTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprScroller self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprScrollerNew(&self, &coordinates, NULL, NULL);
	kprContentConstructor(KPR_Scroller);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_ContainerDictionary(the, &xsArg(1), (KprContainer)self);
		KPR_ScrollerDictionary(the, &xsArg(1), self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_Scroller_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Scroller);
	KPR_template(the);
}

void KPR_ColumnPatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		KprSkin skin = NULL;
		KprStyle style = NULL;
		KprColumn self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsArgsToKprSkinAndStyle(the, c, &skin, &style);
		xsThrowIfFskErr(KprColumnNew(&self, &coordinates, skin, style));
		kprContentConstructor(KPR_Column);
		if ((c == 1) && xsTest(xsArg(0))) {
			xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), (KprContent)self);
			KPR_ContainerDictionary(the, &xsArg(0), (KprContainer)self);
			KPR_BehaviorDictionary(the, &xsArg(0), (KprContent)self, &xsVar(1), &xsVar(1));
			xsLeaveSandbox();
		}
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_ColumnTemplate(the);
	}
}

void KPR_ColumnTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprColumn self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprColumnNew(&self, &coordinates, NULL, NULL);
	kprContentConstructor(KPR_Column);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_ContainerDictionary(the, &xsArg(1), (KprContainer)self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_Column_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Column);
	KPR_template(the);
}

void KPR_LinePatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		KprSkin skin = NULL;
		KprStyle style = NULL;
		KprLine self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsArgsToKprSkinAndStyle(the, c, &skin, &style);
		xsThrowIfFskErr(KprLineNew(&self, &coordinates, skin, style));
		kprContentConstructor(KPR_Line);
		if ((c == 1) && xsTest(xsArg(0))) {
			xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), (KprContent)self);
			KPR_ContainerDictionary(the, &xsArg(0), (KprContainer)self);
			KPR_BehaviorDictionary(the, &xsArg(0), (KprContent)self, &xsVar(1), &xsVar(1));
			xsLeaveSandbox();
		}
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_LineTemplate(the);
	}
}

void KPR_LineTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprLine self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprLineNew(&self, &coordinates, NULL, NULL);
	kprContentConstructor(KPR_Line);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_ContainerDictionary(the, &xsArg(1), (KprContainer)self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_Line_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Line);
	KPR_template(the);
}

void KPR_LabelPatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		KprSkin skin = NULL;
		KprStyle style = NULL;
		xsStringValue string = "";
		KprLabel self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsArgsToKprSkinAndStyle(the, c, &skin, &style);
		xsArgsToKprString(the, c, &string);
		xsThrowIfFskErr(KprLabelNew(&self, &coordinates, skin, style, string));
		kprContentConstructor(KPR_Label);
		if ((c == 1) && xsTest(xsArg(0))) {
			xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), (KprContent)self);
			KPR_LabelDictionary(the, &xsArg(0), self);
			KPR_BehaviorDictionary(the, &xsArg(0), (KprContent)self, &xsVar(1), &xsVar(1));
			xsLeaveSandbox();
		}
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_LabelTemplate(the);
	}
}

void KPR_LabelTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprLabel self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprLabelNew(&self, &coordinates, NULL, NULL, "");
	kprContentConstructor(KPR_Label);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_LabelDictionary(the, &xsArg(1), self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_Label_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Label);
	KPR_template(the);
}

void KPR_TextPatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		KprSkin skin = NULL;
		KprStyle style = NULL;
		xsStringValue string = NULL;
		KprText self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsArgsToKprSkinAndStyle(the, c, &skin, &style);
		xsArgsToKprString(the, c, &string);
		xsThrowIfFskErr(KprTextNew(&self, &coordinates, skin, style, string));
		kprContentConstructor(KPR_Text);
		if ((c == 1) && xsTest(xsArg(0))) {
			xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), (KprContent)self);
			KPR_TextDictionary(the, &xsArg(0), self);
			KPR_BehaviorDictionary(the, &xsArg(0), (KprContent)self, &xsVar(1), &xsVar(1));
			xsLeaveSandbox();
		}
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_TextTemplate(the);
	}
}

void KPR_TextTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprText self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprTextNew(&self, &coordinates, NULL, NULL, NULL);
	kprContentConstructor(KPR_Text);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_TextDictionary(the, &xsArg(1), self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_Text_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Text);
	KPR_template(the);
}

void KPR_PicturePatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		KprPicture self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsThrowIfFskErr(KprPictureNew(&self, &coordinates));
		kprContentConstructor(KPR_Picture);
		if ((c == 1) && xsTest(xsArg(0))) {
			xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), (KprContent)self);
			KPR_AspectDictionary(the, &xsArg(0), (KprContent)self);
			KPR_EffectDictionary(the, &xsArg(0), (KprLayer)self);
			KPR_BehaviorDictionary(the, &xsArg(0), (KprContent)self, &xsVar(1), &xsVar(1));
			KPR_URLDictionary(the, &xsArg(0));
			xsLeaveSandbox();
		}
		else
			xsArgsToKprURL(the, c);
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_PictureTemplate(the);
	}
}

void KPR_PictureTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprPicture self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprPictureNew(&self, &coordinates);
	kprContentConstructor(KPR_Picture);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_AspectDictionary(the, &xsArg(1), (KprContent)self);
		KPR_EffectDictionary(the, &xsArg(1), (KprLayer)self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		KPR_URLDictionary(the, &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_Picture_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Picture);
	KPR_template(the);
}

void KPR_ThumbnailPatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		KprThumbnail self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsThrowIfFskErr(KprThumbnailNew(&self, &coordinates));
		kprContentConstructor(KPR_Thumbnail);
		if ((c == 1) && xsTest(xsArg(0))) {
			xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), (KprContent)self);
			KPR_AspectDictionary(the, &xsArg(0), (KprContent)self);
			KPR_EffectDictionary(the, &xsArg(0), (KprLayer)self);
			KPR_BehaviorDictionary(the, &xsArg(0), (KprContent)self, &xsVar(1), &xsVar(1));
			KPR_URLDictionary(the, &xsArg(0));
			xsLeaveSandbox();
		}
		else
			xsArgsToKprURL(the, c);
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_ThumbnailTemplate(the);
	}
}

void KPR_ThumbnailTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprThumbnail self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprThumbnailNew(&self, &coordinates);
	kprContentConstructor(KPR_Thumbnail);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_AspectDictionary(the, &xsArg(1), (KprContent)self);
		KPR_EffectDictionary(the, &xsArg(1), (KprLayer)self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		KPR_URLDictionary(the, &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_Thumbnail_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Thumbnail);
	KPR_template(the);
}

void KPR_MediaPatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		KprMedia self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsThrowIfFskErr(KprMediaNew(&self, &coordinates));
		kprContentConstructor(KPR_Media);
		if ((c == 1) && xsTest(xsArg(0))) {
			xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), (KprContent)self);
			KPR_AspectDictionary(the, &xsArg(0), (KprContent)self);
			KPR_BehaviorDictionary(the, &xsArg(0), (KprContent)self, &xsVar(1), &xsVar(1));
			KPR_URLDictionary(the, &xsArg(0));
			xsLeaveSandbox();
		}
		else
			xsArgsToKprURL(the, c);
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_MediaTemplate(the);
	}
}

void KPR_MediaTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprMedia self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprMediaNew(&self, &coordinates);
	kprContentConstructor(KPR_Media);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_AspectDictionary(the, &xsArg(1), (KprContent)self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		KPR_URLDictionary(the, &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_Media_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Media);
	KPR_template(the);
}

void KPR_PortPatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		KprPort self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsThrowIfFskErr(KprPortNew(&self, &coordinates));
		kprContentConstructor(KPR_Port);
		if ((c == 1) && xsTest(xsArg(0))) {
			xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), (KprContent)self);
			KPR_BehaviorDictionary(the, &xsArg(0), (KprContent)self, &xsVar(1), &xsVar(1));
			xsLeaveSandbox();
		}
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_PortTemplate(the);
	}
}

void KPR_PortTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprPort self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprPortNew(&self, &coordinates);
	kprContentConstructor(KPR_Port);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_Port_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Port);
	KPR_template(the);
}

void KPR_CanvasPatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsIntegerValue c = xsToInteger(xsArgc);
		KprCoordinatesRecord coordinates;
		KprCanvas self;
		xsVars(2);
		xsArgsToKprCoordinates(the, c, &coordinates);
		xsThrowIfFskErr(KprCanvasNew(&self, &coordinates));
		kprContentConstructor(KPR_Canvas);
		if ((c == 1) && xsTest(xsArg(0))) {
			xsEnterSandbox();
			KPR_ContentDictionary(the, &xsArg(0), (KprContent)self);
			KPR_BehaviorDictionary(the, &xsArg(0), (KprContent)self, &xsVar(1), &xsVar(1));
			xsLeaveSandbox();
		}
		xsResult = xsThis;
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		KPR_CanvasTemplate(the);
	}
}

void KPR_CanvasTemplate(xsMachine *the)
{
	KprCoordinatesRecord coordinates;
	KprCanvas self;
	xsVars(1);
	xsSlotToKprCoordinates(the, &xsArg(1), &coordinates);
	KprCanvasNew(&self, &coordinates);
	kprContentConstructor(KPR_Canvas);
	if (xsTest(xsArg(1))) {
		xsEnterSandbox();
		KPR_ContentDictionary(the, &xsArg(1), (KprContent)self);
		KPR_BehaviorDictionary(the, &xsArg(1), (KprContent)self, &xsArg(0), &xsArg(1));
		xsLeaveSandbox();
	}
	xsResult = xsThis;
}

void KPR_Canvas_template(xsMachine *the)
{
	xsThis = xsGet(xsGlobal, xsID__Canvas);
	KPR_template(the);
}

void KPR_SkinAspect(xsMachine *the, xsSlot* slot, UInt32* flags)
{
	char* aspect = xsToString(*slot);
	if (!FskStrCompare(aspect, "fill"))
		*flags |= kprImageFill;
	else if (!FskStrCompare(aspect, "fit"))
		*flags |= kprImageFit;
	else if (!FskStrCompare(aspect, "stretch"))
		*flags |= kprImageFill | kprImageFit;
}

void KPR_SkinBorders(xsMachine *the, xsSlot* slot, KprSkinData data)
{
	(void)xsFindInteger(*slot, xsID_left, &data->color.borders.left);
	(void)xsFindInteger(*slot, xsID_right, &data->color.borders.right);
	(void)xsFindInteger(*slot, xsID_top, &data->color.borders.top);
	(void)xsFindInteger(*slot, xsID_bottom, &data->color.borders.bottom);
}

void KPR_SkinBounds(xsMachine *the, xsSlot* slot, KprSkinData data)
{
	(void)xsFindInteger(*slot, xsID_x, &data->pattern.bounds.x);
	(void)xsFindInteger(*slot, xsID_y, &data->pattern.bounds.y);
	(void)xsFindInteger(*slot, xsID_width, &data->pattern.bounds.width);
	(void)xsFindInteger(*slot, xsID_height, &data->pattern.bounds.height);
}

void KPR_SkinFillColor(xsMachine *the, xsSlot* slot, KprSkinData data)
{
	FskColorRGBARecord color;
	if (KprParseColor(the, xsToString(*slot), &color)) {
		data->color.fill[0] = color;
		data->color.fill[1] = color;
		data->color.fill[2] = color;
		data->color.fill[3] = color;
	}
}

void KPR_SkinFillColors(xsMachine *the, xsSlot* slot, KprSkinData data)
{
	xsIntegerValue i, l = xsToInteger(xsGet(*slot, xsID_length));
	if (l > 4) l = 4;
	for (i = 0; i < l; i++) {
		FskColorRGBARecord color;
		xsVar(0) = xsGetAt(*slot, xsInteger(i));
		if (xsTest(xsVar(0))) {
			if (KprParseColor(the, xsToString(xsVar(0)), &color))
				data->color.fill[i] = color;
		}
	}
}

void KPR_SkinMargins(xsMachine *the, xsSlot* slot, KprSkinData data)
{
	(void)xsFindInteger(*slot, xsID_left, &data->pattern.margins.left);
	(void)xsFindInteger(*slot, xsID_right, &data->pattern.margins.right);
	(void)xsFindInteger(*slot, xsID_top, &data->pattern.margins.top);
	(void)xsFindInteger(*slot, xsID_bottom, &data->pattern.margins.bottom);
}

void KPR_SkinStates(xsMachine *the, xsSlot* slot, KprSkinData data)
{
	data->pattern.delta.y = xsToInteger(*slot);
}

void KPR_SkinStrokeColor(xsMachine *the, xsSlot* slot, KprSkinData data)
{
	FskColorRGBARecord color;
	if (KprParseColor(the, xsToString(*slot), &color)) {
		data->color.stroke[0] = color;
		data->color.stroke[1] = color;
		data->color.stroke[2] = color;
		data->color.stroke[3] = color;
	}
}

void KPR_SkinStrokeColors(xsMachine *the, xsSlot* slot, KprSkinData data)
{
	xsIntegerValue i, l = xsToInteger(xsGet(*slot, xsID_length));
	if (l > 4) l = 4;
	for (i = 0; i < l; i++) {
		FskColorRGBARecord color;
		xsVar(0) = xsGetAt(*slot, xsInteger(i));
		if (xsTest(xsVar(0))) {
			if (KprParseColor(the, xsToString(xsVar(0)), &color))
				data->color.stroke[i] = color;
		}
	}
}

void KPR_SkinTiles(xsMachine *the, xsSlot* slot, KprSkinData data, UInt32* flags)
{
	if (xsFindInteger(*slot, xsID_left, &data->pattern.tiles.left))
		*flags |= kprRepeatX;
	if (xsFindInteger(*slot, xsID_right, &data->pattern.tiles.right))
		*flags |= kprRepeatX;
	if (xsFindInteger(*slot, xsID_top, &data->pattern.tiles.top))
		*flags |= kprRepeatY;
	if (xsFindInteger(*slot, xsID_bottom, &data->pattern.tiles.bottom))
		*flags |= kprRepeatY;
}

void KPR_SkinVariants(xsMachine *the, xsSlot* slot, KprSkinData data)
{
	data->pattern.delta.x = xsToInteger(*slot);
}

void KPR_SkinPatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		KprSkin self;
		KprShell shell = gShell;
		xsIntegerValue c = xsToInteger(xsArgc);
		UInt32 flags = 0;
		KprSkinDataRecord data;
		xsVars(1);
		FskMemSet(&data, 0, sizeof(data));
		xsEnterSandbox();
		if (c > 0) {
			if (xsTypeOf(xsArg(0)) == xsStringType) {
				KPR_SkinFillColor(the, &xsArg(0), &data);
				if ((c > 1) && xsTest(xsArg(1)))
					KPR_SkinBorders(the, &xsArg(1), &data);
				if (c > 2) {
					if (xsTypeOf(xsArg(2)) == xsStringType)
						KPR_SkinStrokeColor(the, &xsArg(2), &data);
					else if (xsIsInstanceOf(xsArg(2), xsArrayPrototype))
						KPR_SkinStrokeColors(the, &xsArg(2), &data);
				}
			}
			else if (xsIsInstanceOf(xsArg(0), xsArrayPrototype)) {
				KPR_SkinFillColors(the, &xsArg(0), &data);
				if ((c > 1) && xsTest(xsArg(1)))
					KPR_SkinBorders(the, &xsArg(1), &data);
				if (c > 2) {
					if (xsTypeOf(xsArg(2)) == xsStringType)
						KPR_SkinStrokeColor(the, &xsArg(2), &data);
					else if (xsIsInstanceOf(xsArg(2), xsArrayPrototype))
						KPR_SkinStrokeColors(the, &xsArg(2), &data);
				}
			}
			else if (xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID_KPR), xsID_texture))) {
				flags |= kprPattern;
				data.pattern.texture = xsGetHostData(xsArg(0));
				if ((c > 1) && xsTest(xsArg(1))) 
					KPR_SkinBounds(the, &xsArg(1), &data);
				if (c > 2)
					KPR_SkinVariants(the, &xsArg(2), &data);
				if (c > 3)
					KPR_SkinStates(the, &xsArg(3), &data);
				if ((c > 4) && xsTest(xsArg(4)))
					KPR_SkinTiles(the, &xsArg(4), &data, &flags);
				if ((c > 5) && xsTest(xsArg(5)))
					KPR_SkinMargins(the, &xsArg(5), &data);
				if ((c > 6) && xsTest(xsArg(6)))
					KPR_SkinAspect(the, &xsArg(6), &flags);
			}
			else if (xsTest(xsArg(0))) {
				if (xsFindResult(xsArg(0), xsID_texture)) {
					flags |= kprPattern;
					data.pattern.texture = xsGetHostData(xsResult);
					KPR_SkinBounds(the, &xsArg(0), &data);
					if (xsFindResult(xsArg(0), xsID_variants))
						KPR_SkinVariants(the, &xsResult, &data);
					if (xsFindResult(xsArg(0), xsID_states))
						KPR_SkinStates(the, &xsResult, &data);
					if (xsFindResult(xsArg(0), xsID_tiles))
						KPR_SkinTiles(the, &xsResult, &data, &flags);
					if (xsFindResult(xsArg(0), xsID_margins))
						KPR_SkinMargins(the, &xsResult, &data);
					if (xsFindResult(xsArg(0), xsID_aspect))
						KPR_SkinAspect(the, &xsResult, &flags);
				}
				else {
					if (xsFindResult(xsArg(0), xsID_borders))
						KPR_SkinBorders(the, &xsResult, &data);
					if (xsFindResult(xsArg(0), xsID_fill)) {
						if (xsTypeOf(xsResult) == xsStringType)
							KPR_SkinFillColor(the, &xsResult, &data);
						else if (xsIsInstanceOf(xsResult, xsArrayPrototype))
							KPR_SkinFillColors(the, &xsResult, &data);
					}
					if (xsFindResult(xsArg(0), xsID_stroke)) {
						if (xsTypeOf(xsResult) == xsStringType)
							KPR_SkinStrokeColor(the, &xsResult, &data);
						else if (xsIsInstanceOf(xsResult, xsArrayPrototype))
							KPR_SkinStrokeColors(the, &xsResult, &data);
					}
				}
			}
		}
		xsLeaveSandbox();
		KprSkinNew(&self, (KprContext)shell, flags, &data);
		kprVolatileConstructor(KPR_Skin);
		xsResult = xsThis;
	}
	else {
		xsResult = xsCall1(xsFunction, xsID_template, xsArg(0));
	}
}

void KPR_StyleColor(xsMachine *the, xsSlot* slot, KprStyle self)
{
	FskColorRGBARecord color;
	if (xsIsInstanceOf(*slot, xsArrayPrototype)) {
		xsIntegerValue i, l = xsToInteger(xsGet(*slot, xsID_length));
		if (l > 4) l = 4;
		for (i = 0; i < l; i++) {
			xsVar(0) = xsGetAt(*slot, xsInteger(i));
			if (xsTest(xsVar(0)))
				if (KprParseColor(the, xsToString(xsVar(0)), &color))
					KprStyleSetColor(self, i, &color);
		}
	}
	else {
		if (KprParseColor(the, xsToString(*slot), &color))
			KprStyleSetColor(self, -1, &color);
	}
}

void KPR_StyleHorizontal(xsMachine *the, xsSlot* slot, KprStyle self)
{
	UInt16 alignment;
	if (KprParseHorizontalAlignment(xsToString(*slot), &alignment))
		KprStyleSetHorizontalAlignment(self, alignment);
}

void KPR_StyleVertical(xsMachine *the, xsSlot* slot, KprStyle self)
{
	UInt16 alignment;
	if (KprParseVerticalAlignment(xsToString(*slot), &alignment))
		KprStyleSetVerticalAlignment(self, alignment);
}

void KPR_StylePatch(xsMachine *the)
{
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		KprStyle self;
		KprShell shell = gShell;
		xsIntegerValue c = xsToInteger(xsArgc);
		xsVars(1);
		KprStyleNew(&self, (KprContext)shell, NULL, NULL);
		kprVolatileConstructor(KPR_Style);
		if ((c == 1) && xsTest(xsArg(0)) && (xsTypeOf(xsArg(0)) != xsStringType)) {
			xsIntegerValue integer;
			xsEnterSandbox();
			if (xsFindResult(xsArg(0), xsID_font))
				xsSet(xsThis, xsID_font, xsResult);
			if (xsFindResult(xsArg(0), xsID_horizontal))
				KPR_StyleHorizontal(the, &xsResult, self);
			if (xsFindResult(xsArg(0), xsID_vertical))
				KPR_StyleVertical(the, &xsResult, self);
			if (xsFindResult(xsArg(0), xsID_color))
				KPR_StyleColor(the, &xsResult, self);
			if (xsFindInteger(xsArg(0), xsID_indentation, &integer))
				KprStyleSetIndentation(self, integer);
			if (xsFindInteger(xsArg(0), xsID_leading, &integer))
				KprStyleSetLineHeight(self, integer);
			if (xsFindInteger(xsArg(0), xsID_lines, &integer))
				KprStyleSetLineCount(self, integer);
			if (xsFindInteger(xsArg(0), xsID_size, &integer))
				KprStyleSetTextSize(self, integer);
			if (xsFindInteger(xsArg(0), xsID_left, &integer))
				KprStyleSetMarginLeft(self, integer);
			if (xsFindInteger(xsArg(0), xsID_right, &integer))
				KprStyleSetMarginRight(self, integer);
			if (xsFindInteger(xsArg(0), xsID_top, &integer))
				KprStyleSetMarginTop(self, integer);
			if (xsFindInteger(xsArg(0), xsID_bottom, &integer))
				KprStyleSetMarginBottom(self, integer);
			xsLeaveSandbox();
		}
		else {
			if ((c > 0) && xsTest(xsArg(0)))
				xsSet(xsThis, xsID_font, xsArg(0));
			if ((c > 1) && xsTest(xsArg(1)))
				KPR_StyleColor(the, &xsArg(1), self);
			if ((c > 2) && xsTest(xsArg(2)))
				KPR_StyleHorizontal(the, &xsArg(2), self);
			if ((c > 3) && xsTest(xsArg(3)))
				KprStyleSetMarginLeft(self, xsToInteger(xsArg(3)));
			if ((c > 4) && xsTest(xsArg(4)))
				KprStyleSetMarginRight(self, xsToInteger(xsArg(4)));
			if ((c > 5) && xsTest(xsArg(5)))
				KprStyleSetIndentation(self, xsToInteger(xsArg(5)));
			if ((c > 6) && xsTest(xsArg(6)))
				KPR_StyleVertical(the, &xsArg(6), self);
			if ((c > 7) && xsTest(xsArg(7)))
				KprStyleSetMarginTop(self, xsToInteger(xsArg(7)));
			if ((c > 8) && xsTest(xsArg(8)))
				KprStyleSetMarginBottom(self, xsToInteger(xsArg(8)));
			if ((c > 9) && xsTest(xsArg(9)))
				KprStyleSetLineHeight(self, xsToInteger(xsArg(9)));
			if ((c > 10) && xsTest(xsArg(10)))
				KprStyleSetLineCount(self, xsToInteger(xsArg(10)));
		}
		xsResult = xsThis;
	}
	else {
		xsResult = xsCall1(xsFunction, xsID_template, xsArg(0));
	}
}

void KPR_Template(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsVars(5);
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (!xsIsInstanceOf(xsThis, xsResult)) {
		xsThis = xsNewInstanceOf(xsResult);
	}
	if (c > 0)
		xsVar(0) = xsArg(0);
	if (c > 1)
		xsVar(1) = xsArg(1);
	xsVar(2) = xsGet(xsFunction, xsID_it);
	xsEnterSandbox();
	xsVar(3) = xsCallFunction1(xsVar(2), xsThis, xsVar(0));
	if (xsTest(xsVar(1))) {
		if (xsFindResult(xsVar(3), xsID_contents))
			xsVar(4) = xsResult;
		else
			xsVar(4) = xsUndefined;
		fxPush(xsVar(3));
		fxPush(xsVar(1));
		fxCopyObject(the);
		the->stack++;
		if (xsTest(xsVar(4))) {
			if (xsFindResult(xsVar(1), xsID_contents)) {
				xsVar(4) = xsCall1(xsVar(4), xsID_concat, xsResult);
				xsSet(xsVar(3), xsID_contents, xsVar(4));
			}
		}
	}
	xsLeaveSandbox();
	xsVar(2) = xsGet(xsFunction, xsID_constructor);
	xsResult = xsCallFunction2(xsVar(2), xsThis, xsVar(0), xsVar(3));
}

void KPR_template(xsMachine *the)
{
	xsVars(2);
	xsAssert(xsIsInstanceOf(xsArg(0), xsFunctionPrototype));
	xsVar(0) = xsGet(xsThis, xsID_prototype);
	xsVar(1) = xsGet(xsVar(0), xsID_constructor);
	xsResult = xsNewHostConstructor(KPR_Template, 2, xsVar(0));
	xsSet(xsVar(0), xsID_constructor, xsVar(1));
	xsVar(0) = xsGet(xsGlobal, xsID_template);
	xsNewHostProperty(xsResult, xsID_constructor, xsThis, xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_it, xsArg(0), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_template, xsVar(0), xsDefault, xsDontScript);
}

void KPR_container_recurse(xsMachine* the) 
{
	if (xsIsInstanceOf(xsArg(0), xsArrayPrototype)) {
		xsIntegerValue c = xsToInteger(xsGet(xsArg(0), xsID_length)), i;
		for (i = 0; i < c; i++) {
			xsResult = xsGetAt(xsArg(0), xsInteger(i));
			xsCall1(xsThis, xsID_recurse, xsResult);
		}
	}
	else if (xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
		KprContainer self = kprGetHostData(xsThis, container, container);
		KprContent content = kprGetHostData(xsArg(0), content, content);
		xsAssert(content->container == NULL);
		xsAssert(content->previous == NULL);
		xsAssert(content->next == NULL);
		KprContainerAdd(self, content);
	}
}

void KPR_Handler_Bind(xsMachine *the)
{
	xsOverflow(-4);
	fxPush(xsArg(0));
	fxPushCount(the, 1);
	fxPush(xsThis);
	fxNew(the);
	xsResult = fxPop();
	fxPush(xsResult);
	fxPushCount(the, 1);
	fxPush(xsArg(1));
	fxNew(the);
	xsArg(1) = fxPop();
	xsSet(xsResult, xsID_behavior, xsArg(1));
	xsCall1(xsThis, xsID_put, xsResult);
}

void KPR_Handler_bind(xsMachine *the)
{
	xsOverflow(-4);
	fxPush(xsArg(0));
	fxPushCount(the, 1);
#ifndef XS6
	fxPush(xsGlobal);
#endif
	fxPush(xsThis);
	fxNew(the);
	xsResult = fxPop();
	xsSet(xsResult, xsID_behavior, xsArg(1));
	xsCall1(xsThis, xsID_put, xsResult);
}

void KPR_Behavior_Template(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsVars(1);
	xsResult = xsGet(xsFunction, xsID_prototype);
	if (xsIsInstanceOf(xsThis, xsResult)) {
		xsVar(0) = xsGet(xsThis, xsID("onCreate"));
		if (c > 2)
			xsCallFunction3(xsVar(0), xsThis, xsArg(0), xsArg(1), xsArg(2));
		else if (c > 1)
			xsCallFunction2(xsVar(0), xsThis, xsArg(0), xsArg(1));
		else if (c > 0)
			xsCallFunction1(xsVar(0), xsThis, xsArg(0));
		else
			xsCallFunction0(xsVar(0), xsThis);
	}
	else {
		xsThis = xsNewInstanceOf(xsResult);
		if ((c > 0) && xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
			fxPush(xsThis);
			fxPush(xsArg(0));
			fxCopyObject(the);
		}
	}
	xsResult = xsThis;
}

void KPR_Behavior_template(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsVars(1);
	xsVar(0) = xsNewInstanceOf(xsGet(xsThis, xsID_prototype));
	if ((c > 0) && xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
		fxPush(xsVar(0));
		fxPush(xsArg(0));
		fxCopyObject(the);
	}
	xsResult = xsNewHostConstructor(KPR_Behavior_Template, 3, xsVar(0));
	xsVar(0) = xsNewHostFunction(KPR_Behavior_template, 1);
	xsNewHostProperty(xsResult, xsID_template, xsVar(0), xsDefault, xsDontScript);
}

#ifdef XS6
#define kprPatchConstructor(_ID) \
	xsVar(1) = xsGet(xsGet(xsGlobal, xsID_##_ID), xsID_prototype); \
	xsVar(2) = xsNewHostConstructorObject(KPR_##_ID##Template, 3, xsVar(1), xsID__##_ID); \
	xsNewHostProperty(xsVar(2), xsID_template, xsVar(0), xsDefault, xsDontScript); \
	xsNewHostProperty(xsGlobal, xsID__##_ID, xsVar(2), xsDefault, xsDontScript); \
	xsVar(2) = xsNewHostConstructorObject(KPR_##_ID##Patch, 0, xsVar(1), xsID_##_ID); \
	xsVar(3) = xsNewHostFunctionObject(KPR_##_ID##_template, 1, xsID_template); \
	xsNewHostProperty(xsVar(2), xsID_template, xsVar(3), xsDefault, xsDontScript); \
	xsNewHostProperty(xsGlobal, xsID_##_ID, xsVar(2), xsDefault, xsDontScript)
#else
#define kprPatchConstructor(_ID) \
	xsVar(1) = xsGet(xsGet(xsGlobal, xsID_##_ID), xsID_prototype); \
	xsVar(2) = xsNewHostConstructor(KPR_##_ID##Template, 3, xsVar(1)); \
	xsNewHostProperty(xsVar(2), xsID_template, xsVar(0), xsDefault, xsDontScript); \
	xsNewHostProperty(xsGlobal, xsID__##_ID, xsVar(2), xsDefault, xsDontScript); \
	xsVar(2) = xsNewHostConstructor(KPR_##_ID##Patch, 0, xsVar(1)); \
	xsVar(3) = xsNewHostFunction(KPR_##_ID##_template, 1); \
	xsNewHostProperty(xsVar(2), xsID_template, xsVar(3), xsDefault, xsDontScript); \
	xsNewHostProperty(xsGlobal, xsID_##_ID, xsVar(2), xsDefault, xsDontScript)
#endif

void KPR_patchConstructors(xsMachine *the)
{
	/*
	xsID_Content
	xsID_Container
	xsID_Layer
	xsID_Layout
	xsID_Scroller
	xsID_Column
	xsID_Line
	xsID_Label
	xsID_Text
	xsID_Picture
	xsID_Thumbnail
	xsID_Media
	xsID_Canvas
	xsID_Port
	*/
	xsVars(4);
	xsVar(0) = xsNewHostFunction(KPR_template, 1);
	xsSet(xsGlobal, xsID_template, xsVar(0));
	kprPatchConstructor(Content);
	kprPatchConstructor(Container);
	kprPatchConstructor(Layer);
	kprPatchConstructor(Layout);
	kprPatchConstructor(Scroller);
	kprPatchConstructor(Column);
	kprPatchConstructor(Line);
	kprPatchConstructor(Label);
	kprPatchConstructor(Text);
	kprPatchConstructor(Picture);
	kprPatchConstructor(Thumbnail);
	kprPatchConstructor(Media);
	kprPatchConstructor(Canvas);
	kprPatchConstructor(Port);
	
	xsVar(1) = xsGet(xsGet(xsGlobal, xsID_Container), xsID_prototype);
#ifdef XS6
	xsVar(2) = xsNewHostFunctionObject(KPR_container_recurse, 1, xsID_recurse);
#else
	xsVar(2) = xsNewHostFunction(KPR_container_recurse, 1);
#endif
	xsNewHostProperty(xsVar(1), xsID_recurse, xsVar(2), xsDefault, xsDefault);

	xsVar(1) = xsGet(xsGet(xsGlobal, xsID_Skin), xsID_prototype);
#ifdef XS6
	xsVar(2) = xsNewHostConstructorObject(KPR_SkinPatch, 1, xsVar(1), xsID_Skin);
#else
	xsVar(2) = xsNewHostConstructor(KPR_SkinPatch, 1, xsVar(1));
#endif
	xsNewHostProperty(xsGlobal, xsID_Skin, xsVar(2), xsDefault, xsDontScript);
	
	xsVar(1) = xsGet(xsGet(xsGlobal, xsID_Style), xsID_prototype);
#ifdef XS6
	xsVar(2) = xsNewHostConstructorObject(KPR_StylePatch, 1, xsVar(1), xsID_Style);
#else
	xsVar(2) = xsNewHostConstructor(KPR_StylePatch, 1, xsVar(1));
#endif
	xsNewHostProperty(xsGlobal, xsID_Style, xsVar(2), xsDefault, xsDontScript);

	xsVar(0) = xsGet(xsGlobal, xsID_Handler);
#ifdef XS6
	xsVar(1) = xsNewHostFunctionObject(KPR_Handler_bind, 2, xsID_bind);
#else
	xsVar(1) = xsNewHostFunction(KPR_Handler_bind, 2);
#endif
	xsNewHostProperty(xsVar(0), xsID_bind, xsVar(1), xsDefault, xsDontScript);
#ifdef XS6
	xsVar(1) = xsNewHostFunctionObject(KPR_Handler_Bind, 2, xsID_Bind);
	xsNewHostProperty(xsVar(0), xsID_Bind, xsVar(1), xsDefault, xsDontScript);
#endif
	
	xsVar(0) = xsGet(xsGet(xsGlobal, xsID_Behavior), xsID_prototype);
#ifdef XS6
	xsVar(1) = xsNewHostFunctionObject(KPR_Behavior_template, 1, xsID_template);
#else
	xsVar(1) = xsNewHostFunction(KPR_Behavior_template, 1);
#endif
	xsVar(2) = xsNewHostConstructor(KPR_Behavior_Template, 1, xsVar(0));
	xsNewHostProperty(xsVar(2), xsID_template, xsVar(1), xsDefault, xsDontScript);
	xsNewHostProperty(xsGlobal, xsID_Behavior, xsVar(2), xsDefault, xsDontScript);
}









