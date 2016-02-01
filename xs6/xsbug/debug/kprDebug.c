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

#include "kpr.h"
#include "kprUtilities.h"

#include "FskHTTPClient.h"
//#include "FskHTTPServer.h"

FskExport(FskErr) kprDebug_fskLoad(FskLibrary library)
{
	return kFskErrNone;
}

FskExport(FskErr) kprDebug_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

typedef struct KprDebugStruct KprDebugRecord, *KprDebug;
typedef struct KprDebugMachineStruct KprDebugMachineRecord, *KprDebugMachine;

#define XS_BUFFER_COUNT 1024

struct KprDebugStruct {
	KprDebug next;
	xsSlot behavior;
	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	UInt32 port;
	KprSocketServer server;
	KprDebugMachine machine;
	FskInstrumentedItemDeclaration
};

struct KprDebugMachineStruct {
	KprDebugMachine next;
	KprDebug debug;
	FskSocket socket;
	FskThreadDataHandler reader;
	KprSocketWriter writer;
	char address[32];
	char* title;
	
	int broken;

	int done;
	int view;
	int row;
	int column;

	int state;
	int attributeIndex;
	int dataIndex;
	int tagIndex;
	int entityNumber;
	int entityState;

	char buffer[XS_BUFFER_COUNT + 1];

	char attribute[XS_BUFFER_COUNT + 1];
	char data[XS_BUFFER_COUNT + 1];
	char tag[XS_BUFFER_COUNT + 1];

	char flags[XS_BUFFER_COUNT + 1];
	int line;
	char name[XS_BUFFER_COUNT + 1];
	char path[XS_BUFFER_COUNT + 1];
	char value[XS_BUFFER_COUNT + 1];

	FskInstrumentedItemDeclaration
};

#define mxNoCommand -1

enum {
	mxFramesView = 0,
	mxLocalsView,
	mxGlobalsView,
	mxFilesView,
	mxBreakpointsView,
	mxGrammarsView,
	mxFileView,
	mxLogView,
	mxViewCount,
	
	mxAbortCommand,
	mxGoCommand,
	mxPauseCommand,
	mxStepCommand,
	mxStepInCommand,
	mxStepOutCommand,
	
	mxStackCommand,
	mxFrameCommand,
	
	mxGlobalCommand,
	mxInstanceCommand,
	
	mxListBreakpointsCommand,
	mxClearAllBreakpointsCommand,
	mxClearBreakpointCommand,
	mxSetBreakpointCommand,
	
	mxListFilesCommand
};


#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprDebugInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprDebug", FskInstrumentationOffset(KprDebugRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
static FskInstrumentedTypeRecord KprDebugMachineInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprDebugMachine", FskInstrumentationOffset(KprDebugMachineRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

static FskErr KprDebugNew(KprDebug* it, UInt32 port);
static void KprDebugDispose(KprDebug self);
static KprDebugMachine KprDebugFindMachine(KprDebug self, char* address);

static void KprDebugMachineCallback(KprDebugMachine machine, char* function);
static void KprDebugMachineCallbackText(KprDebugMachine machine, char* function, int view, char* text);
static void KprDebugMachineCallbackView(KprDebugMachine machine, char* function, int view);

static FskErr KprDebugMachineNew(KprDebugMachine* it, KprDebug debug, FskSocket skt);
static void KprDebugMachineDispose(KprDebugMachine self);

static void KprDebugMachineDispatchCommand(KprDebugMachine self, int theCommand, char* name, char* value, SInt32 number);

static void KprDebugMachineParse(KprDebugMachine self, char* theString, int theLength);
static void KprDebugMachineParseAttribute(KprDebugMachine self, char* theName, char* theValue);
static void KprDebugMachineParseData(KprDebugMachine self, char* theData);
static void KprDebugMachineParseTag(KprDebugMachine self, char* theName);
static void KprDebugMachinePopTag(KprDebugMachine self, char* theName);
static void KprDebugMachinePushTag(KprDebugMachine self, char* theName);

static void KprDebugMachineChangeTitle(KprDebugMachine self, char* theTitle);
static void KprDebugMachineLockView(KprDebugMachine self, int theView);
static void KprDebugMachineEchoItem(KprDebugMachine self, int theView, char* path, int line, char* name, char* value);
static void KprDebugMachineEchoNode(KprDebugMachine self, int theView, int column, char flags, char* path, int line, char* name, char* data, char* value);
static void KprDebugMachineEchoProperty(KprDebugMachine self, int theView, int column, char* flags, char* name, char* value);
static void KprDebugMachineEchoView(KprDebugMachine self, int theView, char* theName, char* theValue);
static void KprDebugMachineLoadView(KprDebugMachine self, int theView, char* thePath, int theLine);
static void KprDebugMachinePrintView(KprDebugMachine self, int theView, char* theFormat, ...);
static void KprDebugMachineSetViewSelection(KprDebugMachine self, int theView, char* theSelection);
static void KprDebugMachineSetViewTitle(KprDebugMachine self, int theView, char* theTitle);
static void KprDebugMachineUnlockView(KprDebugMachine self, int theView);

//--------------------------------------------------
// KprDebug
//--------------------------------------------------

static FskErr KprDebugAcceptNewConnection(KprSocketServer server UNUSED, FskSocket skt, const char *interfaceName UNUSED, UInt32 ip, void *refcon);

KprDebug gKprDebugList = NULL;

FskErr KprDebugNew(KprDebug* it, UInt32 port)
{
	FskErr err = kFskErrNone;
	KprDebug self = NULL;
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(KprDebugRecord), it));
	self = *it;
	self->port = port;
	BAIL_IF_ERR(err = KprSocketServerNew(&self->server, self));
	self->server->debugName = "Debug";
	self->server->acceptCallback = KprDebugAcceptNewConnection;
	BAIL_IF_ERR(err = KprSocketServerListen(self->server, self->port, NULL));
	FskListAppend(&gKprDebugList, self);
	FskInstrumentedItemNew(self, NULL, &KprDebugInstrumentation);
	return err;
bail:
	KprDebugDispose(self);
	return err;
}

void KprDebugClose(KprDebug self)
{
	KprDebugMachine machine = NULL;
	FskListRemove(&gKprDebugList, self);
	while ((machine = self->machine))
		KprDebugMachineDispose(machine);
	if (self->server) {
		KprSocketServerDispose(self->server);
		self->server = NULL;
	}
}

