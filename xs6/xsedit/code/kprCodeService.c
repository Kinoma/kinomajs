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
#include "xs6Script.h"
#include "FskGrowableStorage.h"
#include "kpr.h"
#include "kprCode.h"

typedef struct {
	char *buffer;
	int offset;
	int size;
} KprCodeServiceStream;

typedef struct {
	txParser* parser;
	txInteger level;
	txInteger flags;
} txMarker;

enum {
	txMarkerClassPropertyFlag = 1,
};

static void KprCodeServiceCancel(KprService service, KprMessage message);
static int KprCodeServiceGetter(void* it);
static void KprCodeServiceInvoke(KprService service, KprMessage message);
static void KprCodeServiceReportError(void* console, txString path, txInteger line, txString format, c_va_list arguments);
static void KprCodeServiceReportWarning(void* console, txString path, txInteger line, txString format, c_va_list arguments);
static void KprCodeServiceStart(KprService self, FskThread thread, xsMachine* the);
static void KprCodeServiceStop(KprService self);
static void fxMarkerAdd(txMarker* marker, txInteger line, txString format, ...);
static void fxVMarkerAdd(txMarker* marker, txInteger line, txString format, c_va_list arguments);
static void fxParserMark(txParser* parser);
static void fxNodeMark(void* it, void* param);
static void vxsPrint(xsMachine* the, xsSlot* slot, txString format, c_va_list arguments);

KprServiceRecord gCodeService = {
	NULL,
	kprServicesThread,
	"xscode:",
	NULL,
	NULL,
	KprServiceAccept,
	KprCodeServiceCancel,
	KprCodeServiceInvoke,
	KprCodeServiceStart,
	KprCodeServiceStop,
	NULL,
	NULL,
	NULL
};

void KprCodeServiceCancel(KprService service UNUSED, KprMessage message UNUSED)
{
}

int KprCodeServiceGetter(void* it)
{
	KprCodeServiceStream* self = it;
	int result = EOF;
	if (self->offset < self->size) {
		result = *(self->buffer + self->offset);
		self->offset++;
	}
	return result;
}

void KprCodeServiceInvoke(KprService self, KprMessage message)
{
	FskErr err = kFskErrNone;
	KprCodeServiceStream stream;
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	txString name = NULL;
	if (!KprMessageContinue(message))
		return;
	stream.buffer = message->request.body;
	stream.offset = 0;
	stream.size = message->request.size;
	fxInitializeParser(parser, C_NULL, 32*1024, 1993);
	xsBeginHost(self->machine);
	{
		xsVars(4);
		{
			txScript* script = NULL;
			xsTry {
				KprMessageScriptTarget target = (KprMessageScriptTarget)message->stream;
				xsResult = xsNewInstanceOf(xsObjectPrototype);
				xsVar(0) = xsNewInstanceOf(xsArrayPrototype);
				xsNewHostProperty(xsResult, xsID("errors"), xsVar(0), xsDefault, xsDontScript);
				xsVar(1) = xsNewInstanceOf(xsArrayPrototype);
				xsNewHostProperty(xsResult, xsID("markers"), xsVar(1), xsDefault, xsDontScript);
				parser->console = the;
				parser->firstJump = &jump;
				parser->origin = parser->path = fxNewParserSymbol(parser, KprMessageGetRequestHeader(message, "path"));
				parser->reportError = KprCodeServiceReportError;
				parser->reportWarning = KprCodeServiceReportWarning;
				if (c_setjmp(jump.jmp_buf) == 0) {
					fxParserTree(parser, &stream, (txGetter)KprCodeServiceGetter, mxDebugFlag | mxCFlag, &name);
					fxParserHoist(parser);
					fxParserBind(parser);
					script = fxParserCode(parser);
					fxParserMark(parser);
					target->result = xsMarshall(xsResult);
					message->status = 200;
				}
				else {
					target->result = xsMarshall(xsResult);
					message->status = 500;
				}
			}
			xsCatch {
			}
			if (script)
				fxDeleteScript(script);
		}
	}
	xsEndHost(self->the);
	fxTerminateParser(parser);
	message->error = err;
	KprMessageComplete(message);
}

void KprCodeServiceReportError(void* console, txString path, txInteger line, txString format, c_va_list arguments)
{
	xsMachine *the = console;
	xsVar(2) = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsVar(2), xsID("line"), xsNumber(line), xsDefault, xsDontScript);
	vxsPrint(the, &xsVar(3), format, arguments);
	xsNewHostProperty(xsVar(2), xsID("reason"), xsVar(3), xsDefault, xsDontScript);
	xsCall1(xsVar(0), xsID("push"), xsVar(2));
}

void KprCodeServiceReportWarning(void* console, txString path, txInteger line, txString format, c_va_list arguments)
{
	xsMachine *the = console;
	xsVar(2) = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsVar(2), xsID("line"), xsNumber(line), xsDefault, xsDontScript);
	vxsPrint(the, &xsVar(3), format, arguments);
	xsNewHostProperty(xsVar(2), xsID("reason"), xsVar(3), xsDefault, xsDontScript);
	xsCall1(xsVar(0), xsID("push"), xsVar(2));
}

void KprCodeServiceStart(KprService self, FskThread thread, xsMachine* the)
{
	self->machine = the;
	self->thread = thread;
}

void KprCodeServiceStop(KprService self UNUSED)
{
}

void KPR_code_writeMessage(xsMachine *the)
{
	KprCode self = xsGetHostData(xsThis);
	KprMessage message = xsGetHostData(xsArg(0));
	message->request.size = FskStrLen(self->string);
	xsThrowIfFskErr(FskMemPtrNewFromData(message->request.size + 1, self->string, &message->request.body));
}

