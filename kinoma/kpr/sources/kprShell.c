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
#define __FSKECMASCRIPT_PRIV__
#define __FSKEVENT_PRIV__
#define __FSKPORT_PRIV__
#define __FSKWINDOW_PRIV__
#include "Fsk.h"
#include "FskECMAScript.h"
#include "FskEnvironment.h"
#include "FskExtensions.h"
#include "FskHardware.h"
#include "FskMain.h"
#include "FskTextConvert.h"
#ifdef KPR_CONFIG
#include "FskSSL.h"
#endif

#include "kprApplication.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprEffect.h"
#include "kprHandler.h"
#include "kprHTTPClient.h"
#include "kprHTTPServer.h"
#include "kprImage.h"
#include "kprLabel.h"
#include "kprLayer.h"
#include "kprMessage.h"
#include "kprSkin.h"
#include "kprSound.h"
#include "kprStyle.h"
#include "kprStorage.h"
#include "kprText.h"
#include "kprTransition.h"
#include "kprURL.h"
#include "kprUtilities.h"

#include "kprShell.h"

#if TARGET_OS_ANDROID
#include "FskHardware.h"
#elif TARGET_OS_MAC
	#if TARGET_OS_IPHONE
		#include "FskCocoaSupportPhone.h"
	#else
		#include "FskCocoaSupport.h"
	#endif
#endif

#if TARGET_OS_KPL
	#include "KplECMAScript.h"
	#if SUPPORT_LINUX_GTK
		#include "FskGtkWindow.h"
	#endif
#endif

#define KPR_NETWORKINTERFACE 1

#if FSK_EXTENSION_EMBED
extern void FskExtensionsEmbedGrammar(char *vmName, char **xsbName, xsGrammar **grammar);
extern FskErr loadGrammar(const char *xsbPath, xsGrammar *grammar);
#endif

#define kprTouchSampleCount 64
typedef struct {
	SInt32 x;
	SInt32 y;
	double ticks;
} KprTouchSampleRecord, *KprTouchSample;

typedef struct {
	KprContentLinkPart;
	UInt32 id;
	UInt32 index;
	KprTouchSampleRecord samples[kprTouchSampleCount];
} KprTouchLinkRecord, *KprTouchLink;

static void KprShellDispose(void* it);
static void KprShellInvalidated(void* it, FskRectangle area);
static void KprShellReflowing(void* it, UInt32 flags);

