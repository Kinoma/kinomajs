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
#define __FSKWINDOW_PRIV__
#define __FSKBITMAP_PRIV__

#include "FskWindow.h"
#include "FskGtkWindow.h"

// This value it used to set a threshold to frame interval (in us).
// Now the value is ALMOST same as 'updateInterval' (20000)
#define ASYNC_TIMEOUT 25000

////////////// OVERVIEW /////////////////
/*
 * There are some *things* in GTK window: initialization, window itself, menu, inputs and draw-area
 * 1. Initialization: Do initialization (Include register signals) in 'gtk_thread' (which run gtk_main()).
 *    And receive all signals and do all callbacks (Include draw to GDK) in same thread
 * 2. Window itself: position and size
 * 3. The inputs(mouse, keyboard) are captured by GTK main thread and will transfer to FSK thread.
 * 4. Menu is initialized and trigged by FSK thread (bind to main windows).
 * 5. Draw area: FSK thread and GTK main thread will exchange data through a bitmap (member name: da) --- need a mutex
 *    The bitmap (is allocated in GTK main thread and will be released by it.
 *	  FSK thread will use it to store draw result only.
 *    TODO: current implementation is NOT good for performance:
 *		A. there is one copy operation!
 *		B. the mutex cause extra wait!
 * 6. Please note multi-threads: some function run in gtk_thread, others in main thread
 * 7. multi-threads on GTK+: callbacks are not need add thread_calls, but idle and timeout function need -- for example idle_func
*/

////////////  Menu bar and Items ///////////
typedef struct menuItemsRecord menuItemsRecord;
typedef struct menuItemsRecord *menuItems;
typedef struct menuBarsRecord  menuBarsRecord;
typedef struct menuBarsRecord  *menuBars;

struct menuItemsRecord {
	menuItems		next;
	FskGtkWindow	win;
	GtkWidget		*item;
	int 			id;
};

struct menuBarsRecord {
	menuBars		next;
	GtkWidget		*item;
	FskList			menulist;
	int 			id;
};

///   gtk_threads functions start ///
////////////// Event Handlers /////////////////
/*
 * Note: event handler for GTK return:
 * 	FALSE:	means continue process by GTK system (default operation)
 * 	TRUE :	means we have done this event
*/
static gboolean app_quit(GtkWidget *widget, gpointer data)
{
	FskGtkWindow win = (FskGtkWindow)data;

	gdk_threads_enter();
	FskWindowGtkClose(win->owner);
	gdk_threads_leave();
	/* we need GTK continue this event to release window memory etc */
	return FALSE;
}

// TODO: performance should be tuned later: frame rate and consistent
static gboolean idle_func(gpointer data)
{
	FskGtkWindow win = (FskGtkWindow)data;
	gpointer thread_data = g_async_queue_timeout_pop(win->queue, ASYNC_TIMEOUT);

	if(win->menuStatus && thread_data) {
		if(win == thread_data) {
			FskMutexAcquire(win->bufMutex);
			if(GTK_IS_WIDGET(win->window))
				gtk_widget_queue_draw(win->window);
			FskMutexRelease(win->bufMutex);
		}
		else {
			//Show the dialog
			GtkWidget *dialog = thread_data;
			gtk_dialog_run(GTK_DIALOG(dialog));	// Model window
			gtk_widget_destroy(dialog);
		}
	}

	return TRUE;	//TRUE means will process again (loop)
}

static gboolean configure_event_win(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	FskGtkWindow win = (FskGtkWindow)data;

	win->posX = event->x;
	win->posY = event->y;
	return FALSE;	// Must return FALSE to make the event continue.
}

