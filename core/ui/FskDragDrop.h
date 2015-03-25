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
#ifndef __FSK_DRAGDROP__
#define __FSK_DRAGDROP__

#include "Fsk.h"
#include "FskWindow.h"

#ifdef __cplusplus
extern "C" {
#endif

// Messages passed to the client
enum {
	kFskDragDropTargetEnterWindow = 0,
	kFskDragDropTargetOverWindow,
	kFskDragDropTargetLeaveWindow,
	kFskDragDropTargetDropInWindow
};

struct FskDragDropFileRecord {
	struct FskDragDropFileRecord  *next;

	char *fullPathName;
};
typedef struct FskDragDropFileRecord FskDragDropFileRecord;
typedef FskDragDropFileRecord *FskDragDropFile;

typedef FskErr (*FskDragDropTargetProc)(UInt32 message, UInt32 mouseX, UInt32 mouseY, void *dropData, void *refCon);

FskErr FskDragDropTargetRegisterWindow(FskWindow window);
FskErr FskDragDropTargetUnregisterWindow(FskWindow window);

void FskDragDropTargetDropResult(FskWindow window, Boolean success);

FskErr FskDragDropTargetUseNativeProxy(FskWindow window, Boolean useNativeProxy);


#ifdef __cplusplus
}
#endif

#endif