static void KprShellClose(KprShell self);
static void KprShellDump(KprShell self);
static void KprShellDumpContent(void* it, SInt32 c);
static void KprShellDumpEffects(KprShell self);
static void KprShellDumpSkins(KprShell self);
static void KprShellDumpStyles(KprShell self);
static void KprShellDumpTextures(KprShell self);
static Boolean KprShellEventHandler(FskEvent event, UInt32 eventCode, FskWindow window, void *it);
static void KprShellGLContextLost(void *refcon);
static void KprShellIdleCheck(KprShell self);
static Boolean KprShellKey(void* it, FskEvent event, Boolean down);
static Boolean KprShellKeyDown(void* it, char* key, UInt32 modifiers, UInt32 repeat, double ticks);
static Boolean KprShellKeyUp(void* it, char* key, UInt32 modifiers, UInt32 repeat, double ticks);
static void KprShellLowMemory(void *refcon);
#if SUPPORT_EXTERNAL_SCREEN
static void KprShellExtScreenChanged(UInt32 status, int identifier, FskDimension size, void *param);
#endif /* SUPPORT_EXTERNAL_SCREEN */
static void KprShellMenuCommand(KprShell self, FskEvent event);
static void KprShellMenuStatus(KprShell self, FskEvent event);
static void KprShellMouse(void* it, FskEvent event, KprBehaviorTouchCallbackProc proc);
static void KprShellMouseDown(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
static void KprShellMouseIdle(void* it);
static void KprShellMouseMoved(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
static void KprShellMouseUp(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
static Boolean KprShellSensor(void* it, FskEvent event);
static void KprTouchSampleAppend(KprTouchLink link, SInt32 x, SInt32 y, double ticks);
static void KPR_shell_sensorAux(xsMachine* the, UInt32 modifiers);

static SInt32 gKeyboardHeight = 0;
#if TARGET_OS_ANDROID || TARGET_OS_IPHONE
static Boolean gKeyboardVisible = false;
#endif
static Boolean gStandAlone = true;

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprShellInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprShell", FskInstrumentationOffset(KprShellRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprShellDispatchRecord = {
	"shell",
	KprContainerActivated,
	KprContainerAdded,
	KprContainerCascade,
	KprShellDispose,
	KprContentDraw,
	KprContainerFitHorizontally,
	KprContainerFitVertically,
	KprContentGetBitmap,
	KprContainerHit,
	KprContentIdle,
	KprShellInvalidated,
	KprContainerLayered,
	KprContextMark,
	KprContainerMeasureHorizontally,
	KprContainerMeasureVertically,
	KprContainerPlace,
	KprContainerPlaced,
	KprContainerPredict,
	KprShellReflowing,
	KprContainerRemoving,
	KprContainerSetWindow,
	KprContainerShowing,
	KprContainerShown,
	KprContainerUpdate
};

FskErr KprShellNew(KprShell* it, FskWindow window, FskRectangle bounds, char* shellPath, char* modulesPath, char* cdata, char* debugFile, int debugLine)
{
	xsAllocation allocation = {
		256 * 1024,
		1 * 1024,
		8 * 1024,
		1 * 1024,
		2048,
		16000,
		1993
	};
	FskErr err = kFskErrNone;
#if FSK_EXTENSION_EMBED
	char* applicationPath = NULL;
	char* libraryName = NULL;
	char* libraryPath = NULL;
#else
	xsGrammar kprVMGrammar;
    xsStringValue code = NULL;
#endif
	xsGrammar* grammar = NULL;
	xsMachine* root = NULL;
	KprShell self = NULL;
	xsMachine* shell = NULL;
	char* preferencePath = NULL;

	if (window) {
		gStandAlone = false;
	}
	else {
		char *screenScale = FskEnvironmentGet("screenScale");
		UInt32 windowStyle = KprEnvironmentGetUInt32("windowStyle", 16);
		if (screenScale && (0 == FskStrCompare(screenScale, "1.5"))) {
			bounds->width = (3 * bounds->width) >> 1;
			bounds->height = (3 * bounds->height) >> 1;
		}
		else if (screenScale && ((0 == FskStrCompare(screenScale, "2")) || (0 == FskStrCompare(screenScale, "2.0")))) {
			bounds->width = bounds->width << 1;
			bounds->height = bounds->height << 1;
		}
		bailIfError(FskWindowNew(&window, bounds->width, bounds->height, windowStyle, NULL, NULL));
		FskWindowSetSizeConstraints(window, 320, 240, 3200, 2400);
		FskWindowGetSize(window, (UInt32*)&bounds->width, (UInt32*)&bounds->height);
	}
	// Get real allocation size for VM (manifest.xml)
	allocation.initialChunkSize     = (xsIntegerValue)KprEnvironmentGetUInt32("initialChunkSize", 256 * 1024);
	allocation.incrementalChunkSize = (xsIntegerValue)KprEnvironmentGetUInt32("incrementalChunkSize", 1 * 1024);

#if FSK_EXTENSION_EMBED
	applicationPath = FskGetApplicationPath();
	bailIfNULL(applicationPath);
	FskExtensionsEmbedGrammar("kpr", &libraryName, &grammar);
	bailIfError(FskMemPtrNewClear(FskStrLen(applicationPath) + FskStrLen(libraryName) + 1, &libraryPath));
	FskStrCopy(libraryPath, applicationPath);
	FskStrCat(libraryPath, libraryName);
	bailIfError(loadGrammar(libraryPath, grammar));
#else
    kprVMGrammar = kprGrammar;
	grammar = &kprVMGrammar;
	bailIfError(FskMemPtrNewFromData(grammar->codeSize, grammar->code, &code));
	grammar->code = code;
#endif
	bailIfError(FskMemPtrNewClear(sizeof(KprShellRecord), it));
	gShell = self = *it;
	FskInstrumentedItemNew(self, NULL, &KprShellInstrumentation);
	root = xsNewMachine(&allocation, grammar, "kpr");
	bailIfNULL(root);
	self->root = root;
#ifdef KPR_CONFIG
	FskSSLInitialize(FskEnvironmentGet("CA_list"));
#endif
	xsShareMachine(root);

	self->shell = self;
	self->focus = (KprContent)self;
	self->window = window;
	self->port = FskWindowGetPort(window);
	self->dispatch = &KprShellDispatchRecord;
	self->flags = kprContainer | kprClip | kprVisible | kprWindowActive;
	self->handler = KprShellEventHandler;
	self->id = FskStrDoCopy("shell");
	bailIfNULL(self->id);

	self->coordinates.horizontal = kprWidth;
	self->coordinates.width = bounds->width;
	self->coordinates.vertical = kprHeight;
	self->coordinates.height = bounds->height;
	self->bounds = *bounds;
	KprShellDefaultStyles(self);
	bailIfError(KprPathToURL(shellPath, &self->url));
	bailIfError(KprModulesBasesSetup(self->url, modulesPath));
	shell = xsAliasMachine(&allocation, root, "shell", self);
	bailIfNULL(shell);
	xsBeginHost(shell);
	xsResult = xsNewHostFunction(KPR_include, 1);
	xsSet(xsResult, xsID("uri"), xsString(self->url));
	xsNewHostProperty(xsGlobal, xsID("include"), xsResult, xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsResult = xsNewHostFunction(KPR_require, 1);
	xsSet(xsResult, xsID("uri"), xsString(self->url));
	xsNewHostProperty(xsGlobal, xsID("require"), xsResult, xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsResult = xsGet(xsGet(xsGlobal, xsID("KPR")), xsID("shell"));
	xsSetHostData(xsResult, self);
	xsNewHostProperty(xsGlobal, xsID("application"), xsNull, xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	xsNewHostProperty(xsGlobal, xsID("shell"), xsResult, xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	self->the = the;
	self->slot = xsResult;
	self->code = the->code;
	KprServicesStart(self);
	bailIfError(FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeApplicationPreference, true, NULL, &preferencePath));
	KprLocalStorageNew(preferencePath, gShell->url);
	KprImageCacheNew(&gThumbnailImageCache, 128, 197, 3, -128);
	KprSoundSetup();
#ifdef KPR_NETWORKINTERFACE
	KprNetworkInterfaceSetup();
#endif
	if (gStandAlone) {
		FskPortSetGraphicsMode(self->port, kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, NULL);
		FskWindowEventSetHandler(self->window, self->handler, self);
	}
#ifdef KPR_CONFIG
	(void)xsCall1(xsGlobal, xsID("include"), xsString(self->url));
#else
	#ifdef mxDebug
		(void)xsCall4(xsGet(xsGlobal, xsID("xs")), xsID("execute"), xsString(cdata), xsUndefined, xsString(debugFile), xsInteger(debugLine));
	#else
		(void)xsCall2(xsGet(xsGlobal, xsID("xs")), xsID("execute"), xsString(cdata), xsUndefined);
	#endif
#endif
	if (gStandAlone) {
		FskWindowSetVisible(window, true);
		KprShellAdjust(self);
	}
	(void)xsCall0(xsResult, xsID("updateMenus"));
	xsEndHost(shell);
	FskNotificationRegister(kFskNotificationLowMemory, KprShellLowMemory, self);
	FskNotificationRegister(kFskNotificationGLContextLost, KprShellGLContextLost, self);
#if SUPPORT_EXTERNAL_SCREEN
	self->extScreenNotifier = FskExtScreenAddNotifier(KprShellExtScreenChanged, self, "KprShellExtScreenChanged");
#endif	/* SUPPORT_EXTERNAL_SCREEN */
bail:
	FskMemPtrDispose(preferencePath);
#if FSK_EXTENSION_EMBED
	if (grammar) {
		FskMemPtrDispose(grammar->symbols);
		FskMemPtrDispose(grammar->code);
	}
	FskMemPtrDispose(libraryPath);
	FskMemPtrDispose(applicationPath);
#else
	FskMemPtrDispose(code);
#endif
	return err;
}

void KprShellAdjust(KprShell self)
{
	KprContentLink link;
	if (!(self->flags & kprAdjusting)) {
		Boolean all = false;
		self->flags |= kprAdjusting;
		if (gStandAlone) {
			if (self->flags & (kprWidthChanged | kprHeightChanged)) {
				UInt32 width, height;
				FskWindowGetSize(self->window, &width, &height);
				self->coordinates.width = width;
				self->coordinates.height = height - gKeyboardHeight;
				self->flags &= ~(kprWidthChanged | kprHeightChanged);
#if TARGET_OS_ANDROID || TARGET_OS_IPHONE
				kprDelegateAdapt(gShell->applicationChain.first->content);
#endif
				all = true;
			}
		}
		if (self->flags & kprAssetsChanged) {
			self->flags &= ~kprAssetsChanged;
			KprShellUpdateStyles(self);
			(*self->dispatch->cascade)(self, self->style);
			all = true;
		}
		if (all) {
			FskRectangleRecord area;
			FskRectangleSet(&area, 0, 0, self->bounds.width, self->bounds.height);
			(*self->dispatch->invalidated)(self, &area);
			KprContainerMeasureHorizontally(self);
			KprContainerFitHorizontally(self);
			KprContainerMeasureVertically(self);
			KprContainerFitVertically(self);
		}
		while (self->flags & (kprContentsChanged | kprContentsPlaced)) {
			while (self->flags & kprContentsChanged) {
				if (self->flags & kprContentsHorizontallyChanged)
					KprContainerPlaceHorizontally(self);
				if (self->flags & kprContentsVerticallyChanged)
					KprContainerPlaceVertically(self);
			}
			link = KprContentChainGetFirst(&self->placeChain);
			while (link) {
				KprContent content = link->content;
				KprContainer container = content->container;
				while (container) {
					if (container->flags & kprPlaced)
						break;
					container = container->container;
				}
				if (container) {
					content->flags |= kprPlaced;
					container = content->container;
					while (container) {
						if (container->flags & kprContentsPlaced)
							break;
						container->flags |= kprContentsPlaced;
						container = container->container;
					}
				}
				link = KprContentChainGetNext(&self->placeChain);
			}

			if (self->flags & kprContentsPlaced)
				KprContainerPlace(self);
		}
		KprShellIdleCheck(self);
		self->flags &= ~kprAdjusting;
	}
}

void KprShellCaptureTouch(void* it, KprContent content, UInt32 id, SInt32 x, SInt32 y, double ticks)
{
	KprShell self = it;
	KprTouchLink *linkAddress = (KprTouchLink*)&self->touchChain.first, link;
	while ((link = *linkAddress)) {
		if ((link->id == id) && (link->content != content)) {
			kprDelegateTouchCancelled(link->content, id, x, y, ticks);
			if ((KprTouchLink)self->touchChain.next == link)
				self->touchChain.next = link->next;
			*linkAddress = (KprTouchLink)link->next;
			FskMemPtrDispose(link);
		}
		else
			linkAddress = (KprTouchLink*)&link->next;
	}
}

FskThread KprShellGetThread(KprShell self)
{
	FskThread thread = NULL;
	if (self)
		thread = self->window->thread;
	return thread;
}

void KprShellQuit(KprShell self)
{
	if (gStandAlone)
		FskWindowEventSetHandler(self->window, NULL, NULL);
	xsDeleteMachine(self->the);
	KprContentClose((KprContent)self);
	if (gStandAlone)
		FskMainDoQuit(kFskErrNone);
}

void KprShellReflow(KprShell self, UInt32 flags)
{
	self->flags |= flags | kprContentsPlaced;
	KprShellIdleCheck(self);
}

void KprShellSetCaret(KprShell self, void* it, FskRectangle bounds)
{
	if (bounds) {
		KprShellStartPlacing(self, it);
		self->caretBounds = *bounds;
		self->caretFlags = 3;
		self->caretTicks = KprShellTicks(self) + 500;
	}
	else {
		KprShellStopPlacing(self, it);
		FskRectangleSetEmpty(&self->caretBounds);
		self->caretFlags = 0;
	}
	KprShellIdleCheck(self);
}

void KprShellSetFocus(KprShell self, KprContent content)
{
	if (!content)
		content = (KprContent)self;
	if (self->focus != content) {
		if (self->focus) {
			kprDelegateUnfocused(self->focus);
			(*self->focus->dispatch->activated)(self->focus, false);
		}
		self->focus = content;
		if (self->focus) {
			(*self->focus->dispatch->activated)(self->focus, true);
			kprDelegateFocused(self->focus);
		}
	}
}

void KprShellStartIdling(KprShell self, void* it)
{
	KprIdleLink link = it;
	KprIdleLink chain = self->idleChain;
	while (chain) {
		if (chain == link)
			break;
		chain = chain->idleLink;
	}
	if (!chain) {
		link->idleLink = self->idleChain;
		self->idleChain = link;
	}
	link->ticks = KprShellTicks(self);
	KprShellIdleCheck(self);
}

void KprShellStopIdling(KprShell self, void* it)
{
	KprIdleLink link = it;
	KprIdleLink *chain = &self->idleChain;
	while (*chain) {
		if (*chain == link) {
			*chain = link->idleLink;
			if (self->idleLoop) {
				if (self->idleLoop == link)
					self->idleLoop = link->idleLink;
			}
			else
				KprShellIdleCheck(self);
			break;
		}
		chain = &((*chain)->idleLink);
	}
}

void KprShellStartPlacing(KprShell self, void* it)
{
	KprContentLink link = NULL;
	if (!KprContentChainContains(&self->placeChain, it))
		KprContentChainAppend(&self->placeChain, it, sizeof(KprContentLinkRecord), (KprContentLink*)&link);
}

void KprShellStopPlacing(KprShell self, void* it)
{
	KprContentChainRemove(&self->placeChain, it);
}

double KprShellTicks(void* it UNUSED)
{
	FskTimeRecord now;
	FskTimeGetNow(&now);
	return (((double)now.seconds) * 1000.0) + (((double)now.useconds) / 1000.0);
}

/* DISPATCH */

void KprShellDispose(void* it)
{
	KprShell self = it;
#if SUPPORT_EXTERNAL_SCREEN
	FskExtScreenRemoveNotifier(self->extScreenNotifier);
#endif	/* SUPPORT_EXTERNAL_SCREEN */
	FskNotificationUnregister(kFskNotificationGLContextLost, KprShellGLContextLost, self);
	FskNotificationUnregister(kFskNotificationLowMemory, KprShellLowMemory, self);
//	FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KprSSDPForgetServer, FskStrDoCopy(self->id), NULL, NULL, NULL);
	KprHTTPServerDispose(KprHTTPServerGet(self->id));

	KprServicesStop(self);

	kprDelegateDispose(self);
	KprAssetUnbind(self->style);
	KprAssetUnbind(self->skin);
	KprContextPurge(self, true);
	xsDeleteMachine(self->root);
	KprContextDisposeHandlers(self);
	FskMemPtrDispose(self->url);
	FskMemPtrDispose(self->id);
	FskInstrumentedItemDispose(self);
	//FskMemPtrDispose(self);

	KprImageCacheDispose(gThumbnailImageCache);
	gThumbnailImageCache = NULL;
	KprImageCacheDispose(gPictureImageCache);
	gPictureImageCache = NULL;
	KprLocalStorageDispose();
	KprModulesBasesCleanup();

	KprSoundCleanup();
#ifdef KPR_NETWORKINTERFACE
	KprNetworkInterfaceCleanup();
#endif
	FskMemPtrDispose(self);

	gShell = NULL;
}

void KprShellInvalidated(void* it, FskRectangle area)
{
	KprShell self = it;
	FskPort port = self->port;
	SInt32 x, y;
	FskRectangleOffset(area, self->bounds.x, self->bounds.y);
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedWindowInvalidated, area);
	FskPortGetOrigin(port, &x, &y);
	FskPortSetOrigin(port, 0, 0);
	FskPortInvalidateRectangle(port, area);
	FskPortSetOrigin(port, x, y);
}

void KprShellReflowing(void* it, UInt32 flags)
{
	KprShell self = it;
	if (flags & kprHorizontallyChanged)
		self->flags |= kprContentsHorizontallyChanged;
	else if (flags & kprContentsHorizontallyChanged)
		self->flags |= kprContentsHorizontallyChanged;
	if (flags & kprVerticallyChanged)
		self->flags |= kprContentsVerticallyChanged;
	else if (flags & kprContentsVerticallyChanged)
		self->flags |= kprContentsVerticallyChanged;
	self->flags |= kprContentsPlaced;
	KprShellIdleCheck(it);
}

/* IMPLEMENTATION */

void KprShellClose(KprShell self)
{
	KprScriptBehavior behavior = (KprScriptBehavior)self->behavior;
	if (gStandAlone) {
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
		Boolean flag;
		FskCocoaWindowIsFullScreen(self->window, &flag);
		if (flag) {
			self->flags |= kprClosing;
			FskCocoaWindowToggleFullScreen(self->window);
			return;
		}
#endif
		FskWindowSetVisible(self->window, false);
	}
	if (behavior) {
		xsBeginHostSandboxCode(self->the, self->code);
		{
			if (xsFindResult(behavior->slot, xsID("onQuit"))) {
				(void)xsCallFunction1(xsResult, behavior->slot, self->slot);
			}
		}
		xsEndHostSandboxCode();
	}
	self->flags |= kprQuitting;
	KprShellIdleCheck(self);
}

void KprShellDump(KprShell self)
{
	fprintf(stderr, "CONTENTS\n");
	KprShellDumpContent(self, 1);
	KprShellDumpEffects(self);
	KprShellDumpTextures(self);
	KprShellDumpSkins(self);
	KprShellDumpStyles(self);
}

void KprShellDumpContent(void* it, SInt32 c)
{
	KprContainer self = it;
	SInt32 i;
	for (i = 0; i < c; i++)
		fprintf(stderr, "\t");
		fprintf(stderr, "%s %p coordinates %ld %ld %ld %ld %ld %ld bounds %ld %ld %ld %ld skin %p style %p\n",
		self->dispatch->type, self,
		self->coordinates.left,
		self->coordinates.width,
		self->coordinates.right,
		self->coordinates.top,
		self->coordinates.height,
		self->coordinates.bottom,
		self->bounds.x,
		self->bounds.y,
		self->bounds.width,
		self->bounds.height,
		self->skin,
		self->style);
	if (self->flags & kprContainer) {
		KprContent content = self->first;
		c++;
		while (content) {
			KprShellDumpContent(content, c);
			content = content->next;
		}
	}
}

void KprShellDumpEffects(KprShell self)
{
	KprEffect effect = (KprEffect)self->firstEffect;
	fprintf(stderr, "EFFECTS\n");
	while (effect) {
		fprintf(stderr, "\t%p (%p %ld)\n",
			effect, effect->the, effect->usage);
		effect = (KprEffect)effect->next;
	}
}

void KprShellDumpSkins(KprShell self)
{
	KprSkin skin = (KprSkin)self->firstSkin;
	fprintf(stderr, "SKINS\n");
	while (skin) {
		if (skin->flags)
			fprintf(stderr, "\t%p (%p %ld) texture %p\n",
				skin, skin->the, skin->usage, skin->data.pattern.texture);
		else
			fprintf(stderr, "\t%p (%p %ld)\n",
				skin, skin->the, skin->usage);
		skin = (KprSkin)skin->next;
	}
}

void KprShellDumpStyles(KprShell self)
{
	KprStyle style = (KprStyle)self->firstStyle;
	fprintf(stderr, "STYLES\n");
	while (style) {
		int i;
		fprintf(stderr, "\t%p (%p %ld) father %p mother %p %8.8X ",
			style, style->the, style->usage, style->father, style->mother, (unsigned int)style->flags);
		for (i = 0; i < 4; i++) {
			fprintf(stderr, "#%2.2X%2.2X%2.2X ", style->colors[0].r, style->colors[0].g, style->colors[0].b);
		}
		fprintf(stderr, "%d %d %ld %ld %ld %ld %ld %ld %ld %s %ld %ld\n",
			style->horizontalAlignment, style->verticalAlignment, style->indentation, style->lineCount, style->lineHeight, style->margins.left, style->margins.top, style->margins.right, style->margins.bottom, style->textFont ? style->textFont : "?", style->textSize, style->textStyle);
		style = (KprStyle)style->next;
	}
}

void KprShellDumpTextures(KprShell self)
{
	KprTexture texture = (KprTexture)self->firstTexture;
	fprintf(stderr, "TEXTURES\n");
	while (texture) {
		fprintf(stderr, "\t%p (%p %ld) bitmap %p %s\n",
			texture, texture->the, texture->usage, texture->bitmap, texture->url ? texture->url : "");
		texture = (KprTexture)texture->next;
	}
}

Boolean KprShellEventHandler(FskEvent event, UInt32 eventCode, FskWindow window UNUSED, void *it)
{
	Boolean result = false;
	KprShell self = it;
    FskTimeRecord time;
	switch (eventCode) {
		case kFskEventWindowClose:
			KprShellClose(self);
			break;
		case kFskEventWindowActivated:
			if (self->flags & kprWindowActive) break;
			self->flags |= kprWindowActive;
#ifdef KPR_NETWORKINTERFACE
			KprNetworkInterfaceActivate(1);
#endif
			if (gShell->applicationChain.first)
				kprDelegateActivated(gShell->applicationChain.first->content, 1);
			(*self->dispatch->activated)(self, 1);
			break;
		case kFskEventWindowDeactivated:
			if (!(self->flags & kprWindowActive)) break;
			self->flags &= ~kprWindowActive;
			(*self->dispatch->activated)(self, 0);
			if (gShell->applicationChain.first)
				kprDelegateActivated(gShell->applicationChain.first->content, 0);
#ifdef KPR_NETWORKINTERFACE
			KprNetworkInterfaceActivate(0);
#endif
			//FskNotificationPost(kFskNotificationGLContextLost);
			break;

		case kFskEventWindowUpdate: {
			FskRectangleRecord area;
			FskEventParameterGet(event, kFskEventParameterUpdateRectangle, &area);
            FskEventParameterGet(event, kFskEventParameterTime, &time);
			(*self->dispatch->update)(self, self->port, &area);
		/*{
			FskColorRGBARecord color = {255, 0, 0, 255};
			FskPortSetPenColor(self->port, &color);
			FskPortRectangleFrame(self->port, &area);
		}*/
			break;
		}
		case kFskEventWindowAfterUpdate:
			break;
		case kFskEventWindowBeforeUpdate:
            FskEventParameterGet(event, kFskEventParameterTime, &time);
			KprShellIdleLoop(self, &time);
			break;

		case kFskEventWindowBitmapChanged:
			break;

		case kFskEventWindowBeforeResize:
#if (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
			FskWindowGtkSizeChanged(window);
#endif
			break;
		case kFskEventWindowResize:
#if TARGET_OS_ANDROID
            if (gKeyboardVisible == gAndroidCallbacks->isIMEEnabledCB())
#elif TARGET_OS_IPHONE
            if (!gKeyboardVisible)
#endif
			{
				UInt32 width, height;
				FskWindowGetSize(window, &width, &height);
				if ((self->coordinates.width != (SInt32)width) || (self->coordinates.height != (SInt32)height)) {
					KprShellReflow(self, kprWidthChanged | kprHeightChanged);
				}
			}
			break;
		case kFskEventWindowAfterResize:
#if TARGET_OS_ANDROID
            if (gKeyboardVisible == gAndroidCallbacks->isIMEEnabledCB())
				gAndroidCallbacks->afterWindowResizeCB();
			else
				gAndroidCallbacks->IMEEnableCB(gKeyboardVisible);
#endif
			result = true;
			break;
		case kFskEventMouseDown:
			KprShellMouse(it, event, KprShellMouseDown);
			break;
		case kFskEventRightMouseDown:
			break;
		case kFskEventMouseDoubleClick:
			KprShellMouse(it, event, KprShellMouseDown);
			break;
		case kFskEventMouseUp:
			KprShellMouse(it, event, KprShellMouseUp);
			break;
		case kFskEventRightMouseUp:
			break;

		case kFskEventMouseLeave:
			break;
		case kFskEventMouseMoved:
			KprShellMouse(it, event, KprShellMouseMoved);
			break;
		case kFskEventMouseStillDown:
			break;
		case kFskEventMouseWheel:
			break;

		case kFskEventKeyDown:
			result = KprShellKey(it, event, true);
			break;
		case kFskEventKeyUp:
			result = KprShellKey(it, event, false);
			break;

		case kFskEventApplication:
			break;
#if SUPPORT_REMOTE_NOTIFICATION
		case kFskEventSystemRemoteNotificationRegistered: {
			char buff[256];
			char *token;
			UInt32 size;
			FskEventParameterGetSize(event, kFskEventParameterStringValue, &size);
			if (size <= sizeof(buff))
			{
				char *osType;
				if (size > 1)
				{
					FskEventParameterGet(event, kFskEventParameterStringValue, buff);
					token = buff;
				}
				else
				{
					token = NULL;
				}
#if TARGET_OS_IPHONE
				osType = "iOS";
#elif TARGET_OS_ANDROID
				osType = "android";
#else
				osType = "unknown";
#endif
				if (gShell->applicationChain.first)
					kprDelegateRemoteNotificationRegistered(gShell->applicationChain.first->content, token, osType);
			}
			break;
		}
		case kFskEventSystemRemoteNotification: {
			char buff[256];
			UInt32 size;
			FskEventParameterGetSize(event, kFskEventParameterStringValue, &size);
			if (size <= sizeof(buff))
			{
				FskEventParameterGet(event, kFskEventParameterStringValue, buff);
				if (gShell->applicationChain.first)
					kprDelegateRemoteNotified(gShell->applicationChain.first->content, buff);
			}
			break;
		}
#endif	/* SUPPORT_REMOTE_NOTIFICATION */
		case kFskEventClipboardChanged:
			break;
		case kFskEventMenuCommand:
			KprShellMenuCommand(it, event);
			break;
		case kFskEventMenuStatus:
			KprShellMenuStatus(it, event);
			break;
		case kFskEventWindowOpenFiles:
			break;
		case kFskEventWindowDragEnter:
			break;
		case kFskEventWindowDragLeave:
			break;
		case kFskEventButton:
			result = KprShellSensor(it, event);
			break;
		case kFskEventWindowTransition:
			break;
	}
	if (self->flags & kprQuitting)
		KprShellQuit(self);
	return result;
}

static Boolean KprShellGLContextLostAux(KprContent container)
{
	Boolean result = false;
	if (container->flags & kprContainer) {
		KprContent content = ((KprContainer)container)->first;
		while (content) {
			result |= KprShellGLContextLostAux(content);
			content = content->next;
		}
		if (result)
			container->flags |= kprContentsPlaced;
	}
	if (container->flags & kprLayer)
		result |= KprLayerGLContextLost((KprLayer)container);
	return result;
}

void KprShellGLContextLost(void *refcon)
{
	KprShell self = refcon;
	KprContentLink link = self->applicationChain.first;
	while (link) {
		KprContextGLContextLost(link->content);
		link = link->next;
	}
	KprContextGLContextLost(self);
	if (KprShellGLContextLostAux((KprContent)self))
		KprShellIdleCheck(self);
}

void KprShellIdleCheck(KprShell self)
{
	Boolean idle = false;
	if (self->idleChain)
		idle = true;
	if (self->caretFlags & 1)
		idle = true;
	if (self->touchChain.first)
		idle = true;
	if (self->flags & (kprContentsChanged | kprContentsPlaced | kprQuitting))
		idle = true;
	if (gStandAlone)
		FskWindowSetUpdates(self->window, &idle, NULL, NULL);
}

void KprShellIdleLoop(void* it, const FskTime atTime)
{
	KprShell self = it;
	KprIdleLink link;
	double ticks = (NULL == atTime) ? KprShellTicks(self) :  (((double)atTime->seconds) * 1000.0) + (((double)atTime->useconds) / 1000.0);
	link = self->idleChain;
	while (link) {
		double interval = ticks - link->ticks;
		self->idleLoop = link->idleLink;
		if (link->interval <= interval) {
			link->ticks = ticks;
			link->dispatch->idle(link, interval);
		}
		link = self->idleLoop;
	}
	if (self->caretFlags & 1) {
		if (self->caretTicks <= ticks) {
			self->caretTicks = ticks + 500;
			self->caretFlags ^= 2;
			if (self->caretFlags)
				KprShellInvalidated(self, &self->caretBounds);
		}
	}
	if (self->touchChain.first) {
		KprShellMouseIdle(self);
	}
	if (self->flags & kprCollectGarbage) {
		self->flags &= ~kprCollectGarbage;
		FskInstrumentedItemSendMessageNormal(self, kprInstrumentedContentBeginCollect, self);
		fxCollectGarbage(self->the);
		FskInstrumentedItemSendMessageNormal(self, kprInstrumentedContentEndCollect, self);
		KprContextPurge(self, false);
	}
	KprShellAdjust(self);
	KprShellIdleCheck(self);
}

Boolean KprShellKey(void* it, FskEvent event, Boolean down)
{
	KprShell self = it;
	UInt32 functionKey = 0;
	unsigned char* utf8;
	Boolean functionKeyFlag = false;
	Boolean cursorKeyFlag = false;
	UInt32 size;
	char shortKey[256];
	char* longKey = NULL;
	char* key = NULL;
	UInt32 modifiers;
	FskTimeRecord time;
	double ticks;
	static UInt32 repeat = 0;
	Boolean result;

#if TARGET_OS_ANDROID || TARGET_OS_IPHONE
	if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterCommand, &functionKey)) {
		if (functionKey == 1025) {
			if (kFskErrNone == FskEventParameterGetSize(event, kFskEventParameterFileList, &size)) {
				if (size < sizeof(shortKey)) {
					if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterFileList, shortKey)) {
						SInt32 offset = FskStrToNum(shortKey);
						SInt32 length = FskStrToNum(shortKey + FskStrLen(shortKey) + 1);
						if (!FskStrCompare(self->focus->dispatch->type, "label")) {
							KprLabel focus = (KprLabel)self->focus;
							focus->from = offset;
							focus->to = offset + length;
						}
						else if (!FskStrCompare(self->focus->dispatch->type, "text")) {
							KprText focus = (KprText)self->focus;
							focus->from = offset;
							focus->to = offset + length;
						}
					}
				}
			}
		}
#if TARGET_OS_IPHONE
        else if (functionKey == 1024) {
            // used later
        }
		else {
            if (functionKey == 1026) {
                if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterIntegerValue, &size)) {
                }
            }
            else if (functionKey == 1027) {
                if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterIntegerValue, &size)) {
                	gKeyboardHeight = size;
					KprShellReflow(self, kprHeightChanged);
               }
            }
            else if (functionKey == 1028) {
                if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterIntegerValue, &size)) {
                	if (!gKeyboardVisible) {
						gKeyboardHeight = 0;
						KprShellReflow(self, kprHeightChanged);
                	}
                }
            }
            else if (functionKey == 1029) {
                if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterIntegerValue, &size)) {
                }
            }
            return true;
		}