// Event handler: da->draw
static gboolean draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	FskGtkWindow win = (FskGtkWindow)data;

	FskMutexAcquire(win->bufMutex);
	if(win->pixbufDraw)
		gdk_cairo_set_source_pixbuf(cr, win->pixbufDraw, 0, 0);
	cairo_paint(cr);
	FskMutexRelease(win->bufMutex);

	return TRUE;
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	FskEvent fskEvent;
	int x = (int)event->x;
	int y = (int)event->y;
	FskGtkWindow win = (FskGtkWindow)data;

	if(win->menuStatus && (kFskErrNone == FskEventNew(&fskEvent, kFskEventMouseDown, NULL, kFskEventModifierNotSet))) {
		FskPointAndTicksRecord pat;
		pat.pt.x = x;
		pat.pt.y = y;
		pat.index = event->button - 1; //Different with FSK button index
		pat.ticks = event->time;
		FskEventParameterAdd(fskEvent, kFskEventParameterMouseLocation, sizeof(pat), &pat);
		FskWindowEventQueue(win->owner, fskEvent);
	}
	return TRUE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	FskEvent fskEvent;
	int x = (int)event->x;
	int y = (int)event->y;
	FskGtkWindow win = (FskGtkWindow)data;

	if(win->menuStatus && (kFskErrNone == FskEventNew(&fskEvent, kFskEventMouseUp, NULL, kFskEventModifierNotSet))) {
		FskPointAndTicksRecord pat;
		pat.pt.x = x;
		pat.pt.y = y;
		pat.index = event->button - 1;
		pat.ticks = event->time;
		FskEventParameterAdd(fskEvent, kFskEventParameterMouseLocation, sizeof(pat), &pat);
		FskWindowEventQueue(win->owner, fskEvent);
	}
	return TRUE;
}

// TODO: here are one issue, for example, input: SHIFT+A in KPR app
//  So the event handler should be modified
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	FskEvent fskEvent;
	FskGtkWindow win = (FskGtkWindow)data;
	if(win->menuStatus && (kFskErrNone == FskEventNew(&fskEvent, kFskEventKeyDown, NULL, event->is_modifier))) {
		char key[2];

		if(event->is_modifier) {
			UInt32 funcKey = event->keyval & 0xFFFF;
			FskEventParameterAdd(fskEvent, kFskEventParameterFunctionKey, sizeof(funcKey), &funcKey);
		} else {
			if(event->keyval < 0xFF) {
				key[0] = (char)event->keyval;
			} else {
				switch(event->keyval) {
					case GDK_KEY_Left:
						key[0] = 28;
						break;
					case GDK_KEY_Right:
						key[0] = 29;
						break;
					case GDK_KEY_Up:
						key[0] = 30;
						break;
					case GDK_KEY_Down:
						key[0] = 31;
						break;
					default:
						key[0] = (char)event->keyval;
						break;
				}
			}

			key[1] = 0;
			FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, 2, key);
		}
		FskWindowEventQueue(win->owner, fskEvent);
	}

	return FALSE;
}

static gboolean on_key_release(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	FskEvent fskEvent;
	FskGtkWindow win = (FskGtkWindow)data;
	if(win->menuStatus && (kFskErrNone == FskEventNew(&fskEvent, kFskEventKeyUp, NULL, event->is_modifier))) {
		char key[2];

		if(event->is_modifier) {
			UInt32 funcKey = event->keyval & 0xFFFF;
			FskEventParameterAdd(fskEvent, kFskEventParameterFunctionKey, sizeof(funcKey), &funcKey);
		} else {
			if(event->keyval < 0xFF) {
				key[0] = (char)event->keyval;
			} else {
				switch(event->keyval) {
					case GDK_KEY_Left:
						key[0] = 28;
						break;
					case GDK_KEY_Right:
						key[0] = 29;
						break;
					case GDK_KEY_Up:
						key[0] = 30;
						break;
					case GDK_KEY_Down:
						key[0] = 31;
						break;
					default:
						key[0] = (char)event->keyval;
						break;
				}
			}

			key[1] = 0;
			FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, 2, key);
		}
		FskWindowEventQueue(win->owner, fskEvent);
	}
	return FALSE;
}

static gboolean motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	FskEvent fskEvent;
	FskGtkWindow win = (FskGtkWindow)data;
	UInt32	index = 0;

	int x = (int)event->x;
	int y = (int)event->y;
	if(win->menuStatus && (kFskErrNone == FskEventNew(&fskEvent, kFskEventMouseMoved, NULL, kFskEventModifierNotSet))) {
		FskPointAndTicksRecord pat;
		pat.pt.x = x;
		pat.pt.y = y;
		pat.index = 0;
		pat.ticks = event->time;
		FskEventParameterAdd(fskEvent, kFskEventParameterMouseLocation, sizeof(pat), &pat);
		FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(index), &index);
		FskWindowEventQueue(win->owner, fskEvent);
	}

	return TRUE;
}
///   gtk_threads functions End ///

