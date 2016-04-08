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
#ifndef __FSKCOCOASUPPORT__
#define __FSKCOCOASUPPORT__

#include "FskWindow.h"
#include "FskDragDrop.h"
#include "FskBitmap.h"
#include "FskImage.h"
#include "FskAudio.h"
#include "FskText.h"
#include "FskFiles.h"
#include "FskCocoaSupportCommon.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



	// application
	void FskCocoaApplicationRun(void);
	void FskCocoaApplicationStop(void);

	// thread
	void FskCocoaThreadInitialize(FskThread fskThread);
	void FskCocoaThreadTerminate(FskThread fskThread);
	void FskCocoaThreadRun(FskThread fskThread, SInt32 milliseconds);
	void FskCocoaThreadWake(FskThread fskThread);

	// window
	Boolean FskCocoaWindowCreate(FskWindow fskWindow, Boolean isCustomWindow, SInt32 width, SInt32 height);
	FskAPI(void) FskCocoaWindowDispose(FskWindow fskWindow);
	FskAPI(void *) FskCocoaWindowGetWindowRef(FskWindow fskWindow);
	FskAPI(void) FskCocoaWindowToggleFullScreen(FskWindow fskWindow);
	FskAPI(void) FskCocoaWindowIsFullScreen(FskWindow fskWindow, Boolean *isFullScreen);
	FskAPI(void) FskCocoaWindowMinimize(FskWindow fskWindow);
	FskAPI(void) FskCocoaWindowIsMinimized(FskWindow fskWindow, Boolean *isMinimized);
	FskAPI(void) FskCocoaWindowZoom(FskWindow fskWindow);
	FskAPI(void) FskCocoaWindowIsZoomed(FskWindow fskWindow, Boolean *isZoomed);
	FskAPI(void) FskCocoaWindowCopyBits(FskWindow fskWindow, FskBitmap fskBitmap, const FskRectangle sourceFskRect, const FskRectangle destFskRect);
	FskAPI(void) FskCocoaWindowGetVisible(FskWindow fskWindow, Boolean *visible);
	FskAPI(void) FskCocoaWindowSetVisible(FskWindow fskWindow, Boolean visible);
	FskAPI(void) FskCocoaWindowGetSize(FskWindow fskWindow, UInt32 *width, UInt32 *height);
	FskAPI(void) FskCocoaWindowSetSize(FskWindow fskWindow, UInt32 width, UInt32 height);
	FskAPI(void) FskCocoaWindowGetOrigin(FskWindow fskWindow, SInt32 *x, SInt32 *y);
	FskAPI(void) FskCocoaWindowSetOrigin(FskWindow fskWindow, SInt32 x, SInt32 y);
	FskAPI(void) FskCocoaWindowGetTitle(FskWindow fskWindow, char **title);
	FskAPI(void) FskCocoaWindowSetTitle(FskWindow fskWindow, const char *title);
	FskAPI(void) FskCocoaWindowGetMouseLocation(FskWindow fskWindow, FskPoint fskPoint);
	FskAPI(void) FskCocoaWindowSetNeedsDisplay(FskWindow fskWindow, Boolean needsDisplay);
	FskAPI(void) FskCocoaWindowSetNeedsDisplayInRect(FskWindow fskWindow, FskRectangle displayFskRect);
	FskAPI(void) FskCocoaWindowDragMouseDown(FskWindow fskWindow);
	FskAPI(void) FskCocoaWindowDragMouseMoved(FskWindow fskWindow);
	FskAPI(void) FskCocoaWindowResizeMouseDown(FskWindow fskWindow);
	FskAPI(void) FskCocoaWindowResizeMouseMoved(FskWindow fskWindow);
	FskAPI(void) FskCocoaWindowBeginDraw(FskWindow fskWindow);
	FskAPI(void) FskCocoaWindowEndDraw(FskWindow fskWindow);
#if USE_DISPLAY_LINK
	FskAPI(void) FskCocoaWindowSetUpdates(FskWindow fskWindow);
#endif

	// drag and drop
	FskAPI(void) FskCocoaDragDropWindowRegister(FskWindow fskWindow, FskDragDropTargetProc dropTargetProc);
	FskAPI(void) FskCocoaDragDropWindowUnregister(FskWindow fskWindow);
	FskAPI(void) FskCocoaDragDropWindowResult(FskWindow fskWindow, Boolean result);

	// event
	FskAPI(void) FskCocoaEventSend(FskWindow fskWindow, UInt32 eventClass, UInt32 eventType);
#if USE_DISPLAY_LINK
	FskAPI(Boolean) FskCocoaWindowDisplayPaused(FskWindow window);
