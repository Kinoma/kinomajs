/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#ifndef __FSKCOCOASUPPORTPHONE__
#define __FSKCOCOASUPPORTPHONE__

#if defined(__FSKCOCOASUPPORT_PRIV__)
#import <UIKit/UIKit.h>
#import <MediaPlayer/MediaPlayer.h>
#endif

#include "FskWindow.h"
#include "FskDragDrop.h"
#include "FskBitmap.h"
#include "FskImage.h"
#include "FskText.h"
#include "FskCocoaSupportCommon.h"
#include "xs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	// definition
enum {
	cocoaTextInputTypeDefault = 0,
	cocoaTextInputTypeMultiLine = 1,
	cocoaTextInputTypeNumeric = 2,
	cocoaTextInputTypePassword = 3,
	cocoaTextInputTypeEmail = 4,
	cocoaTextInputTypeURI = 5,
	cocoaTextInputTypePhone = 6
};

	// system
void FskCocoaSystemGetVersion(char *version);

	// application
void FskCocoaApplicationRun();
void FskCocoaApplicationStop();
void FskCocoaApplicationRunLoop(void);
void FskCocoaApplicationRunLoopAddCallback(void);
void FskCocoaApplicationSetIdleTimerDisabled(Boolean disabled);
SInt32 FskCocoaApplicationGetRotation(void);
UInt32 FskCocoaApplicationGetStatusBarHeight();
void FskCocoaApplicationSetStatusBarHidden(Boolean hidden);
typedef FskErr (*FskCocoaApplicationInterruptionCB)(Boolean interrupted, Boolean resume);
void FskCocoaApplicationSetInterruptionCB(FskCocoaApplicationInterruptionCB cb);
UInt32 FskCocoaApplicationInstallInterruptionCB(void (^callback)(Boolean interrupted, Boolean resume));
void FskCocoaApplicationRemoveInterruptionCB(UInt32 callbackId);
void FskCocoaApplicationSetInterruption(Boolean interrupted, Boolean resume);

	// thread
void FskCocoaThreadInitialize(FskThread fskThread);
void FskCocoaThreadTerminate(FskThread fskThread);
void FskCocoaThreadRun(FskThread fskThread, SInt32 milliseconds);
void FskCocoaThreadWake(FskThread fskThread);

	// window
Boolean FskCocoaWindowCreate(FskWindow fskWindow, Boolean isCustomWindow, Boolean isFullScreenWindow, SInt32 width, SInt32 height);
void FskCocoaWindowDispose(FskWindow fskWindow);
void FskCocoaWindowCopyBits(FskWindow fskWindow, FskBitmap fskBitmap, const FskRectangle sourceFskRect, const FskRectangle destFskRect);
void FskCocoaWindowGetVisible(FskWindow fskWindow, Boolean *visible);
void FskCocoaWindowSetVisible(FskWindow fskWindow, Boolean visible);
void FskCocoaWindowGetSize(FskWindow fskWindow, UInt32 *width, UInt32 *height);
void FskCocoaWindowSetSize(FskWindow fskWindow, UInt32 width, UInt32 height);
void FskCocoaWindowGetOrigin(FskWindow fskWindow, SInt32 *x, SInt32 *y);
void FskCocoaWindowSetOrigin(FskWindow fskWindow, SInt32 x, SInt32 y);
void FskCocoaWindowGetMouseLocation(FskWindow fskWindow, FskPoint fskPoint);
void FskCocoaWindowSetNeedsDisplay(FskWindow fskWindow, Boolean needsDisplay);
void FskCocoaWindowSetNeedsDisplayInRect(FskWindow fskWindow, FskRectangle displayFskRect);
void FskCocoaWindowBeginDraw(FskWindow fskWindow);
void FskCocoaWindowEndDraw(FskWindow fskWindow);
Boolean FskCocoaWindowGetSIPEnabled(FskWindow fskWindow);
void FskCocoaWindowSetSIPEnabled(FskWindow fskWindow, Boolean enable);
void FskCocoaWindowInputTextActivate(FskWindow fskWindow, xsMachine *the, xsSlot obj, Boolean active, int mode);
void FskCocoaWindowInputTextSetSelection(FskWindow fskWindow, const char *text, UInt32 textLength, SInt32 selectionStart, SInt32 selectionEnd);
void FskCocoaWindowSetUpdates(FskWindow fskWindow);
void FskCocoaWindowGetScreenScale(FskWindow fskWindow, float *scale);