#endif
	}
#endif
	if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterFunctionKey, &functionKey)) {
		if (functionKey < 0x0000FFFF)
			functionKey |= 0xF0000;
		utf8 = (unsigned char*)shortKey;
		utf8[0] = (unsigned char)(0xF0 | (functionKey >> 18));
		utf8[1] = (unsigned char)(0x80 | ((functionKey >> 12) & 0x3F));
		utf8[2] = (unsigned char)(0x80 | ((functionKey >> 6) & 0x3F));
		utf8[3] = (unsigned char)(0x80 | (functionKey & 0x3F));
		utf8[4] = 0;
		functionKeyFlag = true;
		key = shortKey;
	}
	else if (kFskErrNone == FskEventParameterGetSize(event, kFskEventParameterKeyUTF8, &size)) {
		if (size < sizeof(shortKey)) {
			if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterKeyUTF8, shortKey)) {
				if ((28 <= shortKey[0]) && (shortKey[0] <= 31) && (shortKey[1] == 0))
					cursorKeyFlag = true;
				key = shortKey;
			}
		}
		else if (kFskErrNone == FskMemPtrNew(size, &longKey)) {
			if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterKeyUTF8, longKey))
				key = longKey;
		}
	}
	if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterModifiers, &modifiers))
		self->modifiers = modifiers;
	else
		self->modifiers = 0;
	if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterTime, &time))
		ticks = ((xsNumberValue)time.seconds) * 1000.0 + (((xsNumberValue)time.useconds) / 1000);
	else
		ticks = KprShellTicks(it);
	if (down) {
		if ((functionKeyFlag || cursorKeyFlag))
			repeat++;
		else
			repeat = 1;
		result = KprShellKeyDown(it, key, modifiers, repeat, ticks);
	}
	else {
		result = KprShellKeyUp(it, key, modifiers, repeat, ticks);
		repeat = 0;
	}
	FskMemPtrDispose(longKey);
#if TARGET_OS_ANDROID || TARGET_OS_IPHONE
	if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterCommand, &functionKey)) {
		if (functionKey == 1024) {
			SInt32 length = fxUnicodeLength(key);
			if (!FskStrCompare(self->focus->dispatch->type, "label")) {
				KprLabel focus = (KprLabel)self->focus;
				KprLabelSelect(focus, focus->from - length, length);
			}
			else if (!FskStrCompare(self->focus->dispatch->type, "text")) {
				KprText focus = (KprText)self->focus;
				KprTextSelect(focus, focus->from - length, length);
			}
		}
	}
#endif
	return result;
}

void KprShellKeyActivate(void* it, Boolean activateIt)
{
#if TARGET_OS_ANDROID
	KprShell self = it;
	enum {
		androidAutoCorrectTextType = 0,
		androidAutoCorrectTextMultiLineType = 1,
		androidNumericInputType = 2,
		androidPasswordInputType = 3,
		androidEmailInputType = 4,
		androidURIInputType = 5,
		androidPhoneInputType = 6
	};
	if (!FskStrCompare(self->focus->dispatch->type, "label")) {
		KprLabel focus = (KprLabel)self->focus;
		int mode = androidAutoCorrectTextType;
		if (focus->flags & kprTextHidden)
			mode = androidPasswordInputType;
		gAndroidCallbacks->tweakKbdCB(focus->text, FskStrLen(focus->text), mode);
	}
	else if (!FskStrCompare(self->focus->dispatch->type, "text")) {
		KprText focus = (KprText)self->focus;
		int mode = androidAutoCorrectTextMultiLineType;
		char* text;
		FskGrowableStorageGetPointerToItem(focus->textBuffer, 0, (void **)&text);
		gAndroidCallbacks->tweakKbdCB(text, focus->textOffset, mode);
	}
#elif TARGET_OS_IPHONE
	KprShell self = it;
	if (!FskStrCompare(self->focus->dispatch->type, "label")) {
		KprLabel focus = (KprLabel)self->focus;
		int mode = cocoaTextInputTypeDefault;
		if (focus->flags & kprTextHidden)
			mode = cocoaTextInputTypePassword;
   	 	FskCocoaWindowInputTextActivate(self->window, focus->the, focus->slot, activateIt, mode);
	}
	else if (!FskStrCompare(self->focus->dispatch->type, "text")) {
		KprText focus = (KprText)self->focus;
		int mode = cocoaTextInputTypeMultiLine;
   		FskCocoaWindowInputTextActivate(self->window, focus->the, focus->slot, activateIt, mode);
	}
#endif
}

Boolean KprShellKeyDown(void* it, char* key, UInt32 modifiers, UInt32 repeat, double ticks)
{
	KprShell self = it;
	KprContent content = self->focus;
	while (content) {
		if ((content->flags & kprLayer) && (content->flags & kprBlocking))
			return false;
		content = (KprContent)content->container;
	}
	content = self->focus;
	while (content) {
		if (kprDelegateKeyDown(content, key, modifiers, repeat, ticks))
			return true;
		content = (KprContent)content->container;
	}
	return false;
}

void KprShellKeySelect(void* it)
{
#if TARGET_OS_ANDROID || TARGET_OS_IPHONE
	KprShell self = it;
#endif
#if TARGET_OS_ANDROID
	if (!FskStrCompare(self->focus->dispatch->type, "label")) {
		KprLabel focus = (KprLabel)self->focus;
		gAndroidCallbacks->tweakKbdSelectionCB(focus->from, focus->to);
	}
	else if (!FskStrCompare(self->focus->dispatch->type, "text")) {
		KprText focus = (KprText)self->focus;
		gAndroidCallbacks->tweakKbdSelectionCB(focus->from, focus->to);
	}
#elif TARGET_OS_IPHONE
	if (!FskStrCompare(self->focus->dispatch->type, "label")) {
		KprLabel focus = (KprLabel)self->focus;
   		FskCocoaWindowInputTextSetSelection(self->window, focus->text, focus->length, focus->from, focus->to);
	}
	else if (!FskStrCompare(self->focus->dispatch->type, "text")) {
		KprText focus = (KprText)self->focus;
		char* text;
		FskGrowableStorageGetPointerToItem(focus->textBuffer, 0, (void **)&text);
   		FskCocoaWindowInputTextSetSelection(self->window, text, focus->length, focus->from, focus->to);
	}
#endif
}

Boolean KprShellKeyUp(void* it, char* key, UInt32 modifiers, UInt32 repeat, double ticks)
{
	KprShell self = it;
	KprContent content = self->focus;
	while (content) {
		if ((content->flags & kprLayer) && (content->flags & kprBlocking))
			return false;
		content = (KprContent)content->container;
	}
	content = self->focus;
	while (content) {
		if (kprDelegateKeyUp(content, key, modifiers, repeat, ticks))
			return true;
		content = (KprContent)content->container;
	}
	return false;
}

void KprShellLowMemory(void *refcon)
{
	KprShell self = refcon;
	KprContentLink link = self->applicationChain.first;
	while (link) {
		KprContextPurge(link->content, false);
		link = link->next;
	}
	KprContextPurge(self, false);
	KprImageCachePurge(gPictureImageCache);
	KprImageCachePurge(gThumbnailImageCache);
}

#if SUPPORT_EXTERNAL_SCREEN
void KprShellExtScreenChanged(UInt32 status, int identifier, FskDimension size, void *param)
{
	KprShell self = param;

	switch (status) {
		case kFskExtScreenStatusNew: {
			// should support several screens...
			if (self->extScreenId != 0)
				break;
			UInt32 windowStyle = KprEnvironmentGetUInt32("windowStyle", 16);
			FskWindowNew(&self->extWindow, size->width, size->height, windowStyle, NULL, NULL);
//			FskWindowEventSetHandler(self->extWindow, KprShellExtEventHandler, self);
//			kprDelegateExternalScreenChanged(self->applicationChain.first->content, status, identifier, size->width, size->height);
			break;
		}

		case kFskExtScreenStatusRemoved: {
			if (identifier != self->extScreenId)
				break;
//			kprDelegateExternalScreenChanged(self->applicationChain.first->content, status, identifier, 0, 0);
			FskWindowDispose(self->extWindow);
			self->extWindow = NULL;
			self->extScreenId = 0;
			break;
		}

		case kFskExtScreenStatusChanged: {
			// Nothing to do? (FskWindow{Cocoa,Android}SizeChanged will be called).
//			kprDelegateExternalScreenChanged(self->applicationChain.first->content, status, identifier, size->width, size->height);
			break;
		}

		default:
			break;
	}
}
#endif	/* SUPPORT_EXTERNAL_SCREEN */

void KprShellMenuCommand(KprShell self, FskEvent event)
{
#if TARGET_OS_MAC || TARGET_OS_WIN32 || (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
	SInt32 id = 0;
	KprScriptBehavior behavior;
	if (kFskErrNone != FskEventParameterGet(event, kFskEventParameterCommand, &id)) return;
	if (id <= 0) {
		if ((id == 0) || ((id == -1) && (self->flags & kprClosing)))
			KprShellClose(self);
		return;
	}
	behavior = (KprScriptBehavior)self->behavior;
	if (!behavior) return;
	xsBeginHostSandboxCode(self->the, self->code);
	{
		xsVars(3);
		if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterCommand, &id)) {
			xsResult = xsGet(self->slot, xsID("menus"));
			if (xsTest(xsResult)) {
				xsIntegerValue c, i, d, j;
				c = xsToInteger(xsGet(xsResult, xsID("length")));
				i = (id & 0x0000FF00) >> 8;
				if ((0 <= i) && (i < c)) {
					xsVar(0) = xsGetAt(xsResult, xsInteger(i));
					xsVar(1) = xsGet(xsVar(0), xsID("items"));
					d = xsToInteger(xsGet(xsVar(1), xsID("length")));
					j = (id & 0x000000FF) - 1;
					if ((0 <= j) && (j < d)) {
						xsVar(2) = xsGetAt(xsVar(1), xsInteger(j));
						if (xsTest(xsVar(2))) {
							xsIndex doID = xsToInteger(xsGet(xsVar(2), xsID("doID")));
							if (xsFindResult(behavior->slot, doID)) {
								(void)xsCallFunction2(xsResult, behavior->slot, self->slot, xsVar(2));
							}
						}
					}
				}
			}
		}
	}
	xsEndHostSandboxCode();
#endif
}

void KprShellMenuStatus(KprShell self, FskEvent event)
{
#if (TARGET_OS_MAC && !TARGET_OS_IPHONE) || TARGET_OS_WIN32 || (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
	KprScriptBehavior behavior = (KprScriptBehavior)self->behavior;
	if (!behavior) return;
	xsBeginHostSandboxCode(self->the, self->code);
	{
		UInt32 id = 0;
		xsVars(3);
		if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterCommand, &id)) {
			xsResult = xsGet(self->slot, xsID("menus"));
			if (xsTest(xsResult)) {
				xsIntegerValue c, i, d, j;
				c = xsToInteger(xsGet(xsResult, xsID("length")));
				i = (id & 0x0000FF00) >> 8;
				if ((0 <= i) && (i < c)) {
					xsResult = xsGetAt(xsResult, xsInteger(i));
					xsResult = xsGet(xsResult, xsID("items"));
					d = xsToInteger(xsGet(xsResult, xsID("length")));
					j = (id & 0x000000FF) - 1;
					if ((0 <= j) && (j < d)) {
						xsVar(0) = xsGetAt(xsResult, xsInteger(j));
						if (xsTest(xsVar(0))) {
                            Boolean enabled = false;
                            Boolean checked = false;
							xsIndex canID = xsToInteger(xsGet(xsVar(0), xsID("canID")));
							if (xsFindResult(behavior->slot, canID)) {
								xsResult = xsCallFunction2(xsResult, behavior->slot, self->slot, xsVar(0));
								enabled = xsTest(xsResult);
                            }
							if (xsFindResult(xsVar(0), xsID("check")))
								checked = xsTest(xsResult);
                            {
#if TARGET_OS_MAC
                                CocoaMenuItemSetEnable(i, id, enabled);
                                CocoaMenuItemSetCheck(i, id, checked);
#elif TARGET_OS_WIN32
                                HMENU menu = GetSubMenu(GetMenu(self->window->hwnd), i);
                                if (enabled)
                                	EnableMenuItem(menu, j, MF_BYPOSITION | MF_ENABLED);
                                else
                                    EnableMenuItem(menu, j, MF_BYPOSITION | MF_GRAYED);
                                if (checked)
                                	CheckMenuItem(menu, j, MF_BYPOSITION | MF_CHECKED);
                                else
                                    CheckMenuItem(menu, j, MF_BYPOSITION | MF_UNCHECKED);
#elif (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
								FskGtkWindow win = self->window->gtkWin;
								FskGtkWindowSetMenuItemStatus(win, id, enabled, checked);
#endif
							}
						}
					}
				}
			}
		}
	}
	xsEndHostSandboxCode();
#endif
}

void KprShellMouse(void* it, FskEvent event, KprBehaviorTouchCallbackProc proc)
{
	KprShell self = it;
	UInt32 c, i;
	FskPointAndTicksRecord scratch, *pts, *pt;
	if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterModifiers, &c))
		self->modifiers = c;
	else
		self->modifiers = 0;
	if (kFskErrNone != FskEventParameterGetSize(event, kFskEventParameterMouseLocation, &c))
		return;
	if (sizeof(scratch) < c) {
		if (kFskErrNone != FskMemPtrNew(c, &pts))
			return;
	}
	else
		pts = &scratch;
	FskEventParameterGet(event, kFskEventParameterMouseLocation, pts);
	c /= sizeof(FskPointAndTicksRecord);
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	if ((!FskWindowCursorIsVisible(self->window)) && self->applicationChain.first) {
		KprContent application = self->applicationChain.first->content;
		FskRectangleRecord srcRect = self->bounds;
		FskRectangleRecord dstRect = application->bounds;
		KprContentToWindowCoordinates(application, dstRect.x, dstRect.y, &dstRect.x, &dstRect.y);
		for (i = 0, pt = pts; i < c; i++, pt++) {
			SInt32 x = dstRect.x + ((pt->pt.x *  dstRect.width) / srcRect.width);
			SInt32 y = dstRect.y + ((pt->pt.y *  dstRect.height) / srcRect.height);
			(*proc)(it, pt->index, x, y, pt->ticks);
		}
	}
	else
#endif
	{
		for (i = 0, pt = pts; i < c; i++, pt++)
			(*proc)(it, pt->index, pt->pt.x, pt->pt.y, pt->ticks);
	}
	if (pts != &scratch)
		FskMemPtrDispose(pts);
}

void KprShellMouseDown(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks)
{
	KprShell self = it;
	KprTouchLink link = (KprTouchLink)self->touchChain.first;
	if (link && (link->content->flags & kprExclusiveTouch)) {
		if (link->content->flags & kprMultipleTouch) {
			KprContentChainPrepend(&self->touchChain, link->content, sizeof(KprTouchLinkRecord), (KprContentLink*)&link);
			link->id = id;
		}
	}
	else {
		KprContent content = self->dispatch->hit(self, x - self->bounds.x, y - self->bounds.y);
		if (content) {
			if (!link && (content->flags & kprExclusiveTouch)) {
				KprContentChainPrepend(&self->touchChain, content, sizeof(KprTouchLinkRecord), (KprContentLink*)&link);
				link->id = id;
			}
			else {
				if (!KprContentChainContains(&self->touchChain, content) || (content->flags & kprMultipleTouch)) {
					KprContentChainPrepend(&self->touchChain, content, sizeof(KprTouchLinkRecord), (KprContentLink*)&link);
					link->id = id;
				}
				content = (KprContent)content->container;
				while (content) {
					if ((content->flags & kprActive) && (content->flags & kprBackgroundTouch)) {
						if (!KprContentChainContains(&self->touchChain, content) || (content->flags & kprMultipleTouch)) {
							KprContentChainAppend(&self->touchChain, content, sizeof(KprTouchLinkRecord), (KprContentLink*)&link);
							link->id = id;
						}
					}
					content = (KprContent)content->container;
				}
			}
		}
	}
	link = (KprTouchLink)KprContentChainGetFirst(&self->touchChain);
	while (link) {
		if (link->id == id) {
			KprContent content = link->content;
			KprTouchSampleAppend(link, x, y, ticks);
			kprDelegateTouchBegan(content, id, x, y, ticks);
		}
		link = (KprTouchLink)KprContentChainGetNext(&self->touchChain);
	}
	kprDelegateTouchBegan((KprContent)self, id, x, y, ticks);
	KprShellIdleCheck(self);
}

void KprShellMouseIdle(void* it)
{
	KprShell self = it;
	KprTouchLink link;
	link = (KprTouchLink)KprContentChainGetFirst(&self->touchChain);
	while (link) {
		if (link->index) {
			KprContent content = link->content;
			KprTouchSample sample = &(link->samples[link->index - 1]);
			kprDelegateTouchMoved(content, link->id, sample->x, sample->y, sample->ticks);
		}
		link = (KprTouchLink)KprContentChainGetNext(&self->touchChain);
	}
    
	link = (KprTouchLink)KprContentChainGetFirst(&self->touchChain);
	while (link) {
		link->index = 0;
		link = (KprTouchLink)KprContentChainGetNext(&self->touchChain);
	}
}

void KprShellMouseMoved(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks)
{
	KprShell self = it;
	KprTouchLink link;
	link = (KprTouchLink)KprContentChainGetFirst(&self->touchChain);
	while (link) {
		if (link->id == id)
			KprTouchSampleAppend(link, x, y, ticks);
		link = (KprTouchLink)KprContentChainGetNext(&self->touchChain);
	}
	kprDelegateTouchMoved((KprContent)self, id, x, y, ticks);
}

void KprShellMouseUp(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks)
{
	KprShell self = it;
	KprTouchLink link, *linkAddress;
	link = (KprTouchLink)KprContentChainGetFirst(&self->touchChain);
	while (link) {
		if (link->id == id) {
			KprContent content = link->content;
			KprTouchSampleAppend(link, x, y, ticks);
			kprDelegateTouchEnded(content, id, x, y, ticks);
		}
		link = (KprTouchLink)KprContentChainGetNext(&self->touchChain);
	}
	kprDelegateTouchEnded((KprContent)self, id, x, y, ticks);
	linkAddress = (KprTouchLink*)&self->touchChain.first;
	while ((link = *linkAddress)) {
		if (link->id == id) {
			*linkAddress = (KprTouchLink)link->next;
			FskMemPtrDispose(link);
		}
		else
			linkAddress = (KprTouchLink*)&link->next;
	}
	KprShellIdleCheck(self);
}

Boolean KprShellSensor(void* it, FskEvent event)
{
	Boolean result = false;
	FskErr err = kFskErrNone;
	KprShell self = it;
	UInt32 modifiers;
	UInt32 id = 0;
	FskTimeRecord time;
	double ticks;
	UInt32 c = 0;
	double scratch[6], *values = NULL;
	KprContent content = self->focus;
	while (content) {
		if ((content->flags & kprLayer) && (content->flags & kprBlocking))
			goto bail;
		content = (KprContent)content->container;
	}
	bailIfError(FskEventParameterGet(event, kFskEventParameterModifiers, &modifiers));
	bailIfError(FskEventParameterGet(event, kFskEventParameterCommand, &id));
	if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterTime, &time))
		ticks = ((xsNumberValue)time.seconds) * 1000.0 + (((xsNumberValue)time.useconds) / 1000);
	else
		ticks = KprShellTicks(it);
	if (kFskErrNone == FskEventParameterGetSize(event, kFskEventParameterApplicationParam1, &c)) {
		if (sizeof(scratch) < c) {
			bailIfError(FskMemPtrNew(c, &values));
		}
		else
			values = scratch;
		c /= sizeof(double);
		FskEventParameterGet(event, kFskEventParameterApplicationParam1, values);
	}
	content = self->focus;
	if (modifiers == kFskEventModifierShift) {
		while (content) {
			if (kprDelegateSensorBegan(content, id, ticks, c, values)) {
				result = true;
				break;
			}
			content = (KprContent)content->container;
		}
	}
	else if (modifiers == kFskEventModifierControl) {
		while (content) {
			if (kprDelegateSensorChanged(content, id, ticks, c, values)) {
				result = true;
				break;
			}
			content = (KprContent)content->container;
		}
	}
	else {
		while (content) {
			if (kprDelegateSensorEnded(content, id, ticks, c, values)) {
				result = true;
				break;
			}
			content = (KprContent)content->container;
		}
	}
bail:
	if (values != scratch)
		FskMemPtrDispose(values);
	return result;
}

void KprTouchSampleAppend(KprTouchLink link, SInt32 x, SInt32 y, double ticks)
{
	KprTouchSample sample;
	if (link->index == kprTouchSampleCount) {
		FskMemMove(&(link->samples[0]), &(link->samples[1]), (kprTouchSampleCount - 1) * sizeof(KprTouchSampleRecord));
		link->index--;
	}
	sample = &(link->samples[link->index]);
	sample->x = x;
	sample->y = y;
	sample->ticks = ticks;
	link->index++;
}

double KprShellGetVolume(KprShell self UNUSED)
{
	double volume;
#if TARGET_OS_ANDROID
	volume = gAndroidCallbacks->getVolumeCB();
#else
	volume = 1;	// @@
#endif
	return volume;
}

void KprShellSetVolume(KprShell self UNUSED, double volume)
{
#if TARGET_OS_ANDROID
	gAndroidCallbacks->setVolumeCB(volume);
#endif
}

void KPR_shell_get_breakOnException(xsMachine* the)
{
#ifdef mxDebug
	xsResult = xsCall0(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("getBreakOnException"));
#endif
}

void KPR_shell_get_debugging(xsMachine* the)
{
#ifdef mxDebug
	xsResult = xsCall0(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("getConnected"));
#endif
}

void KPR_shell_get_id(xsMachine* the)
{
	KprShell self = xsGetHostData(xsThis);
	xsResult = xsString(self->id);
}

void KPR_shell_get_profiling(xsMachine* the)
{
	xsResult = xsCall0(xsGet(xsGlobal, xsID("xs")), xsID("isProfiling"));
}

void KPR_shell_get_profilingDirectory(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* directory = NULL;
	xsResult = xsCall0(xsGet(xsGlobal, xsID("xs")), xsID("getProfilingDirectory"));
	if (xsTest(xsResult))
		err = KprPathToURL(xsToString(xsResult), &directory);
	if (directory) {	
		xsResult = xsString(directory);
		FskMemPtrDispose(directory);
	}
	else {
		xsThrowIfFskErr(err);
	}
}

void KPR_shell_get_touchMode(xsMachine* the)
{
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	KprShell self = xsGetHostData(xsThis);
	xsResult = FskWindowCursorIsVisible(self->window) ? xsFalse : xsTrue;
#else
	xsResult = xsFalse;
#endif
}

void KPR_shell_get_url(xsMachine* the)
{
	KprShell self = xsGetHostData(xsThis);
	xsResult = xsString(self->url);
}

void KPR_shell_get_windowState(xsMachine *the)
{
	KprShell self = xsGetHostData(xsThis);
	Boolean zoomed = true;
	SInt32 x, y;
	UInt32 width, height;
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	FskCocoaWindowIsZoomed(self->window, &zoomed);
	if (zoomed) FskCocoaWindowZoom(self->window);
#elif TARGET_OS_WIN32
	zoomed = IsZoomed(self->window->hwnd);
	if (zoomed) ShowWindow(self->window->hwnd, SW_RESTORE);
#endif
	FskWindowGetLocation(self->window, &x, &y);
	FskWindowGetSize(self->window, &width, &height);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID("zoomed"), xsBoolean(zoomed), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_x, xsInteger(x), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_y, xsInteger(y), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_width, xsInteger(width), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID_height, xsInteger(height), xsDefault, xsDontScript);
}

void KPR_shell_get_windowTitle(xsMachine *the)
{
	KprShell self = xsGetHostData(xsThis);
	char *title;
	FskWindowGetTitle(self->window, &title);
	xsResult = xsString(title);
	FskMemPtrDispose(title);
}

void KPR_shell_set_breakOnException(xsMachine* the)
{
#ifdef mxDebug
	(void)xsCall1(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("setBreakOnException"), xsArg(0));
#endif
}

void KPR_shell_set_debugging(xsMachine* the)
{
#ifdef mxDebug
	(void)xsCall1(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("setConnected"), xsArg(0));
#endif
}

void KPR_shell_set_profiling(xsMachine* the)
{
	if (xsTest(xsArg(0)))
		(void)xsCall0(xsGet(xsGlobal, xsID("xs")), xsID("startProfiling"));
	else
		(void)xsCall0(xsGet(xsGlobal, xsID("xs")), xsID("stopProfiling"));
}

void KPR_shell_set_profilingDirectory(xsMachine* the)
{
	char* directory;
	if (xsTest(xsArg(0))) {
		xsThrowIfFskErr(KprURLToPath(xsToString(xsArg(0)), &directory));
		xsResult = xsString(directory);
		FskMemPtrDispose(directory);
	}
	xsCall1(xsGet(xsGlobal, xsID("xs")), xsID("setProfilingDirectory"), xsResult);
}

void KPR_shell_set_touchMode(xsMachine* the)
{
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	KprShell self = xsGetHostData(xsThis);
	if (xsTest(xsArg(0))) {
		SInt32 x, y;
		UInt32 width, height;
		if (FskWindowCursorIsVisible(self->window))
			FskWindowHideCursor(self->window);
		FskWindowGetLocation(self->window, &x, &y);
		FskWindowGetSize(self->window, &width, &height);
		FskWindowMoveCursor(self->window, x + (width >> 1), y + (height >> 1));
	}
	else {
		if (!FskWindowCursorIsVisible(self->window))
			FskWindowShowCursor(self->window);
	}
#endif
}

void KPR_shell_set_windowState(xsMachine *the)
{
	KprShell self = xsGetHostData(xsThis);
#if (TARGET_OS_MAC && !TARGET_OS_IPHONE) || TARGET_OS_WIN32
	Boolean zoomed;
#endif
	SInt32 x, y;
	UInt32 width, height;
	xsEnterSandbox();
#if (TARGET_OS_MAC && !TARGET_OS_IPHONE) || TARGET_OS_WIN32
	zoomed = xsToBoolean(xsGet(xsArg(0), xsID("zoomed")));
#endif
	x = xsToInteger(xsGet(xsArg(0), xsID_x));
	y = xsToInteger(xsGet(xsArg(0), xsID_y));
	width = xsToInteger(xsGet(xsArg(0), xsID_width));
	height = xsToInteger(xsGet(xsArg(0), xsID_height));
	xsLeaveSandbox();
	FskWindowSetLocation(self->window, x, y);
	FskWindowSetSize(self->window, width, height);
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	if (zoomed) FskCocoaWindowZoom(self->window);
#elif TARGET_OS_WIN32
	if (zoomed) ShowWindow(self->window->hwnd, SW_MAXIMIZE);
#endif
}

void KPR_shell_set_windowTitle(xsMachine *the)
{
	KprShell self = xsGetHostData(xsThis);
	FskWindowSetTitle(self->window, xsToString(xsArg(0)));
}

void KPR_shell_alert(xsMachine* the)
{
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	xsStringValue type = xsToString(xsArg(0));
	xsStringValue title = xsToString(xsArg(1));
	xsStringValue message = xsToString(xsArg(2));
	CFOptionFlags flags;
	CFStringRef iconRef = NULL;
	CFURLRef urlRef = NULL;
	CFStringRef titleRef = CFStringCreateWithCString(kCFAllocatorDefault, title, kCFStringEncodingUTF8);
	CFStringRef messageRef = CFStringCreateWithCString(kCFAllocatorDefault, message, kCFStringEncodingUTF8);
	if (!FskStrCompare(type, "about")) {
		flags = kCFUserNotificationPlainAlertLevel;
		iconRef = CFStringCreateWithCString(kCFAllocatorDefault, "fsk.icns", kCFStringEncodingUTF8);
		urlRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(), iconRef, NULL, NULL);
	}
	else if (!FskStrCompare(type, "stop"))
		flags = kCFUserNotificationStopAlertLevel;
	else if (!FskStrCompare(type, "note"))
		flags = kCFUserNotificationNoteAlertLevel;
	else
		flags = kCFUserNotificationCautionAlertLevel;
	CFUserNotificationDisplayAlert(0, flags, urlRef, NULL, NULL, titleRef, messageRef, NULL, NULL, NULL, &flags);
	if (iconRef)
		CFRelease(iconRef);
	if (urlRef)
		CFRelease(urlRef);
	CFRelease(titleRef);
	CFRelease(messageRef);
#elif TARGET_OS_WIN32
	xsStringValue type = xsToString(xsArg(0));
	xsStringValue title = xsToString(xsArg(1));
	xsStringValue message = xsToString(xsArg(2));
	UInt16 *titleW = NULL;
	UInt16 *messageW = NULL;
	MSGBOXPARAMSW params;
	FskTextUTF8ToUnicode16LE(title, FskStrLen(title), &titleW, NULL);
	FskTextUTF8ToUnicode16LE(message, FskStrLen(message), &messageW, NULL);
	params.cbSize = sizeof(params);
	params.hwndOwner = NULL;
	params.hInstance = FskMainGetHInstance();
	params.lpszText = messageW;
	params.lpszCaption = titleW;
	if (!FskStrCompare(type, "about")) {
		params.dwStyle = MB_USERICON;
		params.lpszIcon = MAKEINTRESOURCE(107);
	}
	else if (!FskStrCompare(type, "stop"))
		params.dwStyle = MB_ICONSTOP;
	else if (!FskStrCompare(type, "note"))
		params.dwStyle = MB_ICONINFORMATION;
	else
		params.dwStyle = MB_ICONEXCLAMATION;
	params.dwContextHelpId = 0;
	params.lpfnMsgBoxCallback = NULL;
	params.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
	MessageBoxIndirectW(&params);
	FskMemPtrDispose(titleW);
	FskMemPtrDispose(messageW);
#elif (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
	KprShell shell = xsGetHostData(xsThis);
	FskGtkWindow win = shell->window->gtkWin;

	xsStringValue type = xsToString(xsArg(0));
	xsStringValue title = xsToString(xsArg(1));
	xsStringValue message = xsToString(xsArg(2));

	if(!FskStrCompare(type, "about")) {
		FskGtkWindowSetDialog(win, GTK_MESSAGE_OTHER, title, message);
	} else if(!FskStrCompare(type, "stop")) {
		FskGtkWindowSetDialog(win, GTK_MESSAGE_ERROR, title, message);
	} else if(!FskStrCompare(type, "note")) {
		FskGtkWindowSetDialog(win, GTK_MESSAGE_INFO, title, message);
	}
#endif
}

void KPR_shell_dump(xsMachine* the)
{
	KprShell self = xsGetHostData(xsThis);
	KprShellDump(self);
	if (gPictureImageCache) {
		fprintf(stderr, "IMAGES\n");
		KprImageCacheDump(gPictureImageCache);
	}
	if (gThumbnailImageCache) {
		fprintf(stderr, "THUMBNAILS\n");
		KprImageCacheDump(gThumbnailImageCache);
	}
}

void KPR_shell_keyDown(xsMachine* the)
{
	KprShell self = xsGetHostData(xsThis);
	char* key = xsToString(xsArg(0));
	UInt32 modifiers = xsToInteger(xsArg(1));
	UInt32 repeat = xsToInteger(xsArg(2));
	double ticks = xsToNumber(xsArg(3));
	KprShellKeyDown(self, key, modifiers, repeat, ticks);
}

void KPR_shell_keyUp(xsMachine* the)
{
	KprShell self = xsGetHostData(xsThis);
	char* key = xsToString(xsArg(0));
	UInt32 modifiers = xsToInteger(xsArg(1));
	UInt32 repeat = xsToInteger(xsArg(2));
	double ticks = xsToNumber(xsArg(3));
	KprShellKeyUp(self, key, modifiers, repeat, ticks);
}

void KPR_shell_purge(xsMachine* the)
{
	KprShell self = xsGetHostData(xsThis);
	self->shell->flags |= kprCollectGarbage;
}

void KPR_shell_quit(xsMachine* the)
{
	KprShell self = xsGetHostData(xsThis);
	KprShellClose(self);
}

void KPR_shell_sensorAux(xsMachine* the, UInt32 modifiers)
{
    FskErr err = kFskErrNone;
	xsIntegerValue c = xsToInteger(xsArgc), i;
	KprShell self = xsGetHostData(xsThis);
	xsIntegerValue id = xsToInteger(xsArg(0));
//	double ticks = xsToNumber(xsArg(1));
	double scratch[6];
    double *values = NULL;
	FskEvent event = NULL;
	c -= 2;
	if (c > 6) {
		bailIfError(FskMemPtrNew(c * sizeof(double), &values));
	}
	else
		values = scratch;
	for (i = 0; i < c; i++)
		values[i] = xsToNumber(xsArg(2 + i));
	bailIfError(FskEventNew(&event, kFskEventButton, NULL, modifiers));
	bailIfError(FskEventParameterAdd(event, kFskEventParameterCommand, sizeof(UInt32), &id));
	if (c) {
		bailIfError(FskEventParameterAdd(event, kFskEventParameterApplicationParam1, c * sizeof(double), values));
    }
	bailIfError(FskWindowEventSend(self->window, event));
bail:
	if (values != scratch)
		FskMemPtrDispose(values);
	if (err)
		FskEventDispose(event);
	xsThrowIfFskErr(err);
}

void KPR_shell_sensorBegan(xsMachine* the)
{
	KPR_shell_sensorAux(the, kFskEventModifierShift);
}

void KPR_shell_sensorChanged(xsMachine* the)
{
	KPR_shell_sensorAux(the, kFskEventModifierControl);
}

void KPR_shell_sensorEnded(xsMachine* the)
{
	KPR_shell_sensorAux(the, kFskEventModifierAlt);
}

void KPR_shell_updateMenus(xsMachine* the)
{
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	xsIntegerValue c, i, d, j;
	xsVars(3);
	CocoaMenuBarClear();
	xsEnterSandbox();
	xsResult = xsGet(xsThis, xsID("menus"));
	if (xsTest(xsResult)) {
		c = xsToInteger(xsGet(xsResult, xsID("length")));
		for (i = 0; i < c; i++) {
			xsVar(0) = xsGetAt(xsResult, xsInteger(i));
			CocoaMenuAdd(i, xsToString(xsGet(xsVar(0), xsID("title"))));
			xsVar(1) = xsGet(xsVar(0), xsID("items"));
			d = xsToInteger(xsGet(xsVar(1), xsID("length")));
			for (j = 0; j < d; j++) {
				xsVar(2) = xsGetAt(xsVar(1), xsInteger(j));
				if (xsTest(xsVar(2))) {
					xsIntegerValue id = (i << 8) | (j + 1);
					xsStringValue title = xsToString(xsGet(xsVar(2), xsID("title")));
					xsStringValue key, command;
					char buffer[256];
                    xsIndex canID, doID;
					if (!xsFindString(xsVar(2), xsID("key"), &key))
						key = NULL;
					command = xsToString(xsGet(xsVar(2), xsID("command")));
					FskStrCopy(buffer, "can");
					FskStrCat(buffer, command);
					canID = xsID(buffer);
					FskStrCopy(buffer, "do");
					FskStrCat(buffer, command);
					doID = xsID(buffer);
					CocoaMenuItemAdd(i, id, title, key, buffer);
					xsSet(xsVar(2), xsID("canID"), xsInteger(canID));
					xsSet(xsVar(2), xsID("doID"), xsInteger(doID));
				}
				else
					CocoaMenuItemAddSeparator(i);
			}
		}
	}
	xsLeaveSandbox();
#elif TARGET_OS_WIN32
	xsIntegerValue c, i, d, j;
	KprShell shell = xsGetHostData(xsThis);
	HWND hwnd = shell->window->hwnd;
	HMENU hmenu = GetMenu(hwnd);
	HACCEL haccel = shell->window->haccel;
	xsVars(4);
	xsEnterSandbox();
	xsResult = xsGet(xsThis, xsID("menus"));
	if (xsTest(xsResult)) {
		HMENU menuBar = CreateMenu();
		int acceleratorCount = 0;
		ACCEL accelerators[256];
		c = xsToInteger(xsGet(xsResult, xsID("length")));
		for (i = 0; i < c; i++) {
			HMENU menu = CreateMenu();
			xsVar(0) = xsGetAt(xsResult, xsInteger(i));
			AppendMenu(menuBar, MF_POPUP, (UINT)menu, xsToString(xsGet(xsVar(0), xsID("title"))));
			xsVar(1) = xsGet(xsVar(0), xsID("items"));
			d = xsToInteger(xsGet(xsVar(1), xsID("length")));
			for (j = 0; j < d; j++) {
				xsVar(2) = xsGetAt(xsVar(1), xsInteger(j));
				if (xsTest(xsVar(2))) {
					xsIntegerValue id = (i << 8) | (j + 1);
					xsStringValue title = xsToString(xsGet(xsVar(2), xsID("title")));
					xsStringValue key = NULL, command;
					char buffer[256];
                    xsIndex canID, doID;
  					if (xsHas(xsVar(2), xsID("key"))) {
  						xsVar(3) = xsGet(xsVar(2), xsID("key"));
  						if (xsTest(xsVar(3)))
  							key = xsToString(xsVar(3));
					}
					if (key && (acceleratorCount < 256)) {
						ACCEL* accelerator = accelerators + acceleratorCount;
						accelerator->fVirt = FCONTROL | FVIRTKEY;
						accelerator->key = LOBYTE(VkKeyScan(key[0]));
						accelerator->cmd = id;
						acceleratorCount++;
						FskStrCopy(buffer, title);
						FskStrCat(buffer, "\tCtrl+");
						FskStrCat(buffer, key);
						AppendMenu(menu, MF_STRING, id, buffer);
					}
					else
						AppendMenu(menu, MF_STRING, id, title);
					command = xsToString(xsGet(xsVar(2), xsID("command")));
					FskStrCopy(buffer, "can");
					FskStrCat(buffer, command);
					canID = xsID(buffer);
					FskStrCopy(buffer, "do");
					FskStrCat(buffer, command);
					doID = xsID(buffer);
					xsSet(xsVar(2), xsID("canID"), xsInteger(canID));
					xsSet(xsVar(2), xsID("doID"), xsInteger(doID));
				}
				else
					AppendMenu(menu, MF_SEPARATOR, -1, NULL);
			}
		}
		SetMenu(hwnd, menuBar);
		shell->window->haccel = CreateAcceleratorTable(accelerators, acceleratorCount);
	}
	else {
		SetMenu(hwnd, NULL);
		shell->window->haccel = NULL;
	}
	xsLeaveSandbox();
	if (haccel)
		DestroyAcceleratorTable(haccel);
	if (hmenu)
		DestroyMenu(hmenu);
#elif (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
	xsIntegerValue c, i, d, j;
	KprShell shell = xsGetHostData(xsThis);
	FskGtkWindow win = shell->window->gtkWin;
	FskGtkWindowMenuBarClear(win);

	xsVars(3);
	xsEnterSandbox();
	xsResult = xsGet(xsThis, xsID("menus"));

	if (xsTest(xsResult)) {
		char buffer0[256];
		c = xsToInteger(xsGet(xsResult, xsID("length")));
		for (i = 0; i < c; i++) {
			xsVar(0) = xsGetAt(xsResult, xsInteger(i));
			xsStringValue title = xsToString(xsGet(xsVar(0), xsID("title")));
			GtkWidget *menuItem;
			GtkWidget *menuGroup = gtk_menu_new();

			FskStrCopy(buffer0, "_");
			FskStrCat(buffer0, title);
			menuItem = gtk_menu_item_new_with_mnemonic(buffer0);
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), menuGroup);
			gtk_widget_show(menuGroup);
			FskGtkWindowSetMenuBar(win, menuItem, (i<<8)); // Attach current menuItem to menuBar

			xsVar(1) = xsGet(xsVar(0), xsID("items"));
			d = xsToInteger(xsGet(xsVar(1), xsID("length")));
			for (j = 0; j < d; j++) {
				xsVar(2) = xsGetAt(xsVar(1), xsInteger(j));
				if (xsTest(xsVar(2))) {
					xsIntegerValue id = (i << 8) | (j + 1);
					xsStringValue title = xsToString(xsGet(xsVar(2), xsID("title")));
					xsStringValue key, command;
					char buffer[256];
                    xsIndex canID, doID;

					if (!xsFindString(xsVar(2), xsID("key"), &key))
						key = NULL;

					command = xsToString(xsGet(xsVar(2), xsID("command")));
					FskStrCopy(buffer, "can");
					FskStrCat(buffer, command);
					canID = xsID(buffer);
					FskStrCopy(buffer, "do");
					FskStrCat(buffer, command);
					doID = xsID(buffer);
					xsSet(xsVar(2), xsID("canID"), xsInteger(canID));
					xsSet(xsVar(2), xsID("doID"), xsInteger(doID));
					// FIXME: here we should use gtk_check_menu_item_new_with_label, but it has display bug on Ubutun-12
					//  So use gtk_check_menu_item_new_with_mnemonic for workaround
					GtkWidget* menu = gtk_check_menu_item_new_with_mnemonic(title);
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), FALSE); //Default is not checked
					gtk_menu_shell_append(GTK_MENU_SHELL(menuGroup), menu);
					FskGtkWindowSetMenuItemCallback(win, menu, id);
					if( key && (key[0] >= '0' && key[0] <= 'z')) {
						gtk_widget_add_accelerator(menu, "activate", win->accelGroup, (GdkModifierType)key[0], GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
					}
					gtk_widget_show(menu);

				} else {
					GtkWidget* separator = gtk_separator_menu_item_new();
					gtk_menu_shell_append(GTK_MENU_SHELL(menuGroup), separator);
					gtk_widget_show(separator);
				}
			}
		}
	}
	xsLeaveSandbox();