void KprDebugDispose(KprDebug self)
{
	if (self) {
		KprDebugClose(self);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

FskErr KprDebugAcceptNewConnection(KprSocketServer server UNUSED, FskSocket skt, const char *interfaceName UNUSED, UInt32 ip, void *refcon)
{
	KprDebug self = refcon;
	FskErr err = kFskErrNone;
	KprDebugMachine machine = NULL;

	FskNetSocketMakeNonblocking(skt);

	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(3);
	BAIL_IF_ERR(err = KprDebugMachineNew(&machine, self, skt));

bail:
	if (err) {
		FskNetSocketClose(skt);
	}
	xsEndHostSandboxCode();

	return err;
}

KprDebugMachine KprDebugFindMachine(KprDebug self, char* address)
{
	KprDebugMachine machine = NULL;
	for (machine = self->machine; machine; machine = machine->next) {
		if (!FskStrCompareCaseInsensitive(machine->address, address))
			return machine;
	}
	return NULL;
}

//--------------------------------------------------
// KprDebugMachine
//--------------------------------------------------

static void KprDebugMachineReadError(KprSocketErrorContext context UNUSED, FskErr err, void *refcon);
static void KprDebugMachineWriteError(KprSocketErrorContext context UNUSED, FskErr err, void *refcon);

#define kSocketBufferSize (128 * 1024)

enum {
	XS_HEADER_STATE,
	XS_HEADER_RETURN_STATE,
	XS_BODY_STATE,
	XS_DATA_STATE,
	XS_TAG_STATE,
	XS_START_TAG_NAME_STATE,
	XS_START_TAG_SPACE_STATE,
	XS_ATTRIBUTE_NAME_STATE,
	XS_ATTRIBUTE_SPACE_STATE,
	XS_ATTRIBUTE_EQUAL_STATE,
	XS_ATTRIBUTE_VALUE_STATE,
	XS_EMPTY_TAG_STATE,
	XS_END_TAG_STATE,
	XS_END_TAG_NAME_STATE,
	XS_END_TAG_SPACE_STATE,
	XS_PROCESSING_INSTRUCTION_STATE,
	XS_PROCESSING_INSTRUCTION_SPACE_STATE,
	XS_ENTITY_STATE,
	XS_ENTITY_NUMBER_STATE,
	XS_ERROR_STATE
};

static void KprDebugMachineDataReader(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);

FskErr KprDebugMachineNew(KprDebugMachine* it, KprDebug debug, FskSocket skt)
{
	FskErr err = kFskErrNone;
	KprDebugMachine self = NULL;
	UInt32 ip;
	int port;
	
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(KprDebugMachineRecord), it));
	self = *it;
	self->debug = debug;
	self->socket = skt;

	BAIL_IF_ERR(err = FskNetSocketGetRemoteAddress(skt, &ip, &port));
	FskNetIPandPortToString(ip, port, self->address);

	FskListAppend(&self->debug->machine, self);

	FskNetSocketReceiveBufferSetSize(self->socket, kSocketBufferSize);

	FskThreadAddDataHandler(&self->reader, (FskThreadDataSource)self->socket, KprDebugMachineDataReader, true, false, self);

	bailIfError(KprSocketWriterNew(&self->writer, self->socket, self));
	self->writer->errorCallback = KprDebugMachineWriteError;

	KprDebugMachineCallbackText(self, "onMachineRegistered", mxNoCommand, NULL);

	FskInstrumentedItemNew(self, NULL, &KprDebugMachineInstrumentation);
	return err;
bail:
	KprDebugMachineDispose(self);
	return err;
}

