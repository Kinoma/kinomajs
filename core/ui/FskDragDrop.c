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
#include "Fsk.h"
#include "FskUtilities.h"
#include "FskErrors.h"
#include "FskWindow.h"
#include "FskDragDrop.h"

#if TARGET_OS_WIN32
	#include "FskDragDropWin.h"
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
    #include "FskCocoaSupport.h"
#endif

/* --------------------------------------------------------------------------------------------- */

#if (TARGET_OS_WIN32) || (TARGET_OS_MAC && !TARGET_OS_IPHONE)

static FskErr DragDropTargetProc(UInt32 message, UInt32 mouseX, UInt32 mouseY, void *dropData, void *refCon)
{
	FskErr err = kFskErrNone;
	FskWindow window = (FskWindow)refCon;
	FskDragDropFile fileList;
	UInt32 fileListSize;
	FskEvent event;
	FskPointRecord pos;

	pos.x = mouseX;
	pos.y = mouseY;

	switch(message) {
		case kFskDragDropTargetEnterWindow:
			// Calculate the size of the packed file list
			fileList = (FskDragDropFile)dropData;
			fileListSize = 0;
			while (NULL != fileList) {
				fileListSize += FskStrLen(fileList->fullPathName) + 1;
				fileList = fileList->next;
			}

			// Pack the file list into a null separated list
			// Post a "drop target enter window" event
			if (0 != fileListSize) {
				char *packedFileList = 0;
				char *packedFileListWalker;

				++fileListSize;
				FskMemPtrNewClear(fileListSize, &packedFileList);
				*packedFileList = 0;
				packedFileListWalker = packedFileList;
				fileList = (FskDragDropFile)dropData;
				while (NULL != fileList) {
					FskStrCopy(packedFileListWalker, fileList->fullPathName);
					if (NULL != fileList->next) {
						packedFileListWalker += FskStrLen(fileList->fullPathName) + 1;
					}
					fileList = fileList->next;
				}
				if ((err = FskEventNew(&event, kFskEventWindowDragEnter, NULL, kFskEventModifierNotSet)) != kFskErrNone)
					goto bail;
				FskEventParameterAdd(event, kFskEventParameterMouseLocation, sizeof(pos), &pos);
				FskEventParameterAdd(event, kFskEventParameterFileList, fileListSize, packedFileList);
				FskWindowEventSend(window, event);
				FskMemPtrDispose(packedFileList);
			}
			break;

		case kFskDragDropTargetOverWindow:
			// Windows doesn't send WM_MOUSEMOVE events when dragging over a window!!
#if TARGET_OS_WIN32 || TARGET_OS_MAC
			if ((err = FskEventNew(&event, kFskEventMouseMoved, NULL, kFskEventModifierNotSet)) != kFskErrNone)
				goto bail;
			FskEventParameterAdd(event, kFskEventParameterMouseLocation, sizeof(pos), &pos);
			FskWindowEventSend(window, event);
#endif
			break;

		case kFskDragDropTargetLeaveWindow:
			if ((err = FskEventNew(&event, kFskEventWindowDragLeave, NULL, kFskEventModifierNotSet)) != kFskErrNone)
				goto bail;
			FskWindowEventSend(window, event);
			break;

		case kFskDragDropTargetDropInWindow:
			if ((err = FskEventNew(&event, kFskEventMouseUp, NULL, kFskEventModifierNotSet)) != kFskErrNone)
				goto bail;
			FskEventParameterAdd(event, kFskEventParameterMouseLocation, sizeof(pos), &pos);
			FskWindowEventSend(window, event);
			break;
	}

bail:
	return err;
}

#endif

/* --------------------------------------------------------------------------------------------- */

FskErr FskDragDropTargetRegisterWindow(FskWindow window)
{
	FskErr err = kFskErrNone;

#if TARGET_OS_WIN32
	err = FskDragDropTargetWinRegisterNativeWindow(window, DragDropTargetProc, (void*)window);
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
    FskCocoaDragDropWindowRegister(window, DragDropTargetProc);
#else
	err = kFskErrUnimplemented;
#endif

	return err;
}

/* --------------------------------------------------------------------------------------------- */

FskErr FskDragDropTargetUnregisterWindow(FskWindow window)
{
	FskErr err = kFskErrNone;

#if TARGET_OS_WIN32
	FskDragDropTargetWinUnregisterNativeWindow(window);
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
    FskCocoaDragDropWindowUnregister(window);
#else
	err = kFskErrUnimplemented;
#endif

	return err;
}

/* --------------------------------------------------------------------------------------------- */

FskErr FskDragDropTargetUseNativeProxy(FskWindow window, Boolean useNativeProxy)
{
	FskErr err = kFskErrNone;

#if TARGET_OS_WIN32
	FskDragDropTargetWinUseNativeProxy(window, useNativeProxy);
#else
	err = kFskErrUnimplemented;
#endif

	return err;
}

/* --------------------------------------------------------------------------------------------- */

void FskDragDropTargetDropResult(FskWindow window, Boolean result)
{
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
    FskCocoaDragDropWindowResult(window, result);
#else
	// On Windows, we don't need to let the native implementation know if the drop succeeded or not
#endif
}

/* --------------------------------------------------------------------------------------------- */
