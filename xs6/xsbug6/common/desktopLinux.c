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

#include "kpr.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHTTPClient.h"
#include "kprShell.h"
#include "kprURL.h"

#include <gtk/gtk.h>
#include "FskGtkWindow.h"
#include "FskNetInterface.h"

void KPR_Files_toPath(xsMachine *the)
{
	char* path;
	xsThrowIfFskErr(KprURLToPath(xsToString(xsArg(0)), &path));
	xsResult = xsString(path);
	FskMemPtrDispose(path);
}

void KPR_Files_toURI(xsMachine *the)
{
	char* url;
	xsThrowIfFskErr(KprPathToURL(xsToString(xsArg(0)), &url));
	xsResult = xsString(url);
	FskMemPtrDispose(url);
}

void KPR_system_gotoFront(xsMachine *the)
{
}

void KPR_system_getClipboardText(xsMachine* the)
{
	gdk_threads_enter();
	GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	if (gtk_clipboard_wait_is_text_available(clipboard)) {
		char *utf8Ptr = gtk_clipboard_wait_for_text(clipboard);
		if (utf8Ptr) {
			xsResult = xsString(utf8Ptr);
			g_free(utf8Ptr);
			goto bail;
		}
	}
	xsResult = xsString("");
bail:
	gdk_threads_leave();
}

void KPR_system_setClipboardText(xsMachine* the)
{
	gdk_threads_enter();
	GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	if (xsToInteger(xsArgc) > 0) {
		char* utf8Ptr = xsToString(xsArg(0));
		gtk_clipboard_set_text(clipboard, utf8Ptr, FskStrLen(utf8Ptr));
	}
	else
		gtk_clipboard_set_text(clipboard, "", 0);
	gdk_threads_leave();
}

static xsSlot callback;

void KPR_system_alert(xsMachine* the)
{
	int argc = xsToInteger(xsArgc);
	KprShell self = gShell;
	xsStringValue string;
	xsStringValue prompt;
	GtkWidget *dialog;
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	gint res;
	
	if ((argc > 1) && xsTest(xsArg(1)))
		callback = xsArg(1);
	else
		callback = xsNull;
	xsRemember(callback);
	
	if ((argc > 0) && xsTest(xsArg(0))) {
		UInt32 type = GTK_MESSAGE_OTHER;
		if (xsFindString(xsArg(0), xsID_type, &string)) {
			if (!FskStrCompare(string, "about"))
				type = GTK_MESSAGE_WARNING;
			else if (!FskStrCompare(string, "stop"))
				type = GTK_MESSAGE_QUESTION;
			else if (!FskStrCompare(string, "note"))
				type = GTK_MESSAGE_INFO;
		}
		if (xsFindString(xsArg(0), xsID_prompt, &string))
			prompt = string;

		dialog = gtk_message_dialog_new(
			(GtkWindow*)(((FskGtkWindow)self->window->gtkWin)->window),
			flags,
			type,
			GTK_BUTTONS_NONE,
			prompt,
			NULL);
	
		if (xsFindString(xsArg(0), xsID_info, &string))
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), string, NULL);
		if (xsFindResult(xsArg(0), xsID_buttons)) {
			if (xsIsInstanceOf(xsResult, xsArrayPrototype)) {
				xsIntegerValue c = xsToInteger(xsGet(xsResult, xsID_length)), i;
				for (i = 0; i < c; i++) {
					string = xsToString(xsGet(xsResult, i));
					gtk_dialog_add_button(GTK_DIALOG(dialog), string, i);
				}
			}
		}
		(*self->handler)(NULL, kFskEventWindowDeactivated, NULL, self);
		res = gtk_dialog_run(GTK_DIALOG(dialog));
		(*self->handler)(NULL, kFskEventWindowActivated, NULL, self);
		gtk_widget_destroy(dialog);

		xsBeginHostSandboxCode(self->the, self->code);
		{
			xsVars(1);
			{
				xsTry {
					xsResult = xsAccess(callback);
					if (xsTest(xsResult)) {
						xsVar(0) = (res == 0) ? xsTrue : (res == 1) ? xsUndefined : xsFalse;
						(void)xsCallFunction1(xsResult, xsNull, xsVar(0));
					}
				}
				xsCatch {
				}
			}
		}
		xsEndHostSandboxCode();
	}
	xsForget(callback);
	callback = xsNull;
}

static void KPR_system_open(xsMachine* the, xsBooleanValue flag)
{
	int argc = xsToInteger(xsArgc);
	KprShell self = gShell;
	xsStringValue string;

	GtkWidget *dialog;
	GtkFileChooserAction action = flag ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER : GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;
	char *promptString = NULL;
	char *msgString = NULL;
	char *uriString = NULL;

	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_fileType, &string))
			//[openPanel setAllowedFileTypes: [NSArray arrayWithObject:[NSString stringWithUTF8String:string]]];
			xsDebugger();
		if (xsFindString(xsArg(0), xsID_message, &string))
			msgString = FskStrDoCopy(string);
		if (xsFindString(xsArg(0), xsID_name, &string))
			//[openPanel setDelegate:[[KPR_system_open_delegate alloc] initWith:[NSString stringWithUTF8String:string]]];
			xsDebugger();
		if (xsFindString(xsArg(0), xsID_prompt, &string))
			promptString = FskStrDoCopy(string);
		if (xsFindString(xsArg(0), xsID_url, &string))
			uriString = FskStrDoCopy(string);
	}

	dialog = gtk_file_chooser_dialog_new (msgString, (GtkWindow*)(((FskGtkWindow)self->window->gtkWin)->window), action,
		"Cancel", GTK_RESPONSE_CANCEL,
		promptString, GTK_RESPONSE_ACCEPT,
		NULL);

	if (uriString)
		gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), uriString);

	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(dialog), FALSE);

	if ((argc > 1) && xsTest(xsArg(1)))
		callback = xsArg(1);
	else
		callback = xsNull;
	xsRemember(callback);
	(*self->handler)(NULL, kFskEventWindowDeactivated, NULL, self);

	res = gtk_dialog_run (GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	{
		char *uri;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		uri = gtk_file_chooser_get_uri(chooser);
		
		(*self->handler)(NULL, kFskEventWindowActivated, NULL, self);
		xsBeginHostSandboxCode(self->the, self->code);
		{
			xsVars(1);
			{
				xsTry {
					xsResult = xsAccess(callback);
					xsForget(callback);
					callback = xsNull;
					if (xsTest(xsResult)) {
						if (uri) {
							xsVar(0) = xsString((xsStringValue)uri);
							if (flag)
								xsVar(0) = xsCall1(xsVar(0), xsID_concat, xsString("/"));
						}
						(void)xsCallFunction1(xsResult, xsNull, xsVar(0));
					}
				}
				xsCatch {
				}
			}
		}
		xsEndHostSandboxCode();

		g_free(uri);
	}

	gtk_widget_destroy(dialog);

	FskMemPtrDispose(msgString);
	FskMemPtrDispose(promptString);
	FskMemPtrDispose(uriString);
}

void KPR_system_openDirectory(xsMachine* the)
{
	KPR_system_open(the, 1);
}

void KPR_system_openFile(xsMachine* the)
{
	KPR_system_open(the, 0);
}

void KPR_system_save(xsMachine* the, xsBooleanValue flag)
{
	xsIntegerValue argc = xsToInteger(xsArgc);
	KprShell self = gShell;
	xsStringValue string;

	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	gint res;
	char *promptString = NULL;
	char *msgString = NULL;
	char *nameString = NULL;
	char *uriString = NULL;
	
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_name, &string))
			nameString = FskStrDoCopy(string);
		if (xsFindString(xsArg(0), xsID_prompt, &string))
			promptString = FskStrDoCopy(string);
		if (xsFindString(xsArg(0), xsID_url, &string))
			uriString = FskStrDoCopy(string);
	}

	if ((argc > 1) && xsTest(xsArg(1)))
		callback = xsArg(1);
	else
		callback = xsNull;
	xsRemember(callback);

	dialog = gtk_file_chooser_dialog_new (msgString, (GtkWindow*)(((FskGtkWindow)self->window->gtkWin)->window), action,
			"Cancel", GTK_RESPONSE_CANCEL,
			promptString, GTK_RESPONSE_ACCEPT,
			NULL);

	if (uriString)
		gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), string);

	if (nameString)
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(dialog), nameString);

	(*self->handler)(NULL, kFskEventWindowDeactivated, NULL, self);

	res = gtk_dialog_run (GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	{
		char *uri;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		uri = gtk_file_chooser_get_uri(chooser);
		printf("KPR_system_save %s\n", uri);

		(*self->handler)(NULL, kFskEventWindowActivated, NULL, self);
		xsBeginHostSandboxCode(self->the, self->code);
		{
			xsVars(1);
			{
				xsTry {
					xsResult = xsAccess(callback);
					xsForget(callback);
					callback = xsNull;
					if (xsTest(xsResult)) {
						if (uri)
							xsVar(0) = xsString((xsStringValue)uri);
						(void)xsCallFunction1(xsResult, xsNull, xsVar(0));
					}
				}
				xsCatch {
				}
			}
		}
		xsEndHostSandboxCode();

		g_free(uri);
	}

	gtk_widget_destroy(dialog);

	FskMemPtrDispose(nameString);
	FskMemPtrDispose(msgString);
	FskMemPtrDispose(promptString);
	FskMemPtrDispose(uriString);
}


void KPR_system_saveDirectory(xsMachine* the)
{
	KPR_system_save(the, 1);
}

void KPR_system_saveFile(xsMachine* the)
{
	KPR_system_save(the, 0);
}
