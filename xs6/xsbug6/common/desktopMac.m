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
#import <Cocoa/Cocoa.h>
#include "FskCocoaSupport.h"
#include "kpr.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprShell.h"
#include "kprURL.h"

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
	[[NSRunningApplication currentApplication] activateWithOptions:NSApplicationActivateIgnoringOtherApps];
}

void KPR_system_getClipboardText(xsMachine* the)
{
    char *utf8Ptr = NULL;
	FskCocoaClipboardTextGet(&utf8Ptr);
	if (utf8Ptr) {
		xsResult = xsString(utf8Ptr);
		FskMemPtrDispose(utf8Ptr);
	}
	else
		xsResult = xsString("");
}

void KPR_system_setClipboardText(xsMachine* the)
{
	if (xsToInteger(xsArgc) > 0)
		FskCocoaClipboardTextSet(xsToString(xsArg(0)));
	else
		FskCocoaClipboardTextSet("");
}

static xsSlot callback;

void KPR_system_alert(xsMachine* the)
{
	int argc = xsToInteger(xsArgc);
	KprShell self = gShell;
	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	xsStringValue string;
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_type, &string)) {
			if (!FskStrCompare(string, "about"))
				[alert setAlertStyle:NSInformationalAlertStyle];
			else if (!FskStrCompare(string, "stop"))
				[alert setAlertStyle:NSCriticalAlertStyle];
			else if (!FskStrCompare(string, "note"))
				[alert setAlertStyle:NSWarningAlertStyle];
		}
		if (xsFindString(xsArg(0), xsID_prompt, &string))
			[alert setMessageText:[NSString stringWithUTF8String:string]];
		if (xsFindString(xsArg(0), xsID_info, &string))
			[alert setInformativeText:[NSString stringWithUTF8String:string]];
		if (xsFindResult(xsArg(0), xsID_buttons)) {
			if (xsIsInstanceOf(xsResult, xsArrayPrototype)) {
				xsIntegerValue c = xsToInteger(xsGet(xsResult, xsID_length)), i;
				for (i = 0; i < c; i++) {
					string = xsToString(xsGet(xsResult, i));
					[alert addButtonWithTitle:[NSString stringWithUTF8String:string]];
				}
			}
		}
	}
	if ((argc > 1) && xsTest(xsArg(1)))
		callback = xsArg(1);
	else
		callback = xsNull;
	xsRemember(callback);
	(*self->handler)(NULL, kFskEventWindowDeactivated, NULL, self);
	[alert beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {
		(*self->handler)(NULL, kFskEventWindowActivated, NULL, self);
		[alert.window close]; 
		xsBeginHostSandboxCode(self->the, self->code);
		{
			xsVars(1);
			{
				xsTry {
					xsResult = xsAccess(callback);
					xsForget(callback);
					callback = xsNull;
					if (xsTest(xsResult)) {
						xsVar(0) = (result == 1000) ? xsTrue : (result == 1001) ? xsUndefined : xsFalse;
						(void)xsCallFunction1(xsResult, xsNull, xsVar(0));
					}
				}
				xsCatch {
				}
			}
		}
		xsEndHostSandboxCode();
	}];
}

@class KPR_system_open_delegate;
@interface KPR_system_open_delegate : NSObject <NSOpenSavePanelDelegate> {
    NSString *_name;
}
- (instancetype)initWith:(NSString *)name;
- (void)dealloc;
- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url;
@end
@implementation KPR_system_open_delegate {
}
- (instancetype)initWith:(NSString *)name {
	self = [super init];
	if (self) {
		_name = [name retain];
	}
	return self;
}
- (void)dealloc {
    [_name release];
    [super dealloc];
}
- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url {
    NSNumber *isDirectory;
    [url getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];
	return [isDirectory boolValue] || [_name isEqualToString:[url lastPathComponent]];
}
@end

static void KPR_system_open(xsMachine* the, xsBooleanValue flag)
{
	int argc = xsToInteger(xsArgc);
	KprShell self = gShell;
	NSOpenPanel *openPanel = [NSOpenPanel openPanel];
	xsStringValue string;
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_fileType, &string))
			[openPanel setAllowedFileTypes: [NSArray arrayWithObject:[NSString stringWithUTF8String:string]]];
		if (xsFindString(xsArg(0), xsID_message, &string))
			[openPanel setMessage:[NSString stringWithUTF8String:string]];
		if (xsFindString(xsArg(0), xsID_name, &string))
			[openPanel setDelegate:[[KPR_system_open_delegate alloc] initWith:[NSString stringWithUTF8String:string]]];
		if (xsFindString(xsArg(0), xsID_prompt, &string))
			[openPanel setPrompt:[NSString stringWithUTF8String:string]];
		if (xsFindString(xsArg(0), xsID_url, &string))
			[openPanel setDirectoryURL:[NSURL URLWithString:[NSString stringWithUTF8String:string]]];
	}
	[openPanel setAllowsMultipleSelection:NO];
	[openPanel setCanChooseDirectories:flag ? YES : NO];
	[openPanel setCanChooseFiles:flag ? NO : YES];
	if ((argc > 1) && xsTest(xsArg(1)))
		callback = xsArg(1);
	else
		callback = xsNull;
	xsRemember(callback);
	(*self->handler)(NULL, kFskEventWindowDeactivated, NULL, self);
	[openPanel beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {		
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
						if (result)
							xsVar(0) = xsString((xsStringValue)[[[[openPanel URLs] objectAtIndex:0] absoluteString] UTF8String]);
						(void)xsCallFunction1(xsResult, xsNull, xsVar(0));
					}
				}
				xsCatch {
				}
			}
		}
		xsEndHostSandboxCode();
		id delegate = [openPanel delegate];
		if (delegate)
			[delegate release];
	}];
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
	NSSavePanel* savePanel = [NSSavePanel savePanel];
	xsStringValue string;
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_name, &string))
			[savePanel setNameFieldStringValue:[NSString stringWithUTF8String:string]];
		if (xsFindString(xsArg(0), xsID_prompt, &string))
			[savePanel setPrompt:[NSString stringWithUTF8String:string]];
		if (xsFindString(xsArg(0), xsID_url, &string))
			[savePanel setDirectoryURL:[NSURL URLWithString:[NSString stringWithUTF8String:string]]];
	}
	if ((argc > 1) && xsTest(xsArg(1)))
		callback = xsArg(1);
	else
		callback = xsNull;
	xsRemember(callback);
	(*self->handler)(NULL, kFskEventWindowDeactivated, NULL, self);
	[savePanel beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) { 
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
						if (result)
							xsVar(0) = xsString((xsStringValue)[[[savePanel URL] absoluteString] UTF8String]);
						(void)xsCallFunction1(xsResult, xsNull, xsVar(0));
					}
				}
				xsCatch {
				}
			}
		}
		xsEndHostSandboxCode();
	}];
}


void KPR_system_saveDirectory(xsMachine* the)
{
	KPR_system_save(the, 1);
}

void KPR_system_saveFile(xsMachine* the)
{
	KPR_system_save(the, 0);
}