void fxMarkerAdd(txMarker* marker, txInteger line, txString format, ...)
{
	c_va_list arguments;
	c_va_start(arguments, format);
	fxVMarkerAdd(marker, line, format, arguments);
	c_va_end(arguments);
}

void fxVMarkerAdd(txMarker* marker, txInteger line, txString format, c_va_list arguments)
{
	xsMachine *the = marker->parser->console;
	xsVar(2) = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsVar(2), xsID("line"), xsNumber(line), xsDefault, xsDontScript);
	vxsPrint(the, &xsVar(3), format, arguments);
	xsNewHostProperty(xsVar(2), xsID("title"), xsVar(3), xsDefault, xsDontScript);
	xsCall1(xsVar(1), xsID("push"), xsVar(2));
}

void fxParserMark(txParser* parser)
{
	txMarker marker;
	if (parser->errorCount == 0) {
		c_memset(&marker, 0, sizeof(txMarker));
		marker.parser = parser;
		fxNodeMark(parser->root, &marker);
	}
}

void fxNodeMark(void* it, void* param) 
{
	txNode* node = it;
	txMarker* marker = param;
	switch (node->description->token) {
	case XS_TOKEN_ASSIGN:
		if ((marker->level == 0) 
				&& (((txAssignNode*)node)->reference->description->token == XS_TOKEN_LET)
				&& (((txAssignNode*)node)->value->description->token == XS_TOKEN_CLASS)
				&& ((txClassNode*)(((txAssignNode*)node)->value))->symbol)
			fxNodeMark(((txAssignNode*)node)->value, param);
		else
			(*node->description->dispatch->distribute)(node, fxNodeMark, param);
		break;
	case XS_TOKEN_CONST:
		if (marker->level == 0)
			fxMarkerAdd(marker, node->line, "const %s", ((txDeclareNode*)node)->symbol->string);
		(*node->description->dispatch->distribute)(node, fxNodeMark, param);
		break;
	case XS_TOKEN_LET: 
		if (marker->level == 0) { 
			if (((txDeclareNode*)node)->symbol == marker->parser->defaultSymbol)
				fxMarkerAdd(marker, node->line, "export default");
			else
				fxMarkerAdd(marker, node->line, "let %s", ((txDeclareNode*)node)->symbol->string);
		}
		(*node->description->dispatch->distribute)(node, fxNodeMark, param);
		break;
	case XS_TOKEN_VAR: 
		if ((marker->level == 0) && (node->line >= 0))
			fxMarkerAdd(marker, node->line, "var %s", ((txDeclareNode*)node)->symbol->string);
		(*node->description->dispatch->distribute)(node, fxNodeMark, param);
		break;
	case XS_TOKEN_EXPORT: 
		(*node->description->dispatch->distribute)(node, fxNodeMark, param);
		break;
	case XS_TOKEN_IMPORT: 
		if (((txImportNode*)node)->specifiers) {
			txSpecifierNode* specifier = (txSpecifierNode*)(((txImportNode*)node)->specifiers->first);
			while (specifier) {
				txSymbol* symbol = specifier->asSymbol ? specifier->asSymbol : specifier->symbol;
				fxMarkerAdd(marker, specifier->line, "import %s", symbol->string);
				specifier = (txSpecifierNode*)specifier->next;
			}
		}
		break;
	case XS_TOKEN_CLASS:
		if ((marker->level == 0) && ((txClassNode*)node)->symbol) {
			fxMarkerAdd(marker, node->line, "class %s", ((txClassNode*)node)->symbol->string);
			marker->flags |= txMarkerClassPropertyFlag;
		}
		marker->level++;
		(*node->description->dispatch->distribute)(node, fxNodeMark, param);
		marker->level--;
		marker->flags &= ~txMarkerClassPropertyFlag;
		break;
	case XS_TOKEN_FUNCTION:
		if ((marker->level == 0) && ((txFunctionNode*)node)->symbol)
			fxMarkerAdd(marker, node->line, "function %s", ((txFunctionNode*)node)->symbol->string);
		marker->level++;
		(*node->description->dispatch->distribute)(node, fxNodeMark, param);
		marker->level--;
		break;
	case XS_TOKEN_PROPERTY: 
		if (marker->flags & txMarkerClassPropertyFlag) {
			txPropertyNode* propertyNode = (txPropertyNode*)node;
			if (propertyNode->flags & mxGetterFlag)
				fxMarkerAdd(marker, node->line, "  get %s", propertyNode->symbol->string);
			else if (propertyNode->flags & mxSetterFlag)
				fxMarkerAdd(marker, node->line, "  set %s", propertyNode->symbol->string);
			else
				fxMarkerAdd(marker, node->line, "  %s", propertyNode->symbol->string);
		}
		(*node->description->dispatch->distribute)(node, fxNodeMark, param);
		break;
	case XS_TOKEN_PROPERTY_AT: 
		(*node->description->dispatch->distribute)(node, fxNodeMark, param);
		break;
	default: 
		(*node->description->dispatch->distribute)(node, fxNodeMark, param);
		break;
	}
}

void vxsPrint(xsMachine* the, xsSlot* slot, txString format, c_va_list arguments)
{
#if TARGET_OS_KPL
	char dummy[1024];
	vsnprintf(dummy, 1024, format, arguments);
	*slot = xsString(dummy);
#else
	char dummy[1];
	int size = vsnprintf(dummy, 1, format, arguments);
	*slot = xsStringBuffer(NULL, size);
	vsnprintf(xsToString(*slot), size + 1, format, arguments);
#endif
}