#endif

	// cursor
	FskAPI(void) FskCocoaCursorSet(FskWindow fskWindow, UInt32 cursorShape);

	// menu (primitives)
	FskAPI(void) CocoaMenuBarClear(void);
	FskAPI(void) CocoaMenuAdd(UInt32 menuID, char *title);
	FskAPI(void) CocoaMenuItemAdd(UInt32 menuID, UInt32 menuItemID, char *title, char *key, char *command);
	FskAPI(void) CocoaMenuItemAddSeparator(UInt32 menuID);
	FskAPI(void) CocoaMenuItemSetEnable(UInt32 menuID, UInt32 menuItemID, Boolean enable);
	FskAPI(void) CocoaMenuItemSetCheck(UInt32 menuID, UInt32 menuItemID, Boolean check);
	FskAPI(void) CocoaMenuItemSetTitle(UInt32 menuID, UInt32 menuItemID, char *title);
	FskAPI(void) CocoaMenuItemSendAction(UInt32 menuID, UInt32 menuItemID);
	FskAPI(void) CocoaMenuSetupDefaultMenuItems();
	FskAPI(void) CocoaMenuSetupEditMenuItems();
	FskAPI(void) CocoaMenuUpdateFullScreenTitle(Boolean fullScreen);

	// bitmap
	Boolean FskCocoaBitmapCreate(FskBitmap fskBitmap, UInt32 pixelFormat, SInt32 width, SInt32 height);
	FskAPI(void) FskCocoaBitmapDispose(FskBitmap fskBitmap);

	// JPEG
	Boolean FskCocoaJPEGDecompressFrame(const void *data, UInt32 dataSize, UInt32 width, UInt32 height, FskBitmap outputFskBitmap);
	Boolean FskCocoaJPEGCompressFrame(FskImageCompress fskImageCompress, FskBitmap sourceFskBitmap, const void **data, UInt32 *dataSize);

	// file
	FskAPI(void) FskCocoaFileChoose(const FskFileChooseEntry types, const char *prompt, Boolean allowMultiple, const char *initialPath, char **files);
	FskAPI(void) FskCocoaFileSaveChoose(const char *defaultName, const char *prompt, const char *initialDirectory, char **file);
	FskAPI(void) FskCocoaFileDirectoryChoose(const char *prompt, const char *initialPath, char **directory);
	typedef void (*FskCocoaVolumeNotifierCallback)(UInt32 status, const char *path, void *refCon);
	FskAPI(void *) FskCocoaVolumeNotifierNew(FskCocoaVolumeNotifierCallback callback, void *refCon);
	FskAPI(void) FskCocoaVolumeNotifierDispose(void *obj);

	// clipboard
	FskAPI(void) FskCocoaClipboardTextGet(char **text);
	FskAPI(void) FskCocoaClipboardTextSet(char *text);
	FskAPI(void) FskCocoaClipboardTextClear(void);
	FskAPI(void) FskCocoaClipboardTextTypes(char **typesList);

	// timer
	FskAPI(void) FskCocoaTimerReschedule(FskThread thread);

	// time
	time_t FskCocoa_mktime(struct tm *time);

	// word
	char *FskCocoaWordVersion(void);
	Boolean FskCocoaWordGetAttributes(char *filePath, FskMediaPropertyValue name, FskMediaPropertyValue author, FskMediaPropertyValue subject);
	Boolean FskCocoaWordConvertRTF(char *sourcePath, char *destinationPath, char *format);

#if 0
	// html
	char *FskCocoaHTMLVersion(void);
	Boolean FskCocoaHTMLGetAttributes(char *filePath, FskMediaPropertyValue name, FskMediaPropertyValue author, FskMediaPropertyValue subject);
	Boolean FskCocoaHTMLConvertRTF(char *sourcePath, char *destinationPath, char *format);
#endif

	// open
	Boolean FskCocoaOpenURL(const char *url);
	Boolean FskCocoaOpenFile(const char *filePath);

	// system
	FskAPI(const char *) FskCocoaDeviceName(void);
	FskAPI(const char *) FskCocoaUserName(void);
	FskAPI(const char *) FskCocoaSSID(void);
	FskAPI(void) FskCocoaEjectAtPath(const char *path);
	FskAPI(Boolean) FskCocoaIsRemovable(const char *path);
	FskAPI(void) FskCocoaSystemGetVersion(char *version);

	// autoreleasePool
	struct FskCocoaAutoreleasePoolRecord;
	typedef struct FskCocoaAutoreleasePoolRecord *FskCocoaAutoreleasePool;
	FskAPI(FskCocoaAutoreleasePool) FskCocoaAutoreleasePoolNew(void);
	FskAPI(void) FskCocoaAutoreleasePoolDispose(FskCocoaAutoreleasePool pool);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKCOCOASUPPORT__ */