#if SUPPORT_EXTERNAL_SCREEN && TEST_EXTERNAL_SCREEN
FskWindow FskCocoaWindowGetExternalWindow(FskWindow window);
void FskCocoaWindowSetBitmapForExternalWindow(FskWindow window, FskBitmap bitmap);
#endif	/* TEST_EXTERNAL_SCREEN && SUPPORT_EXTERNAL_SCREEN */

	// event
void FskCocoaEventSend(FskWindow fskWindow, UInt32 eventClass, UInt32 eventType);
Boolean FskCocoaWindowDisplayPaused(FskWindow window);

	// bitmap
#if defined(__FSKCOCOASUPPORT_PRIV__)
Boolean FskCocoaBitmapGetColorInfo(UInt32 pixelFormat, SInt32 width, CGImageAlphaInfo *alphaInfoOut, CGColorSpaceRef *colorSpaceOut, UInt32 *rowBytesOut);
#endif
Boolean FskCocoaBitmapCreate(FskBitmap fskBitmap, UInt32 pixelFormat, SInt32 width, SInt32 height);
void FskCocoaBitmapDispose(FskBitmap fskBitmap);

	// timer
void FskCocoaTimerReschedule(FskThread thread);

	// time
time_t FskCocoa_mktime(struct tm *time);

	// word
char *FskCocoaWordVersion();
Boolean FskCocoaWordGetAttributes(char *filePath, FskMediaPropertyValue name, FskMediaPropertyValue author, FskMediaPropertyValue subject);
Boolean FskCocoaWordConvertRTF(char *sourcePath, char *destinationPath, char *format);

	// html
char *FskCocoaHTMLVersion();
Boolean FskCocoaHTMLGetAttributes(char *filePath, FskMediaPropertyValue name, FskMediaPropertyValue author, FskMediaPropertyValue subject);
Boolean FskCocoaHTMLConvertRTF(char *sourcePath, char *destinationPath, char *format);

	// open
Boolean FskCocoaOpenURL(const char *url);
Boolean FskCocoaOpenFile(const char *filePath);

	// device
char *FskCocoaDeviceName();
//char *FskCocoaDeviceUDID();
float FskCocoaDeviceScreenScaleFactor();

	// autorelease pool
struct FskCocoaAutoreleasePoolRecord;
typedef struct FskCocoaAutoreleasePoolRecord *FskCocoaAutoreleasePool;
FskCocoaAutoreleasePool FskCocoaAutoreleasePoolNew();
void FskCocoaAutoreleasePoolDispose(FskCocoaAutoreleasePool pool);

typedef void *FskCocoaPhotoAssets;
typedef void *FskCocoaPhotoAssetGroup;
typedef void (*FskCocoaPhotoGroupAssetsCallback)(FskCocoaPhotoAssetGroup group, void *closure);
typedef void (*FskCocoaPhotoAssetsCallback)(void *data, UInt32 size, void *closure);
FskCocoaPhotoAssets FskCocoaPhotoAssetsNew(FskCocoaPhotoGroupAssetsCallback callback, void *closure);
int FskCocoaPhotoAssetsGetCount(FskCocoaPhotoAssetGroup group);
void FskCocoaPhotoAssetsGetImage(FskCocoaPhotoAssetGroup group, int i, FskCocoaPhotoAssetsCallback callback, void *closure);
void FskCocoaPhotoAssetsDispose(FskCocoaPhotoAssets assets);

	// media
#if defined(__FSKCOCOASUPPORT_PRIV__)
UIImage *FskCocoaMediaImageFromArtwork(MPMediaItemArtwork *artwork, SInt32 generation, UInt32 targetWidth, UInt32 targetHeight);
NSString *FskCocoaMediaIdFromURLString(NSString *urlString, BOOL urlEncoded);
#endif

	// EAGL Context
#ifdef __OBJC__
@class EAGLContext;
#elif !defined(__FSKGLCONTEXT__) /* !__OBJC__ */
struct EAGLContext;
typedef struct EAGLContext EAGLContext;
#endif /* __OBJC__ */
EAGLContext* FskEAGLContextNew(int version, EAGLContext *share);
EAGLContext* FskEAGLContextGetCurrent(void);
FskErr FskEAGLContextSetCurrent(EAGLContext *ctx);
FskErr FskEAGLSwapBuffers(EAGLContext *ctx);
//FskErr FskEAGLContextDispose(EAGLContext *ctx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKCOCOASUPPORTPHONE__ */
