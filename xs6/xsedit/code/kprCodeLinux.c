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
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include "kpr.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHTTPClient.h"
#include "kprShell.h"
#include "kprURL.h"

#include <spawn.h>
#include <sys/wait.h>

#include <gtk/gtk.h>

#include "FskNetInterface.h"

// cannot include both net/if.h and linux/if.h so defined here
extern char *if_indextoname (unsigned int __ifindex, char *__ifname);
#define IF_NAMESIZE 16

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
			NULL,
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

	dialog = gtk_file_chooser_dialog_new (msgString, NULL, action, 
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

	dialog = gtk_file_chooser_dialog_new (msgString, NULL, action, 
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

void KPR_system_beginModal(xsMachine* the)
{
	KprShell self = gShell;
	(*self->handler)(NULL, kFskEventWindowDeactivated, NULL, self);
}

void KPR_system_endModal(xsMachine* the)
{
	KprShell self = gShell;
	(*self->handler)(NULL, kFskEventWindowActivated, NULL, self);
}

// @@ to be tested: no WiFi Ubuntu here, only VM
void KPR_system_getWifiInfo(xsMachine* the)
{
	FskNetInterfaceRecord *ifc;
	int i, length;
	int sockfd;
	char ssid[IW_ESSID_MAX_SIZE + 1];
	struct iwreq req;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd >= 0) {
		memset(ssid, 0, IW_ESSID_MAX_SIZE + 1);
		req.u.essid.pointer = ssid;
		req.u.essid.length = IW_ESSID_MAX_SIZE;
		length = FskNetInterfaceEnumerate();
		for (i = 0; i < length; i++) {
			if (!FskNetInterfaceDescribe(i, &ifc)) {
				strcpy(req.ifr_name, ifc->name);
				if (ioctl(sockfd, SIOCGIWESSID, &req) != -1) {
					xsSet(xsResult, xsID("SSID"), xsString((xsStringValue)ssid));
					break;
				}
				FskNetInterfaceDescriptionDispose(ifc);
			}
		}
		close(sockfd);
	}
}

void KPR_system_networkInterfaceIndexToName(xsMachine* the)
{
	char name[IF_NAMESIZE];
	UInt32 index = xsToInteger(xsArg(0));
	if (if_indextoname(index, name))
		xsResult = xsString((xsStringValue)name);
}

typedef struct {
	xsMachine* the;
	xsSlot slot;
	posix_spawn_file_actions_t action;
	int stdPipe[2];
	int pid;
	xsIntegerValue status;
	xsIntegerValue usage;
} KprShellExecRecord, *KprShellExec;

static void KPR_shell_execute_async(KprShellExec exec);
static void KPR_shell_execute_callback(KprShellExec exec);
static void KPR_shell_execute_cancel(xsMachine* the);
static void KPR_shell_execute_destructor(void* data);
static void KPR_shell_execute_stderr(KprShellExec exec, xsStringValue string);

void KPR_shell_execute_async(KprShellExec exec)
{
#define BUFSIZE 4095
	char buffer[BUFSIZE + 1];
	char* p;
	int status, size, offset = 0;
	for (;;)  {
		if ((size = read(exec->stdPipe[0], buffer + offset, BUFSIZE - offset)) <= 0)
			break;
		offset += size;
		buffer[offset] = 0;
		if ((p = FskStrChr(buffer, '\n')) || (offset == BUFSIZE)) {
			if (p) *p = 0;
			exec->usage++;
			FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KPR_shell_execute_stderr, exec, FskStrDoCopy(buffer), NULL, NULL);
			offset = 0;
		}
	}
	if (waitpid(exec->pid, &status, 0) != -1) {
		exec->status = status;
	}
	else {
		exec->status = -1;
	}
	exec->pid = 0;
	posix_spawn_file_actions_destroy(&exec->action);
	if (exec->stdPipe[0]) close(exec->stdPipe[0]);
	if (exec->stdPipe[1]) close(exec->stdPipe[1]);
	
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KPR_shell_execute_callback, exec, NULL, NULL, NULL);
}

void KPR_shell_execute_callback(KprShellExec exec)
{
	exec->usage--;
	if (exec->usage == 0) {
		xsBeginHost(exec->the);
		xsVars(3);
		xsVar(0) = xsAccess(exec->slot);
		xsVar(1) = xsGet(xsVar(0), xsID_callback);
		if (xsTest(xsVar(1))) {
			xsVar(2) = xsInteger(exec->status);
			xsResult = xsCallFunction1(xsVar(1), xsNull, xsVar(2));
		}
		xsForget(exec->slot);
		xsSetHostData(xsVar(0), NULL);
		xsEndHost(exec->the);
		FskMemPtrDispose(exec);
	}
}

void KPR_shell_execute_cancel(xsMachine* the)
{
	KprShellExec exec = (KprShellExec)xsGetHostData(xsThis);
	if (exec->pid)
		kill(exec->pid, SIGKILL);
}

void KPR_shell_execute_destructor(void* data)
{
	// never
}

void KPR_shell_execute_stderr(KprShellExec exec, xsStringValue string)
{
	if (string) {
		xsBeginHost(exec->the);
		xsVars(3);
		xsVar(0) = xsAccess(exec->slot);
		xsVar(1) = xsGet(xsVar(0), xsID_stderr);
		if (xsTest(xsVar(1))) {
			xsVar(2) = xsString(string);
			xsResult = xsCallFunction1(xsVar(1), xsNull, xsVar(2));
		}
		xsEndHost(exec->the);
		FskMemPtrDispose(string);
	}
	KPR_shell_execute_callback(exec);
}

void KPR_shell_execute(xsMachine* the)
{
	xsStringValue directory = NULL;
	xsStringValue string;
	xsIntegerValue length;
	KprShellExec exec = NULL;
	xsVars(5);
	char* command[] = { "sh", "-c", NULL, NULL };
	char** environment = NULL;
	xsTry {
		command[2] = xsToString(xsArg(0));

		if (xsFindString(xsArg(1), xsID_directory, &string)) {
			length = FskStrLen(string) + 1;
			xsThrowIfFskErr(FskMemPtrNew(length, &directory));
			FskStrCopy(directory, string);
		}
		if (xsFindResult(xsArg(1), xsID_environment)) {
			int i;
			xsVar(1) = xsEnumerate(xsResult);
			xsThrowIfFskErr(FskMemPtrNewClear(sizeof(char*), &environment));
			for (i = 1; ; i++) {
				char *key, *value, *string;
				xsVar(2) = xsCall0(xsVar(1), xsID("next"));
				xsVar(3) = xsGet(xsVar(2), xsID("done"));
				if (xsTest(xsVar(3)))
					break;
				xsVar(3) = xsGet(xsVar(2), xsID("value"));
				xsVar(4) = xsGetAt(xsResult, xsVar(3));
				xsThrowIfFskErr(FskMemPtrRealloc(sizeof(char*) * (i + 1), &environment));
				key = xsToString(xsVar(3));
				value = xsToString(xsVar(4));
				xsThrowIfFskErr(FskMemPtrNewClear(FskStrLen(key) + FskStrLen(value) + 2, &string));
				FskStrCopy(string, key);
				FskStrCat(string, "=");
				FskStrCat(string, value);
				environment[i - 1] = string;
				environment[i] = NULL;
			}
		}
		
		xsThrowIfFskErr(FskMemPtrNewClear(sizeof(KprShellExecRecord), &exec));
		xsVar(0) = xsNewHostObject(KPR_shell_execute_destructor);
		exec->the = the;
		exec->slot = xsVar(0);
		xsSetHostData(xsVar(0), exec);
		
		xsResult = xsNewHostFunction(KPR_shell_execute_cancel, 0);
		xsNewHostProperty(xsVar(0), xsID_cancel, xsResult, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		if (xsFindResult(xsArg(1), xsID_callback)) {
			xsNewHostProperty(xsVar(0), xsID_callback, xsResult, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		}
		else {
			xsNewHostProperty(xsVar(0), xsID_callback, xsNull, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		}
		if (xsFindResult(xsArg(1), xsID_stderr)) {
			xsNewHostProperty(xsVar(0), xsID_stderr, xsResult, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		}
		else {
			xsNewHostProperty(xsVar(0), xsID_stderr, xsNull, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		}
		if (xsFindResult(xsArg(1), xsID_stdout)) {
			xsNewHostProperty(xsVar(0), xsID_stdout, xsResult, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		}
		else {
			xsNewHostProperty(xsVar(0), xsID_stdout, xsNull, xsDefault, xsDontDelete | xsDontEnum | xsDontSet);
		}
		
		exec->stdPipe[0] = -1;
		exec->stdPipe[1] = -1;
		pipe(exec->stdPipe);
		posix_spawn_file_actions_init(&exec->action);
		posix_spawn_file_actions_adddup2(&exec->action, exec->stdPipe[1], STDERR_FILENO);
		posix_spawn_file_actions_adddup2(&exec->action, exec->stdPipe[1], STDOUT_FILENO);
		posix_spawn_file_actions_addclose(&exec->action, exec->stdPipe[0]);
		if (posix_spawnp(&exec->pid, "/bin/sh", &exec->action, NULL, command, environment))
			xsThrowIfFskErr(kFskErrOperationFailed);
		
		close(exec->stdPipe[1]);
		exec->stdPipe[1] = -1;
		
		xsRemember(exec->slot);
		exec->usage++;
		FskThreadPostCallback(KprHTTPGetThread(), (FskThreadCallback)KPR_shell_execute_async, exec, NULL, NULL, NULL);
		if (environment) {
			int i;
			for (i = 0; environment[i]; i++) {
				FskMemPtrDispose(environment[i]);
			}
			FskMemPtrDispose(environment);
		}
		FskMemPtrDispose(directory);
	}
	xsCatch {
		if (exec) {
			if (exec->stdPipe[0]) close(exec->stdPipe[0]);
			if (exec->stdPipe[1]) close(exec->stdPipe[1]);
			posix_spawn_file_actions_destroy(&exec->action);
			FskMemPtrDispose(exec);
		}
		if (environment) {
			int i;
			for (i = 0; environment[i]; i++) {
				FskMemPtrDispose(environment[i]);
			}
			FskMemPtrDispose(environment);
		}
		FskMemPtrDispose(directory);
		xsThrow(xsException);
	}
}

void KPR_shell_splitError(xsMachine* the)
{
	xsStringValue string, p, q;
	char c;
	xsIndex ids[4];
	size_t offsets[4], lengths[4];
	int i;
	xsVars(1);
	string = p = xsToString(xsArg(0));
	ids[0] = xsID_path;
	offsets[0] = p - string;
	c = *p;
	if (c == '/')
		p = FskStrChr(p, ':');
	else if (('A' <= c) && (c <= 'Z') && (*(p + 1) == ':'))
		p = FskStrChr(p + 2, ':');
	else
		goto bail;
	if (!p) goto bail;
	ids[1] = xsID_line;
	q = p - 1;
	c = *q;
	if (c == ')') {
		q--;
		while ((q > string) && ((c = *q)) && ('0' <= c) && (c <= '9'))
			q--;
		if (c != '(') goto bail;
		lengths[0] = q - string;
		offsets[1] = q + 1 - string;
		lengths[1] = (p - q) - 2;
	}
	else {
		lengths[0] = p - string;
		p++;
		offsets[1] = p - string;
		while (((c = *p)) && ('0' <= c) && (c <= '9'))
			p++;
		if (c != ':') goto bail;
		lengths[1] = (p - string) - offsets[1];
	}
	p++;
	c  = *p;
	if (('0' <= c) && (c <= '9')) {
		p++;
		while (((c = *p)) && ('0' <= c) && (c <= '9'))
			p++;
		if (c != ':') goto bail;
		p++;
		c  = *p;
	}
	if (c != ' ') goto bail;
	p++;
	ids[2] = xsID_kind;
	offsets[2] = p - string;
	p = FskStrChr(p, ':');
	if (!p) goto bail;
	lengths[2] = (p - string) - offsets[2];
	p++;
	c = *p;
	if (c != ' ') goto bail;
	p++;
	ids[3] = xsID_reason;
	offsets[3] = p - string;
	lengths[3] = FskStrLen(p);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	for (i = 0; i < 4; i++) {
		xsVar(0) = xsStringBuffer(NULL, lengths[i]);
		string = xsToString(xsArg(0));
		FskMemCopy(xsToString(xsVar(0)), string + offsets[i], lengths[i]);
		xsNewHostProperty(xsResult, ids[i], xsVar(0), xsDefault, xsDontScript);
	}
bail:
	return;
}