// The following two callbacks can be called in both threads!!!
// Event handler for window size/position changed
// main thread: called from 'FskGtkWindowShow' and then trig event in GTK internally.
// gtk_thread: called from gtk_main (event)
static gboolean configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	FskGtkWindow win = (FskGtkWindow)data;

	// TODO: win->menuStatus && 
	gdk_threads_enter();
	if(((event->width != win->width) || (event->height != win->height))) {
		win->width = event->width;
		win->height = event->height;

		FskEvent fskEvent;
		if(kFskErrNone == FskEventNew(&fskEvent, kFskEventWindowBeforeResize, NULL, kFskEventModifierNotSet)) {
			FskWindowEventQueue(win->owner, fskEvent);
		}
	}
	gdk_threads_leave();

	return TRUE;
}
// main thread: call from FskGtkWindowSetMenuItemStatus
// gtk_thread: gtk_main
static void menu_select_cb(GtkWidget *widget, gpointer data)
{
	FskEvent fskEvent;
	menuItems selected = (menuItems)data;
	FskGtkWindow win;
	menuBars entryBar;
	menuItems items = NULL;
	int groupID;

	//Have to check firstly since radio item will emit two signals
	if(selected == NULL)
		return;
	win = selected->win;
	if(win->menuStatus == false)
		return;
	gdk_threads_enter();
	if(GTK_IS_CHECK_MENU_ITEM(selected->item) && !gtk_check_menu_item_get_active((GtkCheckMenuItem*)selected->item))
		return;
	if(kFskErrNone == FskEventNew(&fskEvent, kFskEventMenuCommand, NULL, kFskEventModifierNotSet)) {
		SInt32 command = selected->id;
		FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(command), &command);
		FskWindowEventQueue(selected->win->owner, fskEvent);
	}

	// FIXME: actually, we should do this operations in event like menubar popup, but I do not find suitable event till now
	// So just put them here!!!
	groupID = selected->id & 0xFF00;
	entryBar = win->menu;

	while(entryBar) {
		if(entryBar->id == groupID) {
			break;
		}

		entryBar = entryBar->next;
	}
	if(entryBar) {
		for(items = (menuItems)entryBar->menulist; items != NULL; items = items->next) {
			int id = items->id;
			if(kFskErrNone == FskEventNew(&fskEvent, kFskEventMenuStatus, NULL, kFskEventModifierNotSet)) {
				FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(id), &id);
				FskWindowEventSend(win->owner, fskEvent);
			}
		}
	}
	gdk_threads_leave();
}


