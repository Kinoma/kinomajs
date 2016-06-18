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
#ifndef __FSKGTKWINDOW__
#define __FSKGTKWINDOW__

#include "Fsk.h"
#include "FskDragDrop.h"
#include <gtk/gtk.h>
#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FskGtkWindowRecord FskGtkWindowRecord;
typedef struct FskGtkWindowRecord *FskGtkWindow;

struct FskGtkWindowRecord {
	FskWindow		owner;
	GtkWidget		*window;		// GTK window
	GtkWidget		*vbox;			// Layout
	GtkWidget		*da;			// draw area - where FSK render result will be present

	// Draw buffer
	GdkPixbuf		*pixbuf;		// Pixel buffer for FSK
	GdkPixbuf		*pixbufDraw;	// Pixel buffer for GTK draw --- so there is one COPY between pixbuf and pixbufDraw
	FskMutex		bufMutex;		// Operations on pixbuf

	// Widgets present queue
	GAsyncQueue		*queue;

	// Menu 
	GtkWidget		*menubar;		// GTK menu widget
	GtkAccelGroup	*accelGroup;
	FskList			menu;			// List for all menuItems, used when need clear menu
	FskMutex		menuMutex;		// Operations on menu items
	Boolean			menuStatus;		// False means menu are not complete, can not do any operation on it.
	FskDragDropTargetProc dropTargetProc;
	
	// Window position and size (Draw area only)
	int				width;
	int				height;
	int				posX;
	int				posY;
};


FskErr	FskGtkWindowCreate(FskWindow fskWindow, UInt32 style, SInt32 width, SInt32 height);
void	FskGtkWindowShow(FskWindow fskWindow, Boolean visible);
void	FskGtkWindowDispose(FskWindow fskWindow);

void	FskGtkWindowSetPos(FskWindow fskWindow, SInt32 x, SInt32 y);
void	FskGtkWindowGetPos(FskWindow fskWindow, SInt32* x, SInt32* y);
void	FskGtkWindowSetTitle(FskWindow fskWindow, const char *title);
void 	FskGtkWindowSetSize(FskGtkWindow gtkWin, UInt32 width, UInt32 height);
void	FskGtkWindowGetSize(FskGtkWindow gtkWin, UInt32 *width, UInt32 *height);

FskErr	FskGtkWindowBitmapNew(FskGtkWindow win, UInt32 width, UInt32 height, FskBitmap *outbits);
FskErr	FskGtkWindowBitmapDispose(FskGtkWindow win);
void	FskGtkWindowUpdateDa(FskGtkWindow win);
void	FskGtkWindowInvalidDaRect(FskGtkWindow win, FskRectangle area);

void	FskGtkWindowSetMenuBar(FskGtkWindow win, GtkWidget* menuBar, int id);
void	FskGtkWindowMenuBarClear(FskGtkWindow win);
void	FskGtkWindowSetMenuItemCallback(FskGtkWindow win, GtkWidget* menuitem, int id);
void	FskGtkWindowSetMenuItemStatus(FskGtkWindow win, int id, char* title, Boolean enabled, Boolean checked);

void	FskGtkWindowSetDialog(FskGtkWindow win,  GtkMessageType type, const char* title, const char* message);

// drag and drop
void FskGtkDragDropWindowRegister(FskWindow fskWindow, FskDragDropTargetProc dropTargetProc);
void FskGtkDragDropWindowUnregister(FskWindow fskWindow);

#ifdef __cplusplus
}
#endif

#endif