#endif
}

void KPR_touches(void* it UNUSED)
{
}

void KPR_touches_peek(xsMachine* the)
{
	KprTouchLink link = (KprTouchLink)gShell->touchChain.first;
    KprTouchSample sample;
	UInt32 id = (UInt32)xsToInteger(xsArg(0)), i;
	xsVars(1);
	while (link) {
		if (link->id == id) {
			xsResult = xsNew1(xsGlobal, xsID_Array, xsNumber(link->index));
			for (i = 0, sample = &(link->samples[0]); i < link->index; i++, sample++) {
				xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
				xsNewHostProperty(xsVar(0), xsID_x, xsInteger(sample->x), xsDefault, xsDontScript);
				xsNewHostProperty(xsVar(0), xsID_y, xsInteger(sample->y), xsDefault, xsDontScript);
				xsNewHostProperty(xsVar(0), xsID_ticks, xsNumber(sample->ticks), xsDefault, xsDontScript);
                xsSetAt(xsResult, xsInteger(i), xsVar(0));
			}
			break;
		}
		link = (KprTouchLink)link->next;
	}
}

void KPR_system_bar(void* it UNUSED)
{
}

void KPR_system_bar_get_visible(xsMachine* the)
{
#if TARGET_OS_ANDROID
	xsResult = xsBoolean(gAndroidCallbacks->isIMEEnabledCB());
#elif TARGET_OS_IPHONE
    xsResult = FskCocoaApplicationGetStatusBarHeight() > 0 ? xsTrue : xsFalse;
#else
	KprShell self = gShell;
	KprScriptBehavior behavior = (KprScriptBehavior)self->behavior;
	Boolean visible = false;
	if (behavior) {
		xsBeginHostSandboxCode(self->the, self->code);
		{
			xsVars(3);
			if (xsFindResult(behavior->slot, xsID("onGetSystemBarVisible"))) {
				visible = xsToBoolean(xsCallFunction1(xsResult, behavior->slot, self->slot));
			}
		}
		xsEndHostSandboxCode();
	}
	xsResult = xsBoolean(visible);
#endif
}

void KPR_system_bar_set_visible(xsMachine* the)
{
#if TARGET_OS_ANDROID
	gAndroidCallbacks->systemBarShowCB(xsTest(xsArg(0)) ? 1 : 0);
#elif TARGET_OS_IPHONE
    FskCocoaApplicationSetStatusBarHidden(xsTest(xsArg(0)) ? false : true);
#else
	KprShell self = gShell;
	KprScriptBehavior behavior = (KprScriptBehavior)self->behavior;
	Boolean visible = xsToBoolean(xsArg(0));
	if (behavior) {
		xsBeginHostSandboxCode(self->the, self->code);
		{
			xsVars(3);
			if (xsFindResult(behavior->slot, xsID("onSetSystemBarVisible"))) {
				(void)xsCallFunction2(xsResult, behavior->slot, self->slot, xsBoolean(visible));
			}
		}
		xsEndHostSandboxCode();
	}
#endif
}

void KPR_system_set_volume(xsMachine* the)
{
	KprShell self = gShell;
	double volume = xsToNumber(xsArg(0));
	KprShellSetVolume(self, volume);
}

void KPR_system_get_volume(xsMachine* the)
{
	KprShell self = gShell;
	double volume = KprShellGetVolume(self);
	xsResult = xsNumber(volume);
}

void KPR_system_keyboard(void* it UNUSED)
{
}

void KPR_system_keyboard_get_available(xsMachine* the)
{
#if TARGET_OS_ANDROID
	xsResult = xsTrue;
#elif TARGET_OS_IPHONE
	xsResult = xsTrue;
#else
	xsResult = xsTrue;
#endif
}

void KPR_system_keyboard_get_visible(xsMachine* the)
{
#if TARGET_OS_ANDROID
	xsResult = xsBoolean(gAndroidCallbacks->isIMEEnabledCB());
#elif TARGET_OS_IPHONE
	xsResult = xsBoolean(FskCocoaWindowGetSIPEnabled(gShell->window));
#else
	KprShell self = gShell;
	KprScriptBehavior behavior = (KprScriptBehavior)self->behavior;
	Boolean visible = false;
	if (behavior) {
		xsBeginHostSandboxCode(self->the, self->code);
		{
			xsVars(3);
			if (xsFindResult(behavior->slot, xsID("onGetSystemKeyboardVisible"))) {
				visible = xsToBoolean(xsCallFunction1(xsResult, behavior->slot, self->slot));
			}
		}
		xsEndHostSandboxCode();
	}
	xsResult = xsBoolean(visible);
#endif
}

void KPR_system_keyboard_set_visible(xsMachine* the)
{
#if TARGET_OS_ANDROID
	gKeyboardVisible = xsToBoolean(xsArg(0));
	gAndroidCallbacks->IMEEnableCB(xsToBoolean(xsArg(0)));
#elif TARGET_OS_IPHONE
	gKeyboardVisible = xsToBoolean(xsArg(0));
	FskCocoaWindowSetSIPEnabled(gShell->window, xsToBoolean(xsArg(0)));
#else
	KprShell self = gShell;
	KprScriptBehavior behavior = (KprScriptBehavior)self->behavior;
	Boolean visible = xsToBoolean(xsArg(0));
	if (behavior) {
		xsBeginHostSandboxCode(self->the, self->code);
		{
			xsVars(3);
			if (xsFindResult(behavior->slot, xsID("onSetSystemKeyboardVisible"))) {
				(void)xsCallFunction2(xsResult, behavior->slot, self->slot, xsBoolean(visible));
			}
		}
		xsEndHostSandboxCode();
	}
#endif
}

void KPR_system_Power(xsMachine *the)
{
	char *what = xsToString(xsArg(0));

	if (0 == FskStrCompare(what, "backlight")) {
		FskUtilsEnergySaverUpdate(kFskUtilsEnergySaverDisableScreenDimming, kFskUtilsEnergySaverDisableScreenDimming);
		xsSetHostData(xsThis, (void *)kFskUtilsEnergySaverDisableScreenDimming);
	}
	else
	if (0 == FskStrCompare(what, "dim")) {
		FskUtilsEnergySaverUpdate(kFskUtilsEnergySaverDisableScreenSleep, kFskUtilsEnergySaverDisableScreenSleep);
		xsSetHostData(xsThis, (void *)kFskUtilsEnergySaverDisableScreenSleep);
	}
	else
	if (0 == FskStrCompare(what, "active")) {
		FskUtilsEnergySaverUpdate(kFskUtilsEnergySaverDisableSleep, kFskUtilsEnergySaverDisableSleep);
		xsSetHostData(xsThis, (void *)kFskUtilsEnergySaverDisableSleep);
	}
	else
		xsThrowIfFskErr(kFskErrUnimplemented);
}

void KPR_system_power_destructor(void *data)
{
	if (data)
		FskUtilsEnergySaverUpdate((UInt32)data, 0);
}

void KPR_system_power_close(xsMachine *the)
{
	KPR_system_power_destructor(xsGetHostData(xsThis));
	xsSetHostData(xsThis, NULL);
}

void KPR_system_get_platform(xsMachine* the)
{
	char *platform = NULL;
#if TARGET_OS_WIN32
	platform = "win";
#elif TARGET_OS_MAC
	#if TARGET_OS_IPHONE
		platform = "iphone";
	#else
		platform = "mac";
	#endif
#elif TARGET_OS_LINUX
	#if TARGET_OS_ANDROID
		platform = "android";
	#else
		platform = "linux";
	#endif
#elif TARGET_OS_KPL
	platform = (char*)KplECMAScriptGetPlatform();
#else
	platform = "unknown";
#endif
	xsResult = xsString(platform);
}

//void KprShellHTTPAuthenticate(KprShell self, void* it, char* host, char* realm)
//{
//	KprBehavior behavior = self->behavior;
//	Boolean result = false;
//	xsBeginHostSandboxCode(self->the, self->code);
//	if (xsFindResult(behavior->slot, xsID_onHTTPAuthenticate)) {
//		KprHTTPClientMessage message = it;
//		message->the = the;
//		message->slot = xsNewInstanceOf(xsGet(xsGet(xsGet(xsGet(xsGlobal, xsID_KPR), xsID_HTTP), xsID_Client), xsID_message));
//		xsSetHostData(message->slot, message);
//		(void)xsCall1(xsGet(xsGlobal, xsID_Object), xsID_seal, message->slot);
//		xsResult = xsCallFunction3(xsResult, self->slot, message->slot, xsString(host), xsString(realm));
//		result = xsToBoolean(xsResult);
//	}
//	xsEndHostSandboxCode();
//}