////////////// Main and Init /////////////////
static void FskGtkWindowLoop(void *refcon)
{
	FskWindow 		fskWindow = (FskWindow)refcon;
	FskGtkWindow	gtkWin = (FskGtkWindow)fskWindow->gtkWin;
	GtkWidget		*win;
	GdkPixbuf		*iconBuf;

	// Start GTK special initialization
	XInitThreads(); //fix a xcb issue happened when using multithread gtk, see http://stackoverflow.com/a/18690540/472927
	gtk_init(NULL, NULL);

	gtkWin->owner = fskWindow;
	gtkWin->window = win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size((GtkWindow*)win, 800, 600);

	//	Set App Icon
	char *iconPath = FskStrDoCat(FskGetApplicationPath(), "fsk.png");
	iconBuf = gdk_pixbuf_new_from_file(iconPath, NULL);
	if(iconBuf != NULL) {
		gtk_window_set_icon(GTK_WINDOW(win), iconBuf);
		g_object_unref(G_OBJECT(iconBuf));
	}
	FskMemPtrDispose(iconPath);

	gtkWin->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(win), gtkWin->vbox);
	gtk_widget_show(gtkWin->vbox);

	gtkWin->menubar = gtk_menu_bar_new();
	gtk_widget_show(gtkWin->menubar);

	gtkWin->da = gtk_drawing_area_new();

	gtk_widget_show(gtkWin->da);

	gtkWin->accelGroup = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(win), gtkWin->accelGroup);

	FskMutexNew(&gtkWin->bufMutex, "drawAreaBuffer");
	FskMutexNew(&gtkWin->menuMutex, "GTKMenuItems");
	gtkWin->menuStatus = true;

	gtkWin->queue = g_async_queue_new();
	gtk_widget_add_events(GTK_WIDGET(win), GDK_EXPOSURE_MASK | GDK_KEY_PRESS_MASK);
	gtk_widget_add_events(GTK_WIDGET(gtkWin->da), GDK_ALL_EVENTS_MASK);

	g_signal_connect(win, "destroy", 			G_CALLBACK(app_quit), gtkWin);
	g_signal_connect(win, "key-press-event", 	G_CALLBACK(on_key_press), gtkWin);
	g_signal_connect(win, "key-release-event", 	G_CALLBACK(on_key_release), gtkWin);
	g_signal_connect(win, "configure-event", 	G_CALLBACK(configure_event_win), gtkWin);

	// If want to resize draw-area, it will be the event receiver.
	g_signal_connect(gtkWin->da, "configure-event", 	G_CALLBACK(configure_event_cb), gtkWin);
	g_signal_connect(gtkWin->da, "draw", 				G_CALLBACK(draw_callback), gtkWin);
	g_signal_connect(gtkWin->da, "button-press-event", 	G_CALLBACK(on_button_press), gtkWin);
	g_signal_connect(gtkWin->da, "button-release-event",G_CALLBACK(on_button_release), gtkWin);
	g_signal_connect(gtkWin->da, "motion-notify-event", G_CALLBACK(motion_notify), gtkWin);

	gtk_box_pack_start(GTK_BOX(gtkWin->vbox), gtkWin->menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gtkWin->vbox), gtkWin->da, TRUE, TRUE, 0); // Set all to true!

	gdk_threads_add_idle(idle_func, gtkWin);

	FskThreadInitializationComplete(FskThreadGetCurrent());

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
}

FskErr FskGtkWindowCreate(FskWindow fskWindow, UInt32 style, SInt32 width, SInt32 height)
{
	FskErr			err = kFskErrNone;
	FskGtkWindow	gtkWin;

	if(fskWindow->gtkWin == NULL) {
		err = FskMemPtrNewClear(sizeof(FskGtkWindowRecord), &(fskWindow->gtkWin));
		BAIL_IF_ERR(err);
	}

	gtkWin = (FskGtkWindow)fskWindow->gtkWin;
	gtkWin->width = width;
	gtkWin->height = height;

	//Create gtk thread: initialization and main loop for GTK
	FskThreadCreate(&fskWindow->gtkThread, FskGtkWindowLoop, kFskThreadFlagsJoinable | kFskThreadFlagsWaitForInit, fskWindow, "gtk_thread");

bail:
	return err;
}

void FskGtkWindowShow(FskWindow fskWindow, Boolean visible)
{
	FskGtkWindow gtkWin;

	if(fskWindow == NULL)
		return;

	if(fskWindow->gtkWin == NULL)
		return;

	gtkWin = (FskGtkWindow)fskWindow->gtkWin;
	if(gtkWin->window == NULL)
		return;

	gdk_threads_enter();
	if( GTK_IS_WIDGET(gtkWin->window))
		gtk_widget_set_visible(gtkWin->window, (visible? TRUE: FALSE));

	gdk_threads_leave();
}

void FskGtkWindowDispose(FskWindow fskWindow)
{
	FskGtkWindow gtkWin = (FskGtkWindow)fskWindow->gtkWin;

	gtk_main_quit();
	FskThreadJoin(fskWindow->gtkThread);
	FskMutexDispose(gtkWin->bufMutex);
	FskMutexDispose(gtkWin->menuMutex);
}

////////////// APIs used by FSK  /////////////////
// All these APIs are called from main thread, so should add gdk_threads_xxx()
void FskGtkWindowSetPos(FskWindow fskWindow, SInt32 x, SInt32 y)
{
	FskGtkWindow gtkWin;

	if( (fskWindow == NULL) || (fskWindow->gtkWin == NULL))
		return;

	gtkWin = (FskGtkWindow)fskWindow->gtkWin;
	if(gtkWin->window == NULL)
		return;
	gdk_threads_enter();
	gtk_window_move(GTK_WINDOW(gtkWin->window), x, y);
	gdk_threads_leave();
}

void FskGtkWindowGetPos(FskWindow fskWindow, SInt32* x, SInt32* y)
{
	FskGtkWindow gtkWin;

	if( (fskWindow == NULL) || (fskWindow->gtkWin == NULL))
		return;

	gtkWin = (FskGtkWindow)fskWindow->gtkWin;
	*x = gtkWin->posX;
	*y = gtkWin->posY;
}

void FskGtkWindowSetSize(FskGtkWindow gtkWin, UInt32 width, UInt32 height)
{
	if( (gtkWin == NULL) || (gtkWin->window == NULL))
		return;

	if((width == 0) || (height == 0)) {
		width = 800;
		height = 600;
	}

	gdk_threads_enter();
	gtk_window_resize(GTK_WINDOW(gtkWin->window), (int)width, (gint)height);
	gtkWin->width = width;
	gtkWin->height = height;
	gdk_threads_leave();
}

void FskGtkWindowGetSize(FskGtkWindow gtkWin, UInt32 *width, UInt32 *height)
{
	*width	= gtkWin->width;
	*height	= gtkWin->height;
}

void FskGtkWindowSetTitle(FskWindow fskWindow, const char *title)
{
	FskGtkWindow gtkWin;

	if( (fskWindow == NULL) || (fskWindow->gtkWin == NULL))
		return;

	gtkWin = (FskGtkWindow)fskWindow->gtkWin;
	if(gtkWin->window == NULL)
		return;
	gdk_threads_enter();
	gtk_window_set_title(GTK_WINDOW(gtkWin->window), title);
	gdk_threads_leave();
}

// This function will be called at a rate about (1000/updateInterval)
void FskGtkWindowUpdateDa(FskGtkWindow win)
{
	if(win->menuStatus == false)
		return;

	FskMutexAcquire(win->bufMutex);
	if(win->pixbufDraw)
		g_object_unref(win->pixbufDraw);

	win->pixbufDraw = gdk_pixbuf_copy(win->pixbuf);
	g_async_queue_push(win->queue, win);
	FskMutexRelease(win->bufMutex);

#if 0
	cairo_t *cr;
	cr = gdk_cairo_create (gtk_widget_get_window(win->da));
	gdk_cairo_set_source_pixbuf(cr, win->pixbufDraw, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);
	gtk_widget_queue_draw(win->window);
#endif
}

void FskGtkWindowInvalidDaRect(FskGtkWindow win, FskRectangle area)
{
	if(win->menuStatus == false)
		return;

	FskMutexAcquire(win->bufMutex);
	if(win->pixbufDraw)
		g_object_unref(win->pixbufDraw);

	win->pixbufDraw = gdk_pixbuf_copy(win->pixbuf);
	FskMutexRelease(win->bufMutex);

	g_async_queue_push(win->queue, win);
}

FskErr FskGtkWindowBitmapNew(FskGtkWindow win, UInt32 width, UInt32 height, FskBitmap *outBits)
{
	FskErr err = kFskErrNone;

	//allocate a pixbuf for drawing area
	win->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, width, height);
	//win->pixbuf = gdk_pixbuf_get_from_window(gtk_widget_get_window(win->window), 0, 0, width, height);

	if(win->pixbuf) {
		guchar* buf		= gdk_pixbuf_get_pixels(win->pixbuf);
		int rowBytes	= gdk_pixbuf_get_rowstride(win->pixbuf);
		//int depth		= gdk_pixbuf_get_byte_length(win->pixbuf);

		err = FskBitmapNewWrapper(width, height, kFskBitmapFormat32RGBA, 32, (char*)buf, rowBytes, outBits);
	}
	else
		err = kFskErrMemFull;

	return err;
}

FskErr FskGtkWindowBitmapDispose(FskGtkWindow win)
{
	g_object_unref(win->pixbuf);
	return kFskErrNone;
}

////////////// Menu processing /////////////////
void FskGtkWindowSetMenuBar(FskGtkWindow win, GtkWidget* menuBar, int id)
{
	menuBars entryBar = NULL;

	gtk_menu_shell_append(GTK_MENU_SHELL(win->menubar), menuBar);

	if(kFskErrNone == FskMemPtrNewClear(sizeof(menuBarsRecord), (FskMemPtr*)&entryBar)) {
		entryBar->item 	= menuBar;
		entryBar->id   	= id & 0xFF00;		// id for the whole group
		entryBar->menulist = NULL;

		FskListAppend(&win->menu, entryBar);
	}
}