void KprDebugMachineDispose(KprDebugMachine self)
{
	if (self) {
		KprDebug debug = self->debug;
		xsBeginHostSandboxCode(debug->the, debug->code);
		xsVars(1);
		KprDebugMachineCallbackText(self, "onMachineUnregistered", mxNoCommand, NULL);
		xsEndHostSandboxCode();
		FskListRemove(&self->debug->machine, self);
		KprSocketWriterDispose(self->writer);
		self->writer = NULL;
		FskThreadRemoveDataHandler(&self->reader);
		FskNetSocketClose(self->socket);
		self->socket = NULL;
		FskMemPtrDisposeAt(&self->title);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

void KprDebugMachineCallback(KprDebugMachine machine, char* function)
{
	KprDebug self = machine->debug;
	xsMachine* the = self->the;
	if (xsTypeOf(self->behavior) == xsUndefinedType) goto bail;
	{
		xsTry {
			xsVar(0) = xsAccess(self->behavior);
			if (xsFindResult(xsVar(0), xsID(function))) {
				if (xsHasOwn(xsVar(0), xsID(machine->address))) {
					xsVar(1) = xsGet(xsVar(0), xsID(machine->address));
					xsDelete(xsVar(0), xsID(machine->address));
				}
				(void)xsCallFunction2(xsResult, xsVar(0), xsString(machine->address), xsVar(1));
			}
		}
		xsCatch {
		}
	}
bail:
	return;
}

void KprDebugMachineCallbackText(KprDebugMachine machine, char* function, int view, char* text)
{
	KprDebug self = machine->debug;
	xsMachine* the = self->the;
	if (xsTypeOf(self->behavior) == xsUndefinedType) goto bail;
	{
		xsTry {
			xsVar(0) = xsAccess(self->behavior);
			if (xsFindResult(xsVar(0), xsID(function))) {
				if (view >= 0)
					(void)xsCallFunction3(xsResult, xsVar(0), xsString(machine->address), xsInteger(view), xsString(text));
				else if (text)
					(void)xsCallFunction2(xsResult, xsVar(0), xsString(machine->address), xsString(text));
				else
					(void)xsCallFunction1(xsResult, xsVar(0), xsString(machine->address));
			}
		}
		xsCatch {
		}
	}
bail:
	return;
}

void KprDebugMachineCallbackView(KprDebugMachine machine, char* function, int view)
{
	KprDebug self = machine->debug;
	xsMachine* the = self->the;
	if (xsTypeOf(self->behavior) == xsUndefinedType) goto bail;
	{
		xsTry {
			xsVar(0) = xsAccess(self->behavior);
			if (xsFindResult(xsVar(0), xsID(function))) {
				xsVar(1) = xsGet(xsVar(0), xsID(machine->address));
				xsDelete(xsVar(0), xsID(machine->address));
				(void)xsCallFunction3(xsResult, xsVar(0), xsString(machine->address), xsInteger(view), xsVar(1));
			}
		}
		xsCatch {
		}
	}
bail:
	return;
}

void KprDebugMachineDataReader(FskThreadDataHandler handler UNUSED, FskThreadDataSource source UNUSED, void *refCon)
{
	FskErr err = kFskErrNone;
	KprDebugMachine self = refCon;
	KprDebug debug = self->debug;
	int size;

	err = FskNetSocketRecvTCP(self->socket, self->buffer, XS_BUFFER_COUNT, &size);
	if (size > 0) {
		xsBeginHostSandboxCode(debug->the, debug->code);
		xsVars(4);
		KprDebugMachineParse(self, self->buffer, size);
		if (self->done) {
			self->state = XS_HEADER_STATE;
			self->dataIndex = 0;
			
			if (self->broken) {
				xsCall0(xsGet(xsGlobal, xsID_KPR), xsID_gotoFront);
			}
			else {
				xsCall1(debug->slot, xsID_go, xsString(self->address));
				KprDebugMachineCallback(self, "gone");
			}
		}
		xsEndHostSandboxCode();
	}
	if (err) {
		KprDebugMachineDispose(self);
	}
}

void KprDebugMachineWriteError(KprSocketErrorContext context UNUSED, FskErr err, void *refcon)
{
//	KprDebugMachine self = refcon;
	FskDebugStr("write error");
	return;
}


//--------------------------------------------------
// KprDebugMachine - Writer
//--------------------------------------------------

void KprDebugMachineDispatchCommand(KprDebugMachine self, int theCommand, char* name, char* value, SInt32 number)
{
	static char aBuffer[16 * 1024];
	static char aValue[4096];
	int aCount;
	char* aColon;
	char* aLine;
	
	strcpy(aBuffer, "HTTP/1.1 200 OK\15\12Content-type: xml/xsbug\15\12\15\12");
	strcat(aBuffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	switch (theCommand) {
	case mxAbortCommand:
		strcat(aBuffer, "<abort/>");
		break;
	case mxGoCommand:
		strcat(aBuffer, "<go/>");
		break;
	case mxStepCommand:
		strcat(aBuffer, "<step/>");
		break;
	case mxStepInCommand:
		strcat(aBuffer, "<step-inside/>");
		break;
	case mxStepOutCommand:
		strcat(aBuffer, "<step-outside/>");
		break;
	case mxClearAllBreakpointsCommand:
		strcat(aBuffer, "<clear-all-breakpoints/>");
		break;
		
	case mxClearBreakpointCommand:
		if (name && (number >= 0)) {
			strcat(aBuffer, "<clear-breakpoint path=\"");
			strcat(aBuffer, name);
			strcat(aBuffer, "\" line=\"");
			sprintf(aValue, "%ld", number);
			strcat(aBuffer, aValue);
			strcat(aBuffer, "\"/>");
		}
		else
			return;
		break;
	case mxSetBreakpointCommand:
		if (name && (number >= 0)) {
			strcat(aBuffer, "<set-breakpoint path=\"");
			strcat(aBuffer, name);
			strcat(aBuffer, "\" line=\"");
			sprintf(aValue, "%ld", number);
			strcat(aBuffer, aValue);
			strcat(aBuffer, "\"/>");
		}
		else
			return;
		break;
	
	case mxLogView:
		aCount = FskStrLen(name);
		if (aCount) {
			aColon = name;
			if ((name[1] == ':') && ((name[2] == '/') || (name[2] == '\\')))
				aColon += 3;
			aColon = strchr(aColon, ':');
			if (aColon) {
				*aColon = 0;
				aLine = aColon + 1;
				aColon = strchr(aLine, ':');
				if (aColon) {
					*aColon = 0;
					KprDebugMachineLoadView(self, mxFileView, name, strtol(aLine, NULL, 10));
				}
			}
		}
		return;

	case mxFilesView:
		KprDebugMachineLoadView(self, mxFileView, name, -1);
		return;
	case mxBreakpointsView:
		aCount = FskStrLen(name);
		if (aCount)
			KprDebugMachineLoadView(self, mxFileView, name, number);
		return;
	case mxFramesView:
		KprDebugMachineLoadView(self, mxFileView, name, number);
		aCount = FskStrLen(value);
		if (aCount && (value[0] == '@')) {
			strcat(aBuffer, "<select id=\"");
			strcat(aBuffer, value);
			strcat(aBuffer, "\"/>");
		}
		else
			return;
		break;	
	case mxGrammarsView:
		aCount = FskStrLen(name);
		aColon = strrchr(name, ' ');
		if (aColon) {
			aColon++;
			aLine = aColon;
			if ((aLine[1] == ':') && ((aLine[2] == '/') || (aLine[2] == '\\')))
				aLine += 3;
			aLine = strchr(aLine, ':');
			if (aLine) {
				*aLine = 0;
				KprDebugMachineLoadView(self, mxFileView, aColon, strtol(aLine + 1, NULL, 10));
			}
		}
		/* continue */	
	case mxLocalsView:
	case mxGlobalsView:
		aCount = value ? FskStrLen(value) : 0;
		if (aCount && (value[0] == '@')) {
			strcat(aBuffer, "<toggle id=\"");
			strcat(aBuffer, value);
			strcat(aBuffer, "\"/>");
		}
		else
			return;
		break;
	default:
		return;
	}
	aCount = strlen(aBuffer);
	KprSocketWriterSendBytes(self->writer, aBuffer, aCount);
}


//--------------------------------------------------
// KprDebugMachine - Reader
//--------------------------------------------------

#define mxIsDigit(c) \
	(('0' <= c) && (c <= '9'))
#define mxIsFirstLetter(c) \
	((('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '_') || (c == ':'))
#define mxIsNextLetter(c) \
	((('0' <= c) && (c <= '9')) || (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '.') || (c == '-') || (c == '_') || (c == ':'))
#define mxIsSpace(c) \
	((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t'))

void KprDebugMachineParse(KprDebugMachine self, char* theString, int theLength)
{
	char c;
	
//	FskDebugStr("%s: %.*s", __FUNCTION__, theLength, theString);
	while (theLength) {
		c = *theString++;

		switch (self->state) {
		case XS_HEADER_STATE:
			if (c == 13) {
				self->data[self->dataIndex] = 0;
				self->state = XS_HEADER_RETURN_STATE;
			}
			else if (self->dataIndex < XS_BUFFER_COUNT) {
				self->data[self->dataIndex] = c;
				self->dataIndex++;
			}
			else
				self->state = XS_ERROR_STATE;
			break;
		case XS_HEADER_RETURN_STATE:
			if (c == 10) {
				if (self->dataIndex == 0)
					self->state = XS_BODY_STATE;
				else {
					/* parse header */
					self->state = XS_HEADER_STATE;
					self->dataIndex = 0;
				}
			}
			else
				self->state = XS_ERROR_STATE;
			break;	
				
		case XS_BODY_STATE:
			if (c == '<')
				self->state = XS_TAG_STATE;
			else if (!mxIsSpace(c))
				self->state = XS_ERROR_STATE;
			break;
				
		case XS_DATA_STATE:
			if (c == '<') {
				self->data[self->dataIndex] = 0;
				KprDebugMachineParseData(self, self->data);
				self->state = XS_TAG_STATE;
			}
			else if (c == '&') {
				self->entityState = XS_DATA_STATE;
				self->state = XS_ENTITY_STATE;
			}
			else {
				if (self->dataIndex == XS_BUFFER_COUNT) {
					self->data[self->dataIndex] = 0;
					KprDebugMachineParseData(self, self->data);
					self->dataIndex = 0;
				}
				self->data[self->dataIndex] = c;
				self->dataIndex++;
			}
			break;
				
		case XS_TAG_STATE:
			if (c == '/')
				self->state = XS_END_TAG_STATE;
			else if (c == '?') {
				self->state = XS_PROCESSING_INSTRUCTION_STATE;
				self->dataIndex = 0;
			}
			else if (mxIsFirstLetter(c)) {
				self->state = XS_START_TAG_NAME_STATE;
				self->tag[0] = c;
				self->tagIndex = 1;
			}
			else
				self->state = XS_ERROR_STATE;
			break;
			
		case XS_START_TAG_NAME_STATE:
			if (mxIsNextLetter(c) && (self->tagIndex < XS_BUFFER_COUNT)) {	
				self->tag[self->tagIndex] = c;
				self->tagIndex++;
				break;
			}
			else {
				self->tag[self->tagIndex] = 0;
				KprDebugMachinePushTag(self, self->tag);
				self->state = XS_START_TAG_SPACE_STATE;
				/* continue */
			}
		case XS_START_TAG_SPACE_STATE:
			if (mxIsFirstLetter(c)) {
				self->state = XS_ATTRIBUTE_NAME_STATE;
				self->attribute[0] = c;
				self->attributeIndex = 1;
			}
			else if (c == '/')
				self->state = XS_EMPTY_TAG_STATE;
			else if (c == '>') {
				KprDebugMachineParseTag(self, self->tag);
				self->state = XS_DATA_STATE;
				self->dataIndex = 0;
			}
			else if (!mxIsSpace(c))
				self->state = XS_ERROR_STATE;
			break;
			
		case XS_ATTRIBUTE_NAME_STATE:
			if (mxIsNextLetter(c) && (self->tagIndex < XS_BUFFER_COUNT)) {	
				self->attribute[self->attributeIndex] = c;
				self->attributeIndex++;
				break;
			}
			else {
				self->attribute[self->attributeIndex] = 0;
				self->state = XS_ATTRIBUTE_SPACE_STATE;
				/* continue */
			}
		case XS_ATTRIBUTE_SPACE_STATE:
			if (c == '=')
				self->state = XS_ATTRIBUTE_EQUAL_STATE;
			else if (!mxIsSpace(c))
				self->state = XS_ERROR_STATE;
			break;
		case XS_ATTRIBUTE_EQUAL_STATE:
			if (c == '"') {
				self->state = XS_ATTRIBUTE_VALUE_STATE;
				self->dataIndex = 0;
			}
			else if (!mxIsSpace(c))
				self->state = XS_ERROR_STATE;
			break;
		case XS_ATTRIBUTE_VALUE_STATE:
			if (c == '"') {
				self->data[self->dataIndex] = 0;
				KprDebugMachineParseAttribute(self, self->attribute, self->data);
				self->state = XS_START_TAG_SPACE_STATE;
			}
			else if (c == '&') {
				self->entityState = XS_ATTRIBUTE_VALUE_STATE;
				self->state = XS_ENTITY_STATE;
			}
			else if (self->dataIndex < XS_BUFFER_COUNT) {
				self->data[self->dataIndex] = c;
				self->dataIndex++;
			}
			else {
				//self->state = XS_ERROR_STATE;
			}
			break;
			
		case XS_EMPTY_TAG_STATE:
			if (c == '>') {
				KprDebugMachineParseTag(self, self->tag);
				KprDebugMachinePopTag(self, self->tag);
				self->state = XS_BODY_STATE;
			}
			else
				self->state = XS_ERROR_STATE;
			break;
			
		case XS_END_TAG_STATE:
			if (mxIsFirstLetter(c))	{
				self->state = XS_END_TAG_NAME_STATE;
				self->tag[0] = c;
				self->tagIndex = 1;
			}
			else
				self->state = XS_ERROR_STATE;
			break;
		case XS_END_TAG_NAME_STATE:
			if (mxIsNextLetter(c) && (self->tagIndex < XS_BUFFER_COUNT)) {
				self->tag[self->tagIndex] = c;
				self->tagIndex++;
				break;
			}
			else {
				self->tag[self->tagIndex] = 0;
				self->state = XS_END_TAG_SPACE_STATE;
				/* continue */
			}
		case XS_END_TAG_SPACE_STATE:
			if (c == '>') {
				KprDebugMachinePopTag(self, self->tag);
				self->state = XS_BODY_STATE;
			}
			else if (!mxIsSpace(c))
				self->state = XS_ERROR_STATE;
			break;

		case XS_PROCESSING_INSTRUCTION_STATE:
			if (c == '?')
				self->state = XS_PROCESSING_INSTRUCTION_SPACE_STATE;
			else {
				self->data[self->dataIndex] = c;
				self->dataIndex++;
			}
			break;
		case XS_PROCESSING_INSTRUCTION_SPACE_STATE:
			if (c == '>') {
				self->data[self->dataIndex] = 0;
				/* parse processing instruction */
				self->state = XS_BODY_STATE;
			}
			else
				self->state = XS_ERROR_STATE;
			break;
			
		case XS_ENTITY_STATE:
			if (c == '#') {
				self->entityNumber = 0;
				self->state = XS_ENTITY_NUMBER_STATE;
			}
			else
				self->state = XS_ERROR_STATE;
			break;
		case XS_ENTITY_NUMBER_STATE:
			if (mxIsDigit(c))
				self->entityNumber = (self->entityNumber * 10) + (c - '0');
			else if (c == ';') {
				if (self->dataIndex == XS_BUFFER_COUNT) {
					self->data[self->dataIndex] = 0;
					KprDebugMachineParseData(self, self->data);
					self->dataIndex = 0;
				}
				self->data[self->dataIndex] = (char)self->entityNumber;
				self->dataIndex++;
				self->state = self->entityState;
			}
			else
				self->state = XS_ERROR_STATE;
			break;
			
		case XS_ERROR_STATE:
			/*fprintf(stderr, "\nERROR: %c\n", c);*/
			break;
		}
		theLength--;
	}
}

void KprDebugMachineParseAttribute(KprDebugMachine self, char* theName, char* theValue)
{
	if (strcmp(theName, "flags") == 0)
		strcpy(self->flags, theValue);
	else if (strcmp(theName, "line") == 0)
		self->line = strtol(theValue, NULL, 10);
	else if (strcmp(theName, "name") == 0)
		strcpy(self->name, theValue);
	else if (strcmp(theName, "path") == 0)
		strcpy(self->path, theValue);
	else if (strcmp(theName, "value") == 0)
		strcpy(self->value, theValue);
}

void KprDebugMachineParseData(KprDebugMachine self, char* theData)
{	
	if (self->view == mxLogView) {
		if (self->path[0] && (self->line >= 0)) {
			KprDebugMachinePrintView(self, mxLogView, "%s:%d: ", self->path, self->line);
			self->path[0] = 0;
			self->line = -1;
		}
		if (FskStrLen(theData))
			KprDebugMachinePrintView(self, mxLogView, "%s", theData);
	}
}

void KprDebugMachineParseTag(KprDebugMachine self, char* theName)
{
	if (strcmp(theName, "break") == 0) {
		if (self->path[0] && (self->line >= 0))
			KprDebugMachineLoadView(self, mxFileView, self->path, self->line);
	}
	else if (strcmp(theName, "breakpoint") == 0) {
		KprDebugMachineEchoItem(self, self->view, self->path, self->line, NULL, NULL);
	}
	else if (strcmp(theName, "file") == 0) {
		KprDebugMachineEchoItem(self, self->view, self->path, 0, NULL, NULL);
	}
	else if (strcmp(theName, "frame") == 0) {
		char* p = strchr(self->name, ' ');
		if (p) p++;
		else p = self->name;
		KprDebugMachineEchoItem(self, self->view, self->path, self->line, p, self->value);
		self->row++;
	}
	else if (strcmp(theName, "local") == 0) {
		KprDebugMachineSetViewSelection(self, mxFramesView, self->value);
		KprDebugMachineSetViewTitle(self, mxLocalsView, self->name);
	}
	else if (strcmp(theName, "login") == 0) {
		self->broken = 0;
		KprDebugMachineChangeTitle(self, self->name);
	}
	else if (strcmp(theName, "node") == 0) {
		KprDebugMachineEchoNode(self, self->view, self->column, self->flags[0], self->path, self->line, self->name, self->flags + 1, self->value);
		self->row++;
	}
	else if (strcmp(theName, "property") == 0) {
		KprDebugMachineEchoProperty(self, self->view, self->column, self->flags, self->name, self->value);
		self->row++;
	}
}

void KprDebugMachinePopTag(KprDebugMachine self, char* theName)
{
	if (strcmp(theName, "xsbug") == 0) {
		self->done = 1;
	}
	else if (strcmp(theName, "log") == 0) {
		self->view = mxNoCommand;
		self->broken = 0;
	}
	else if (strcmp(theName, "break") == 0) {
		self->view = mxNoCommand;
		self->broken = 1;
	}
	else if (strcmp(theName, "breakpoints") == 0) {
		KprDebugMachineUnlockView(self, mxBreakpointsView);
		self->view = mxNoCommand;
	}
	else if (strcmp(theName, "files") == 0) {
		KprDebugMachineUnlockView(self, mxFilesView);
		self->view = mxNoCommand;
	}
	else if (strcmp(theName, "frames") == 0) {
		KprDebugMachineUnlockView(self, mxFramesView);
		self->view = mxNoCommand;
	}	
	else if (strcmp(theName, "global") == 0) {
		KprDebugMachineUnlockView(self, mxGlobalsView);
		self->view = mxNoCommand;
	}
	else if (strcmp(theName, "grammar") == 0) {
		KprDebugMachineUnlockView(self, mxGrammarsView);
		self->view = mxNoCommand;
	}
	else if (strcmp(theName, "local") == 0) {
		KprDebugMachineUnlockView(self, mxLocalsView);
		self->view = mxNoCommand;
	}	
	else if (strcmp(theName, "node") == 0) {
		self->column--;
	}
	else if (strcmp(theName, "property") == 0) {
		self->column--;
	}
}

void KprDebugMachinePushTag(KprDebugMachine self, char* theName)
{
	self->flags[0] = 0;
	self->line = -1;
	self->name[0] = 0;
	self->path[0] = 0;
	self->value[0] = 0;
	
	if (strcmp(theName, "xsbug") == 0) {
		self->broken = 1;
		self->done = 0;
	}
	if (strcmp(theName, "log") == 0) {
		self->view = mxLogView;
	}
	else if (strcmp(theName, "break") == 0) {
		self->view = mxLogView;
	}
	else if (strcmp(theName, "breakpoints") == 0) {
		self->view = mxBreakpointsView;
		KprDebugMachineLockView(self, mxBreakpointsView);
	}
	else if (strcmp(theName, "files") == 0) {
		self->view = mxFilesView;
		KprDebugMachineLockView(self, mxFilesView);
	}
	else if (strcmp(theName, "frames") == 0) {
		self->view = mxFramesView;
		KprDebugMachineLockView(self, mxFramesView);
	}
	else if (strcmp(theName, "global") == 0) {
		self->row = 0;
		self->column = -1;
		self->view = mxGlobalsView;
		KprDebugMachineLockView(self, mxGlobalsView);
	}
	else if (strcmp(theName, "grammar") == 0) {
		self->row = 0;
		self->column = -1;
		self->view = mxGrammarsView;
		KprDebugMachineLockView(self, mxGrammarsView);
	}
	else if (strcmp(theName, "local") == 0) {
		self->row = 0;
		self->column = -1;
		self->view = mxLocalsView;
		KprDebugMachineLockView(self, mxLocalsView);
	}
	else if (strcmp(theName, "node") == 0) {
		self->column++;
	}
	else if (strcmp(theName, "property") == 0) {
		self->column++;
	}
}

void KprDebugMachineChangeTitle(KprDebugMachine self, char* theTitle)
{
	FskErr err = kFskErrNone;
	
	// FskDebugStr("%s: %s", __FUNCTION__, theTitle);
	BAIL_IF_NULL(self->title = FskStrDoCopy(theTitle), err, kFskErrMemFull);
	KprDebugMachineCallbackText(self, "onMachineTitleChanged", mxNoCommand, theTitle);
bail:
	return;
}

void KprDebugMachineLockView(KprDebugMachine self, int theView)
{
	KprDebug debug = self->debug;
	xsMachine* the = debug->the;

	// FskDebugStr("%s: %d", __FUNCTION__, theView);
	if (xsTypeOf(debug->behavior) == xsUndefinedType) goto bail;
	{
		xsTry {
			xsVar(0) = xsAccess(debug->behavior);
			if (xsHasOwn(xsVar(0), xsID(self->address)))
				xsDelete(xsVar(0), xsID(self->address));
			xsVar(1) = xsNewInstanceOf(xsArrayPrototype);
			xsNewHostProperty(xsVar(0), xsID(self->address), xsVar(1), xsDefault, xsDontScript);
		}
		xsCatch {
		}
	}
bail:
	return;
}

void KprDebugMachineEchoItem(KprDebugMachine self, int theView, char* path, int line, char* name, char* value)
{
	KprDebug debug = self->debug;
	xsMachine* the = debug->the;

	if (xsTypeOf(debug->behavior) == xsUndefinedType) goto bail;
	{
		xsTry {
			xsVar(0) = xsAccess(debug->behavior);
			xsVar(1) = xsGet(xsVar(0), xsID(self->address));
			xsVar(2) = xsNewInstanceOf(xsObjectPrototype);
			if (path)
				xsNewHostProperty(xsVar(2), xsID("path"), xsString(path), xsDefault, xsDontScript);
			if (line >= 0)
				xsNewHostProperty(xsVar(2), xsID("line"), xsInteger(line), xsDefault, xsDontScript);
			if (name)
				xsNewHostProperty(xsVar(2), xsID("name"), xsString(name), xsDefault, xsDontScript);
			if (value)
				xsNewHostProperty(xsVar(2), xsID("value"), xsString(value), xsDefault, xsDontScript);
			if (xsFindResult(xsVar(1), xsID_push)) {
				(void)xsCallFunction1(xsResult, xsVar(1), xsVar(2));
			}
		}
		xsCatch {
		}
	}
bail:
	return;
}

void KprDebugMachineEchoNode(KprDebugMachine self, int theView, int column, char flags, char* path, int line, char* name, char* data, char* value)
{
	KprDebug debug = self->debug;
	xsMachine* the = debug->the;

	if (xsTypeOf(debug->behavior) == xsUndefinedType) goto bail;
	{
		xsTry {
			xsVar(0) = xsAccess(debug->behavior);
			xsVar(1) = xsGet(xsVar(0), xsID(self->address));
			xsVar(2) = xsNewInstanceOf(xsObjectPrototype);
			xsNewHostProperty(xsVar(2), xsID("column"), xsInteger(column), xsDefault, xsDontScript);
			if (flags == ' ')
				xsNewHostProperty(xsVar(2), xsID("state"), xsInteger(0), xsDefault, xsDontScript);
			else if (flags == '+')
				xsNewHostProperty(xsVar(2), xsID("state"), xsInteger(1), xsDefault, xsDontScript);
			else if (flags == '-')
				xsNewHostProperty(xsVar(2), xsID("state"), xsInteger(2), xsDefault, xsDontScript);
			else
				xsNewHostProperty(xsVar(2), xsID("state"), xsInteger(0), xsDefault, xsDontScript);
			if (path)
				xsNewHostProperty(xsVar(2), xsID("path"), xsString(path), xsDefault, xsDontScript);
			if (line >= 0)
				xsNewHostProperty(xsVar(2), xsID("line"), xsInteger(line), xsDefault, xsDontScript);
			if (name)
				xsNewHostProperty(xsVar(2), xsID("name"), xsString(name), xsDefault, xsDontScript);
			if (data)
				xsNewHostProperty(xsVar(2), xsID("data"), xsString(data), xsDefault, xsDontScript);
			if (value)
				xsNewHostProperty(xsVar(2), xsID("value"), xsString(value), xsDefault, xsDontScript);
			if (xsFindResult(xsVar(1), xsID_push)) {
				(void)xsCallFunction1(xsResult, xsVar(1), xsVar(2));
			}
		}
		xsCatch {
		}
	}
bail:
	return;
}

void KprDebugMachineEchoProperty(KprDebugMachine self, int theView, int column, char* flags, char* name, char* value)
{
	KprDebug debug = self->debug;
	xsMachine* the = debug->the;

	// FskDebugStr("%s: %d - %s = %s", __FUNCTION__, theView, theName, theValue);
	if (xsTypeOf(debug->behavior) == xsUndefinedType) goto bail;
	{
		xsTry {
			xsVar(0) = xsAccess(debug->behavior);
			xsVar(1) = xsGet(xsVar(0), xsID(self->address));
			xsVar(2) = xsNewInstanceOf(xsObjectPrototype);
			xsNewHostProperty(xsVar(2), xsID("column"), xsInteger(column), xsDefault, xsDontScript);
			flags[4] = 0;
			xsNewHostProperty(xsVar(2), xsID("flags"), xsString(flags + 1), xsDefault, xsDontScript);
			if (flags[0] == ' ')
				xsNewHostProperty(xsVar(2), xsID("state"), xsInteger(0), xsDefault, xsDontScript);
			else if (flags[0] == '+')
				xsNewHostProperty(xsVar(2), xsID("state"), xsInteger(1), xsDefault, xsDontScript);
			else if (flags[0] == '-')
				xsNewHostProperty(xsVar(2), xsID("state"), xsInteger(2), xsDefault, xsDontScript);
			else
				xsNewHostProperty(xsVar(2), xsID("state"), xsInteger(0), xsDefault, xsDontScript);
			if (name)
				xsNewHostProperty(xsVar(2), xsID("name"), xsString(name), xsDefault, xsDontScript);
			if (value)
				xsNewHostProperty(xsVar(2), xsID("value"), xsString(value), xsDefault, xsDontScript);
			if (xsFindResult(xsVar(1), xsID_push)) {
				(void)xsCallFunction1(xsResult, xsVar(1), xsVar(2));
			}
		}
		xsCatch {
		}
	}
bail:
	return;
}

void KprDebugMachineEchoView(KprDebugMachine self, int theView, char* theName, char* theValue)
{
	KprDebug debug = self->debug;
	xsMachine* the = debug->the;

	// FskDebugStr("%s: %d - %s = %s", __FUNCTION__, theView, theName, theValue);
	if (xsTypeOf(debug->behavior) == xsUndefinedType) goto bail;
	{
		xsTry {
			xsVar(0) = xsAccess(debug->behavior);
			xsVar(1) = xsGet(xsVar(0), xsID(self->address));
			xsVar(2) = xsNewInstanceOf(xsObjectPrototype);
			xsNewHostProperty(xsVar(2), xsID("name"), xsString(theName), xsDefault, xsDontScript);
			xsNewHostProperty(xsVar(2), xsID("value"), xsString(theValue), xsDefault, xsDontScript);
			if (xsFindResult(xsVar(1), xsID_push)) {
				(void)xsCallFunction1(xsResult, xsVar(1), xsVar(2));
			}
		}
		xsCatch {
		}
	}
bail:
	return;
}

#define kKprDebugMachineMaxLineCount 4096

void KprDebugMachineLoadView(KprDebugMachine self, int theView, char* thePath, int theLine)
{
	FskErr err = kFskErrNone;
	KprDebug debug = self->debug;
	xsMachine* the = debug->the;
//	FskFileInfo info;
//	FskFileMapping map = NULL;
//	FskDebugStr("%s: %d - %s:%d", __FUNCTION__, theView, thePath, theLine);
	if (xsTypeOf(debug->behavior) == xsUndefinedType) goto bail;
	xsVar(0) = xsAccess(debug->behavior);
	
	if (xsFindResult(xsVar(0), xsID("onMachineFileChanged"))) {
		(void)xsCallFunction5(xsResult, xsVar(0), xsString(self->address), xsInteger(theView), xsNull, thePath ? xsString(thePath) : xsNull, xsInteger(theLine));
	}
//	if (kFskErrNone == FskFileGetFileInfo(thePath, &info) && (kFskDirectoryItemIsFile == info.filetype)) {
//		unsigned char *data;
//		FskInt64 size;
//		BAIL_IF_ERR(err = FskFileMap(thePath, &data, &size, 0, &map));
//		xsVar(0) = xsAccess(debug->behavior);
//		
//		if (xsFindResult(xsVar(0), xsID("onMachineFileChanged"))) {
//			(void)xsCallFunction5(xsResult, xsVar(0), xsString(self->address), xsInteger(theView), xsNull, xsString(thePath), xsInteger(theLine));
//		}
//	}
bail:
//	FskFileDisposeMap(map);
	xsThrowIfFskErr(err);
}

void KprDebugMachinePrintView(KprDebugMachine self, int theView, char* theFormat, ...)
{
	va_list a_va_list;
	char aBuffer[4096];
	int aBufferCount;

	va_start(a_va_list, theFormat);
	aBufferCount = vsprintf(aBuffer, theFormat, a_va_list);
	va_end(a_va_list);
	// FskDebugStr("%s: %d - %s", __FUNCTION__, theView, aBuffer);
	KprDebugMachineCallbackText(self, "onMachineViewPrint", theView, aBuffer);
}

void KprDebugMachineSetViewSelection(KprDebugMachine self, int theView, char* theSelection)
{
//	FskDebugStr("%s: %d - %s", __FUNCTION__, theView, theSelection);
}

void KprDebugMachineSetViewTitle(KprDebugMachine self, int theView, char* theTitle)
{
	KprDebug debug = self->debug;
	xsMachine* the = debug->the;

//	FskDebugStr("%s: %d - %s", __FUNCTION__, theView, theTitle);
	if (xsTypeOf(debug->behavior) == xsUndefinedType) goto bail;
	{
		xsTry {
			xsVar(0) = xsAccess(debug->behavior);
			xsVar(1) = xsGet(xsVar(0), xsID(self->address));
			if (xsFindResult(xsVar(0), xsID("onMachineViewTitle")))
				(void)xsCallFunction3(xsResult, xsVar(0), xsString(self->address), xsInteger(theView), xsString(theTitle));
		}
		xsCatch {
		}
	}
bail:
	return;
}

void KprDebugMachineUnlockView(KprDebugMachine self, int theView)
{
	KprDebug debug = self->debug;
	xsMachine* the = debug->the;

	// FskDebugStr("%s: %d", __FUNCTION__, theView);
	if (xsTypeOf(debug->behavior) == xsUndefinedType) goto bail;
	{
		xsTry {
			xsVar(0) = xsAccess(debug->behavior);
			xsVar(1) = xsGet(xsVar(0), xsID(self->address));
			KprDebugMachineCallbackView(self, "onMachineViewChanged", theView);
		}
		xsCatch {
		}
	}
bail:
	return;
}


//--------------------------------------------------
// xs
//--------------------------------------------------

static void KPR_debug_trigger(xsMachine *the, UInt32 command);

void KPR_Debug(xsMachine *the)
{
	KprDebug self = NULL;
	UInt32 port = 0;
	xsEnterSandbox();
	if (xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
		(void)(xsFindInteger(xsArg(0), xsID_port, &port));
		// add behavior?
	}
	xsLeaveSandbox();
	xsThrowIfFskErr(KprDebugNew(&self, port));
	xsSetHostData(xsResult, self);
	self->the = the;
	self->slot = xsResult;
	self->code = the->code;
	self->behavior = xsUndefined;
	xsRemember(self->slot);
	return;
}

void KPR_debug(void *it)
{
	KprDebug self = it;
	if (self) {
		xsMachine *the = self->the;
		xsForget(self->slot);
		xsForget(self->behavior);
		KprDebugDispose(self);
	}
}

void KPR_debug_close(xsMachine *the)
{
	KprDebug self = xsGetHostData(xsThis);
	KprDebugClose(self);
}

void KPR_debug_abort(xsMachine *the)
{
	KPR_debug_trigger(the, mxAbortCommand);
}

void KPR_debug_addBreakpoint(xsMachine *the)
{
	KprDebug self = xsGetHostData(xsThis);
	KprDebugMachine machine = NULL;
	xsIntegerValue c = xsToInteger(xsArgc);
	char* path = NULL;
	SInt32 line = -1;
	if ((c >= 3) && (xsTypeOf(xsArg(0)) == xsStringType) && (xsTypeOf(xsArg(1)) == xsStringType) && (xsTypeOf(xsArg(2)) == xsIntegerType)) {
		char* address = xsToString(xsArg(0));
		path = xsToString(xsArg(1));
		line = xsToInteger(xsArg(2));
		machine = KprDebugFindMachine(self, address);
	}
	if (machine && path && (line >= 0))
		KprDebugMachineDispatchCommand(machine, mxSetBreakpointCommand, path, NULL, line);
	else
		xsThrowIfFskErr(kFskErrInvalidParameter);
}


void KPR_debug_file(xsMachine *the)
{
	KprDebug self = xsGetHostData(xsThis);
	KprDebugMachine machine = NULL;
	int command = mxNoCommand;
	char* path = NULL;
	char* value = NULL;
	xsIntegerValue line = -1;
	xsIntegerValue c = xsToInteger(xsArgc);
	xsVars(4);
	if ((c >= 2) && (xsTypeOf(xsArg(0)) == xsStringType) && (xsTypeOf(xsArg(1)) == xsIntegerType)) {
		char* address = xsToString(xsArg(0));
		command = xsToInteger(xsArg(1));
		machine = KprDebugFindMachine(self, address);
	}
	if ((c >= 4) && (xsTypeOf(xsArg(2)) == xsStringType) && (xsTypeOf(xsArg(3)) == xsIntegerType)) {
		path = FskStrDoCopy(xsToString(xsArg(2)));
		line = xsToInteger(xsArg(3));
	}
	if ((c >= 5) && (xsTypeOf(xsArg(4)) == xsStringType)) {
		value = FskStrDoCopy(xsToString(xsArg(4)));
	}
	if (machine && ((command == mxFramesView) || (command == mxFilesView) || (command == mxLogView) || (command == mxBreakpointsView))) {
		xsEnterSandbox();
		KprDebugMachineDispatchCommand(machine, command, path, value, line);
		xsLeaveSandbox();
	}
	else
		xsThrowIfFskErr(kFskErrInvalidParameter);
	FskMemPtrDispose(value);
	FskMemPtrDispose(path);
}

void KPR_debug_get_machines(xsMachine *the)
{
	KprDebug self = xsGetHostData(xsThis);
	KprDebugMachine machine = NULL;
	xsEnterSandbox();
	xsVars(1);
	{
		xsResult = xsNewInstanceOf(xsArrayPrototype);
		for (machine = self->machine; machine; machine = machine->next) {
			xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
			xsNewHostProperty(xsVar(0), xsID("address"), xsString(machine->address), xsDefault, xsDontScript);
			if (machine->title)
				xsNewHostProperty(xsVar(0), xsID("title"), xsString(machine->title), xsDefault, xsDontScript);
			xsCall1(xsResult, xsID_push, xsVar(0));
		}
	}
	xsLeaveSandbox();
}

void KPR_debug_get_behavior(xsMachine *the)
{
	KprDebug self = xsGetHostData(xsThis);
	if (xsTypeOf(self->behavior) != xsUndefinedType)
		xsResult = self->behavior;
	else
		xsResult = xsUndefined;
}

void KPR_debug_go(xsMachine *the)
{
	KPR_debug_trigger(the, mxGoCommand);
}

void KPR_debug_set_behavior(xsMachine *the)
{
	KprDebug self = xsGetHostData(xsThis);
	xsForget(self->behavior);
	if (xsTest(xsArg(0))) {
		if (xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
			self->behavior = xsArg(0);
			xsRemember(self->behavior);
		}
	}
}

void KPR_debug_removeBreakpoint(xsMachine *the)
{
	KprDebug self = xsGetHostData(xsThis);
	KprDebugMachine machine = NULL;
	xsIntegerValue c = xsToInteger(xsArgc);
	char* path = NULL;
	SInt32 line = -1;
	if ((c >= 3) && (xsTypeOf(xsArg(0)) == xsStringType) && (xsTypeOf(xsArg(1)) == xsStringType) && (xsTypeOf(xsArg(2)) == xsIntegerType)) {
		char* address = xsToString(xsArg(0));
		path = xsToString(xsArg(1));
		line = xsToInteger(xsArg(2));
		machine = KprDebugFindMachine(self, address);
	}
	if (machine && path && (line >= 0))
		KprDebugMachineDispatchCommand(machine, mxClearBreakpointCommand, path, NULL, line);
	else
		xsThrowIfFskErr(kFskErrInvalidParameter);
}

void KPR_debug_resetBreakpoints(xsMachine *the)
{
	KprDebug self = xsGetHostData(xsThis);
	KprDebugMachine machine = NULL;
	xsIntegerValue c = xsToInteger(xsArgc);
	if ((c >= 1) && (xsTypeOf(xsArg(0)) == xsStringType)) {
		char* address = xsToString(xsArg(0));
		machine = KprDebugFindMachine(self, address);
	}
	if (machine)
		KprDebugMachineDispatchCommand(machine, mxClearAllBreakpointsCommand, NULL, NULL, 0);
	else
		xsThrowIfFskErr(kFskErrInvalidParameter);
}

void KPR_debug_step(xsMachine *the)
{
	KPR_debug_trigger(the, mxStepCommand);
}

void KPR_debug_stepIn(xsMachine *the)
{
	KPR_debug_trigger(the, mxStepInCommand);
}

void KPR_debug_stepOut(xsMachine *the)
{
	KPR_debug_trigger(the, mxStepOutCommand);
}

void KPR_debug_toggle(xsMachine *the)
{
	KprDebug self = xsGetHostData(xsThis);
	KprDebugMachine machine = NULL;
	int command = mxNoCommand;
	char* path = NULL;
	char* value = NULL;
	xsIntegerValue c = xsToInteger(xsArgc);
	if ((c >= 4) && (xsTypeOf(xsArg(0)) == xsStringType) && (xsTypeOf(xsArg(1)) == xsIntegerType) && (xsTypeOf(xsArg(3)) == xsStringType)) {
		char* address = xsToString(xsArg(0));
		command = xsToInteger(xsArg(1));
		if (xsTypeOf(xsArg(2)) == xsStringType)
			path = FskStrDoCopy(xsToString(xsArg(2)));
		value = FskStrDoCopy(xsToString(xsArg(3)));
		machine = KprDebugFindMachine(self, address);
	}
	if (machine && ((command == mxLocalsView) || (command == mxGlobalsView) || (command == mxGrammarsView)) && value)
		KprDebugMachineDispatchCommand(machine, command, path, value, 0);
	else
		xsThrowIfFskErr(kFskErrInvalidParameter);
	FskMemPtrDispose(value);
	FskMemPtrDispose(path);
}

void KPR_debug_trigger(xsMachine *the, UInt32 command)
{
	KprDebug self = xsGetHostData(xsThis);
	KprDebugMachine machine = NULL;
	xsIntegerValue c = xsToInteger(xsArgc);
	if ((c >= 1) && (xsTypeOf(xsArg(0)) == xsStringType)) {
		char* address = xsToString(xsArg(0));
		machine = KprDebugFindMachine(self, address);
	}
	if (machine && (command >= mxAbortCommand) && (command <= mxStepOutCommand))
		KprDebugMachineDispatchCommand(machine, command, NULL, NULL, 0);
	else
		xsThrowIfFskErr(kFskErrInvalidParameter);
}

void KPR_system_getenv(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	if ((c >= 1) && (xsTypeOf(xsArg(0)) == xsStringType)) {
		char* variable = xsToString(xsArg(0));
		char* value = getenv(variable);
		if (value)
			xsResult = xsString(value);
	}
}