void FskGtkWindowMenuBarClear(FskGtkWindow win)
{
	menuBars entryBar = win->menu;
	menuItems submenu = NULL;

	// Remove pending events
	while(gdk_events_pending()) {
		GdkEvent *event = gdk_event_get();
		if(event != NULL) {
			fprintf(stderr, "There are pending events, removed\n");
			gdk_event_free(event);
		}
	}
	
	// Remove IDs
	entryBar = FskListRemoveFirst(&win->menu);
	while(entryBar) {
		// Remove menuItems
		submenu = FskListRemoveFirst(&entryBar->menulist);
		while(submenu) {
			FskMemPtrDispose(submenu);
			submenu = FskListRemoveFirst(&entryBar->menulist);
		}

		entryBar->menulist = NULL;
		FskMemPtrDispose(entryBar);

		// Next
		entryBar = FskListRemoveFirst(&win->menu);
	}

	// Remove menuBar
	// TODO? disable GDK events when do such clean?
	{
		GList *children, *iter;
		children = gtk_container_get_children(GTK_CONTAINER(win->menubar));
		for(iter = children; iter != NULL; iter = g_list_next(iter))
  			gtk_widget_destroy(GTK_WIDGET(iter->data));
		g_list_free(children);
	}
	gtk_widget_hide(win->menubar);

	win->menu = NULL;
}

void FskGtkWindowSetMenuItemCallback(FskGtkWindow win, GtkWidget* menuItem, int id)
{
	FskEvent fskEvent;
	menuItems entry = NULL;

	if(kFskErrNone == FskMemPtrNewClear(sizeof(menuItemsRecord), (FskMemPtr*)&entry)) {
		menuBars entryBar = NULL;
		entry->win = win;
		entry->id  = id;
		entry->item= menuItem;

		FskMutexAcquire(win->menuMutex);
		entryBar = win->menu;
		while(entryBar) {
			if(entryBar->id == (id & 0xFF00)) {
				break;
			}
			entryBar = entryBar->next;
		}
		if(entryBar) {
			FskListAppend(&entryBar->menulist, entry);

			// Update menuItem immediately (enabled? checked?)
			if((kFskErrNone == FskEventNew(&fskEvent, kFskEventMenuStatus, NULL, kFskEventModifierNotSet))) {
				FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(id), &id);
				FskWindowEventSend(win->owner, fskEvent);
			}
			g_signal_connect(menuItem, "activate", G_CALLBACK(menu_select_cb), (gpointer)entry);
		}
		FskMutexRelease(win->menuMutex);
	}
}

// Update each menuItem according to status: enabled, checked.
void FskGtkWindowSetMenuItemStatus(FskGtkWindow win, int id, Boolean enabled, Boolean checked)
{
	menuBars entryBar = win->menu;
	menuItems items = NULL;
	//if(win->menuStatus == false)
	//	return;
	while(entryBar) {
		if(entryBar->id == (id & 0xFF00)) {
			break;
		}
		entryBar = entryBar->next;
	}
	if(entryBar) {
		for(items = (menuItems)entryBar->menulist; items != NULL; items = items->next) {
			if(items && (items->id == id)) {
				gdk_threads_enter();
				gtk_widget_set_sensitive((GtkWidget*)items->item, enabled);
				gtk_check_menu_item_set_active((GtkCheckMenuItem*)items->item, checked);
				gdk_threads_leave();
				break;
			}
		}
	}
}

////////////// Dialog /////////////////
void FskGtkWindowSetDialog(FskGtkWindow win, GtkMessageType type, const char* title, const char* message)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(win->window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, type, GTK_BUTTONS_OK, "%s", title);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", message);
	g_async_queue_push(win->queue, dialog);
}

/*
  Design consideration:
   1. GTK+, however, is not thread safe. You should only use GTK+ and GDK from the thread gtk_init() and gtk_main() were called on. This is usually referred to as the “main thread” -- our *gtk_thread*
*/

