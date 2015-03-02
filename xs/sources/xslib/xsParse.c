/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/

void fxProcessAttribute(txMachine* the, txSlot* theInstance, txSlot* theGrammar, 
		txID theNamespaceID, txString theName, txString theValue)
{
	txSlot* aSymbol;
	txSlot* aNode;
	txSlot* aProperty;
	txID anID;

	aSymbol = fxFindSymbol(the, theName);
	if (!aSymbol)
		goto bail;
	aNode = theGrammar->next;
	while (aNode) {
		if ((aNode->kind == XS_ATTRIBUTE_RULE) 
				&& (aNode->value.node.part.namespaceID == theNamespaceID) 
				&& (aNode->value.node.part.nameID == aSymbol->ID))
			break;
		aNode = aNode->next;
	}	
	if ((!aNode) && (theNamespaceID == XS_NO_ID)) {
		aNode = theGrammar->next;
		while (aNode) {
			if ((aNode->kind == XS_ATTRIBUTE_RULE)
					&& (aNode->value.node.part.nameID == aSymbol->ID)) 
				break;
			aNode = aNode->next;
		}
		if (aNode) {
			aSymbol = fxGetSymbol(the, aNode->value.node.part.namespaceID);
			fxReportGrammarError(the, "attribute %s instead of {%s}:%s", theName, aSymbol->value.symbol.string, theName);
		}
	}
	if (!aNode)
		goto bail;
	aProperty = fxProcessRule(the, theInstance, aNode, &anID);
	if (fxGetOwnProperty(the, aProperty, anID)) {
		if (theNamespaceID == XS_NO_ID)
			fxReportGrammarError(the, "redundant attribute: %s", theName);
		else {
			aSymbol = fxGetSymbol(the, theNamespaceID);
			fxReportGrammarError(the, "redundant attribute: {%s}:%s", aSymbol->value.symbol.string, theName);
		}
	}
	aProperty = fxSetProperty(the, aProperty, anID, C_NULL);
	
	fxProcessLiteral(the, theValue, aNode, aProperty);
	return;
bail:
	if (theNamespaceID == XS_NO_ID)
		fxReportGrammarError(the, "attribute not found: %s", theName);
	else {
		aSymbol = fxGetSymbol(the, theNamespaceID);
		fxReportGrammarError(the, "attribute not found: {%s}:%s", aSymbol->value.symbol.string, theName);
	}
}

void fxProcessAttributes(txMachine* the, txSlot* theInstance, txSlot* theGrammar, 
		txID theNamespaceID, txSlot* theAttributes) 
{
	txSlot* aSlot;
	txSlot* anAttribute;
	txSlot* aName;
	txSlot* aValue;
	char* aString;
	char* aColon;
	txID aNamespaceID;

	aSlot = theAttributes->next;
	while (aSlot) {
		anAttribute = aSlot->value.reference;
		aName = fxGetOwnProperty(the, anAttribute, the->nameID);
		aValue = fxGetOwnProperty(the, anAttribute, the->valueID);
		aString = aName->value.string;
					
	#ifdef mxXSC
		if (c_strncmp(aString, "pattern:", 8) == 0) {
			mxZeroSlot(--the->stack);
			the->stack->value.string = fxNewChunk(the, c_strlen(aString) - 8 + 1);
			c_strcpy(the->stack->value.string, aName->value.string + 8);
			the->stack->kind = XS_STRING_KIND;
			*(--the->stack) = *aValue;
			fxInteger(the, --the->stack, 2);
			mxZeroSlot(--the->stack);
			the->stack->value.reference = theInstance;
			the->stack->kind = XS_REFERENCE_KIND;
			fxCallID(the, fxID(the, "addPattern"));
			the->stack++;
		}
		else
	#endif	
	
		if (c_strncmp(aString, "xmlns", 5) != 0) {
			aColon = c_strchr(aString, ':');
			if (aColon) {
				aNamespaceID = fxSearchNamespace(the, aString, aColon - aString);
				aString = aColon + 1;
			}
			else
				aNamespaceID = XS_NO_ID;
			fxProcessAttribute(the, theInstance, theGrammar, 
					aNamespaceID, aString, aValue->value.string);				
		}
			
		aSlot = aSlot->next;
	}
}

void fxProcessLiteral(txMachine* the, txString theValue, txSlot* theNode, txSlot* theProperty)
{
	mxZeroSlot(--the->stack);
	the->stack->value.string = theValue;
	the->stack->kind = XS_STRING_KIND;
	*(--the->stack) = *mxArgv(0);
	*(--the->stack) = *mxArgv(1);
	*(--the->stack) = *mxArgv(2);
	mxZeroSlot(--the->stack);
	the->stack->value.integer = 4;
	the->stack->kind = XS_INTEGER_KIND;
	mxZeroSlot(--the->stack);
	the->stack->value.alias = theNode->value.rule.data->alias;
	the->stack->kind = XS_ALIAS_KIND;
	fxCallID(the, the->parseID);
	theProperty->value = the->stack->value;
	theProperty->kind = the->stack->kind;
	the->stack++;
}

void fxProcessNamespaces(txMachine* the) 
{
	txSlot* aProperty;
	txSlot* aTag;
	txSlot* anAttribute;
	txSlot* aName;
	txSlot* aValue;
	txSlot* aSymbol;

	aTag = mxArgv(0)->value.reference;
	aProperty = fxGetOwnProperty(the, aTag, the->valueID);
	if (mxIsReference(aProperty)) {
		aProperty = aProperty->value.reference->next;
		while (aProperty) {
			anAttribute = aProperty->value.reference;
			aName = fxGetOwnProperty(the, anAttribute, the->nameID);
			
			aValue = fxGetOwnProperty(the, anAttribute, the->valueID);
			if (c_strncmp(aName->value.string, "xmlns", 5) == 0) {
				if (c_strlen(aValue->value.string) == 0)
					aValue->value.integer = XS_NO_ID;
				else {
					aSymbol = fxFindSymbol(the, aValue->value.string);
					if (!aSymbol) {
						fxReportGrammarError(the, "namespace not found: %s", aValue->value.string);
						aSymbol = fxNewSymbol(the, aValue);
					}
					aValue->value.integer = aSymbol->ID;
				}
				aValue->kind = XS_INTEGER_KIND;
			}
			aProperty = aProperty->next;
		}
	}
}

void fxProcessPI(txMachine* the) 
{
	txSlot* aPI;
	txSlot* aSlot;
	txSlot* aTag;
	txSlot* anInstance;
	txSlot* aGrammar;
	txString aName;
	txString aColon;
	txID aNamespaceID;
	txSlot* aSymbol;
	txSlot* aNode;
	txSlot* aProperty;
	txID anID;
	
	aPI = mxArgv(0)->value.reference;
	aSlot = fxGetOwnProperty(the, aPI, the->parentID);
	aTag = aSlot->value.reference;
	aSlot = fxGetOwnProperty(the, aTag, the->instanceID);
	if (!aSlot) 
		return;
	anInstance = aSlot->value.reference;
	aSlot = fxGetOwnProperty(the, aTag, the->grammarID);
	aGrammar = aSlot->value.reference;
	
	aSlot = fxGetOwnProperty(the, aPI, the->nameID);
	aName = aSlot->value.string;
	
	aColon = c_strchr(aName, ':');
	if (aColon) {
		aNamespaceID = fxSearchNamespace(the, aName, aColon - aName);
		aName = aColon + 1;
	}
	else
		aNamespaceID = fxSearchNamespace(the, NULL, 0);
	aSymbol = fxFindSymbol(the, aName);
	if (!aSymbol)
		goto bail;

	aNode = aGrammar->next;
	while (aNode) {
		if ((aNode->kind == XS_PI_RULE) 
				&& (aNode->value.node.part.namespaceID == aNamespaceID) 
				&& (aNode->value.node.part.nameID == aSymbol->ID))
			break;
		aNode = aNode->next;
	}	
	if ((!aNode) && (aNamespaceID == XS_NO_ID)) {
		aNode = aGrammar->next;
		while (aNode) {
			if ((aNode->kind == XS_PI_RULE)
					&& (aNode->value.node.part.nameID == aSymbol->ID)) 
				break;
			aNode = aNode->next;
		}
		if (aNode) {
			aSymbol = fxGetSymbol(the, aNode->value.node.part.namespaceID);
			fxReportGrammarError(the, "pi %s instead of {%s}:%s", aName, aSymbol->value.symbol.string, aName);
		}
	}
	if (!aNode)
		goto bail;
		
	aProperty = fxProcessRule(the, anInstance, aNode, &anID);
	if (anID == XS_NO_ID)
		aProperty = fxQueueItem(the, aProperty);
	else if (anID & 0x8000)
		aProperty = fxSetProperty(the, aProperty, anID, C_NULL);
	else
		aProperty = mxResult;
	
	aSlot = fxGetOwnProperty(the, aPI, the->valueID);
	fxProcessLiteral(the, aSlot->value.string, aNode, aProperty);
	return;

bail:	
	if (c_strcmp(aName, "xml"))
		fxReportGrammarError(the, "pi not found: %s", aName);
}

void fxProcessRoots(txMachine* the)
{
	txSlot* aSlot;
	txSlot* grammars;
	txSlot* aRule;
	txInteger aCount;
	txSlot* anArgument;

	aSlot = fxGetProperty(the, mxGlobal.value.reference, the->grammarsID);
	grammars = fxGetInstance(the, aSlot);
	if (!(grammars->next))
		mxDebug0(the, XS_SYNTAX_ERROR, "parse: no roots");

	fxNewInstance(the);
	
	mxZeroSlot(--the->stack);
	fxQueueID(the, the->parentID, XS_GET_ONLY);
	
	mxZeroSlot(--the->stack);
	fxQueueID(the, the->nameID, XS_GET_ONLY);
	
	mxZeroSlot(--the->stack);
	fxQueueID(the, the->valueID, XS_GET_ONLY);

	fxNewInstance(the);
	aRule = the->stack->value.reference;
	aCount = mxArgc - 3;
	if (aCount) {
		anArgument = mxArgv(3);
		while (aCount) {
			if (anArgument->kind != XS_ALIAS_KIND)
				mxDebug0(the, XS_TYPE_ERROR, "parse: no prototype");
			aSlot = grammars->next;
			while (aSlot) {
				if (anArgument->value.alias == aSlot->value.reference->ID)
					break;
				aSlot = aSlot->next;
			}
			if (!aSlot)
				mxDebug0(the, XS_TYPE_ERROR, "parse: no such root");				
			aRule->next = fxDuplicateSlot(the, aSlot->value.reference->next);
			aRule = aRule->next;
			aCount--;
			anArgument--;
		}
	}
	else {
		aSlot = grammars->next;
		while (aSlot) {
			aRule->next = fxDuplicateSlot(the, aSlot->value.reference->next);
			aRule = aRule->next;
			aSlot = aSlot->next;
		}
	}
	fxQueueID(the, the->grammarID, XS_GET_ONLY);

	mxZeroSlot(--the->stack);
	fxQueueID(the, the->instanceID, XS_GET_ONLY);

	mxArgv(0)->value.reference = the->stack->value.reference;
	mxArgv(0)->kind = the->stack->kind;
	the->stack++;
}

txSlot* fxProcessRule(txMachine* the, txSlot* theInstance, txSlot* theRule, txID* theID)
{
	txSlot* aProperty = theInstance;
	txInteger aCount = theRule->value.rule.data->count;
	txID* anID = theRule->value.rule.data->IDs;
	while (aCount > 1) {
		aProperty = fxGetProperty(the, aProperty, *anID);
		aProperty = aProperty->value.reference;
		aCount--;
		anID++;
	}	
	*theID = *anID;
	return aProperty;
}

void fxProcessStartTag(txMachine* the) 
{
	txSlot* aTag;
	txSlot* aSlot;
	txString aName;
	txSlot* aParent;
	txSlot* anInstance;
	txSlot* aGrammar;
	txString aColon;
	txID aNamespaceID;
	txSlot* aSymbol;
	txSlot* aNode;
	txKind aKind;
	txSlot* aProperty;
	txID anID;
	txFlag aFlag;
	
	aTag = mxArgv(0)->value.reference;
	aSlot = fxGetOwnProperty(the, aTag, the->nameID);
	aName = aSlot->value.string;
	
	aSlot = fxGetOwnProperty(the, aTag, the->parentID);
	if (!aSlot)
		return;
	aParent = aSlot->value.reference;
	aSlot = fxGetOwnProperty(the, aParent, the->instanceID);
	if (!aSlot)
		return;
	anInstance = aSlot->value.reference;

	fxProcessNamespaces(the);
	
	aColon = c_strchr(aName, ':');
	if (aColon) {
		aNamespaceID = fxSearchNamespace(the, aName, aColon - aName);
		aName = aColon + 1;
	}
	else
		aNamespaceID = fxSearchNamespace(the, NULL, 0);

	aSlot = fxGetOwnProperty(the, aParent, the->grammarID);
	if (!aSlot)
		goto bail;
	aGrammar = aSlot->value.reference;

	aSymbol = fxFindSymbol(the, aName);
	if (!aSymbol)
		goto bail;
		
	aNode = aGrammar->next;
	while (aNode) {
		if (((aNode->kind == XS_NODE_KIND) || (aNode->kind == XS_JUMP_RULE))
				&& (aNode->value.node.part.namespaceID == aNamespaceID) 
				&& (aNode->value.node.part.nameID == aSymbol->ID)) 
			break;
		aNode = aNode->next;
	}	
	if ((!aNode) && (aNamespaceID == XS_NO_ID)) {
		aNode = aGrammar->next;
		while (aNode) {
			if (((aNode->kind == XS_NODE_KIND) || (aNode->kind == XS_JUMP_RULE))
					&& (aNode->value.node.part.nameID == aSymbol->ID)) 
				break;
			aNode = aNode->next;
		}
		if (aNode) {
			aSymbol = fxGetSymbol(the, aNode->value.node.part.namespaceID);
			fxReportGrammarError(the, "element %s instead of {%s}:%s", aName, aSymbol->value.symbol.string, aName);
		}
	}
	if (!aNode)
		goto bail;
		
	if (aNode->kind == XS_NODE_KIND) {
		aFlag = 1;
		aGrammar = aNode->value.node.link;
	}
	else {
		aFlag = 0;
		aProperty = fxProcessRule(the, anInstance, aNode, &anID);
		if (anID == XS_NO_ID)
			aProperty = fxQueueItem(the, aProperty);
		else if (anID & 0x8000) {
			if (fxGetOwnProperty(the, aProperty, anID)) {
				if (aNamespaceID == XS_NO_ID)
					fxReportGrammarError(the, "redundant element: %s", aName);
				else {
					aSymbol = fxGetSymbol(the, aNamespaceID);
					fxReportGrammarError(the, "redundant element: {%s}:%s", aSymbol->value.symbol.string, aName);
				}
				return;
			}
			aProperty = fxSetProperty(the, aProperty, anID, C_NULL);
		}
		else {
			aProperty = mxResult;
		}
		
		mxZeroSlot(--the->stack);
		the->stack->value.alias = aNode->value.rule.data->alias;
		the->stack->kind = XS_ALIAS_KIND;
		fxNewInstanceOf(the);
		if (the->parseFlag & XS_DEBUG_FLAG) {
			*(--the->stack) = *mxArgv(1);
			fxQueueID(the, the->pathID, XS_DONT_ENUM_FLAG);
			*(--the->stack) = *mxArgv(2);
			fxQueueID(the, the->lineID, XS_DONT_ENUM_FLAG);
		}
		aProperty->value = the->stack->value;
		aProperty->kind = the->stack->kind;
		the->stack++;
		
		anInstance = aProperty->value.reference;
		aGrammar = aNode->value.rule.data->link;
	}
	
	the->hacked = 0;
	aNode = aGrammar->next;
	while (aNode) {
		aKind = aNode->kind;
		if ((aKind == XS_EMBED_RULE) || (aKind == XS_REPEAT_RULE)) {
			aProperty = fxProcessRule(the, anInstance, aNode, &anID);
			aProperty = fxSetProperty(the, aProperty, anID, C_NULL);
			mxZeroSlot(--the->stack);
			if (aKind == XS_EMBED_RULE)
				the->stack->value.alias = aNode->value.rule.data->alias;
			else
				the->stack->value.alias = mxArrayPrototype.value.alias;
			the->stack->kind = XS_ALIAS_KIND;
			fxNewInstanceOf(the);
			aProperty->value = the->stack->value;
			aProperty->kind = the->stack->kind;
			the->stack++;
		}
		else if (aKind == XS_DATA_RULE) {
			the->hacked = the->parseFlag & XS_NO_MIXTURE_FLAG;
			if (aFlag) {
				aSlot = fxSetProperty(the, aTag, the->parseID, C_NULL);
				aSlot->value.boolean = 1;
				aSlot->kind = XS_BOOLEAN_KIND;
			}
		}
		aNode = aNode->next;
	}
	
	aSlot = fxGetOwnProperty(the, aTag, the->valueID);
	fxProcessAttributes(the, anInstance, aGrammar, aNamespaceID, aSlot->value.reference);
	
	aSlot = fxSetProperty(the, aTag, the->grammarID, C_NULL);
	aSlot->value.reference = aGrammar;
	aSlot->kind = XS_REFERENCE_KIND;
	aSlot = fxSetProperty(the, aTag, the->instanceID, C_NULL);
	aSlot->value.reference = anInstance;
	aSlot->kind = XS_REFERENCE_KIND;
	return;
bail:	
	if (aNamespaceID == XS_NO_ID)
		fxReportGrammarError(the, "element not found: %s", aName);
	else {
		aSymbol = fxGetSymbol(the, aNamespaceID);
		fxReportGrammarError(the, "element not found: {%s}:%s", aSymbol->value.symbol.string, aName);
	}
}

void fxProcessStopTag(txMachine* the) 
{
	txSlot* aTag;
	txSlot* aSlot;
	txSlot* anInstance;
	txSlot* aGrammar;
	txSlot* aNode;
	txSlot* aProperty;
	txID anID;
	
	the->hacked = 0;
	aTag = mxArgv(0)->value.reference;
	aSlot = fxGetOwnProperty(the, aTag, the->instanceID);
	if (!aSlot) 
		return;
	anInstance = aSlot->value.reference;
	aSlot = fxGetOwnProperty(the, aTag, the->grammarID);
	aGrammar = aSlot->value.reference;
	aNode = aGrammar->next;
	while (aNode) {
		if (aNode->kind == XS_REPEAT_RULE) {
			aProperty = fxProcessRule(the, anInstance, aNode, &anID);
			aProperty = fxGetProperty(the, aProperty, anID);
			fxCacheArray(the, fxGetOwnInstance(the, aProperty));
		}
		aNode = aNode->next;
	}

	aSlot = fxGetOwnProperty(the, aTag, the->parseID);
	if (!aSlot || !aSlot->value.boolean)
		return;
	fxNewInstance(the);
	*(--the->stack) = *mxArgv(0);
	fxQueueID(the, the->parentID, XS_GET_ONLY);
	mxPush(mxEmptyString);
	fxQueueID(the, the->valueID, XS_GET_ONLY);
	mxArgv(0)->value.reference = the->stack->value.reference;
	mxArgv(0)->kind = the->stack->kind;
	the->stack++;
	fxProcessText(the);
	mxArgv(0)->kind = mxArgv(0)->value.reference->next->kind;
	mxArgv(0)->value.reference = mxArgv(0)->value.reference->next->value.reference;
}

void fxProcessText(txMachine* the) 
{
	txSlot* aText;
	txSlot* aSlot;
	txSlot* aTag;
	txSlot* anInstance;
	txSlot* aGrammar;
	txSlot* aNode;
	txSlot* aProperty;
	txID anID;
	
	aText = mxArgv(0)->value.reference;
	aSlot = fxGetOwnProperty(the, aText, the->parentID);
	aTag = aSlot->value.reference;
	aSlot = fxGetOwnProperty(the, aTag, the->instanceID);
	if (!aSlot) 
		return;
	anInstance = aSlot->value.reference;
	aSlot = fxGetOwnProperty(the, aTag, the->grammarID);
	aGrammar = aSlot->value.reference;

	aNode = aGrammar->next;
	while (aNode) {
		if (aNode->kind == XS_DATA_RULE) {
			aProperty = fxProcessRule(the, anInstance, aNode, &anID);
			if (anID == XS_NO_ID)
				aProperty = fxQueueItem(the, aProperty);
			else if (anID & 0x8000)
				aProperty = fxSetProperty(the, aProperty, anID, C_NULL);
			else
				aProperty = mxResult;
			aSlot = fxGetOwnProperty(the, aText, the->valueID);
			fxProcessLiteral(the, aSlot->value.string, aNode, aProperty);
			aSlot = fxGetOwnProperty(the, aTag, the->parseID);
			if (aSlot)
				aSlot->value.boolean = 0;
			return;
		}

		aNode = aNode->next;
	}	
}

void fxReportGrammarError(txMachine* the, char* theFormat, ...)
{
#define XS_NO_MASK (XS_NO_ERROR_FLAG | XS_NO_WARNING_FLAG)
	char* aPath;
	int aLine;
	c_va_list arguments;

	if ((the->parseFlag & XS_NO_MASK) == XS_NO_MASK)
		return;
	if (mxArgv(1)->kind == XS_STRING_KIND)
		aPath = mxArgv(1)->value.string;
	else
		aPath = C_NULL;
	if (mxArgv(2)->kind == XS_INTEGER_KIND)
		aLine = mxArgv(2)->value.integer;
	else
		aLine = 0;
	c_va_start(arguments, theFormat);
	if ((the->parseFlag & XS_NO_MASK) == XS_NO_ERROR_FLAG)
		fxVReportWarning(the, aPath, aLine, theFormat, arguments);
	else
		fxVReportError(the, aPath, aLine, theFormat, arguments);
	c_va_end(arguments);
	if ((the->parseFlag & XS_NO_MASK) == 0)
		mxDebug0(the, XS_SYNTAX_ERROR, "XML error");
}

txID fxSearchNamespace(txMachine* the, char* thePrefix, int theLength)
{
	txSlot* aProperty;
	txSlot* aReference;
	txSlot* aTag;
	txSlot* anAttribute;
	txSlot* aName;
	txSlot* aValue;
	char* aString;

	aReference = mxArgv(0);
	while mxIsReference(aReference) {
		aTag = aReference->value.reference;
		aProperty = fxGetOwnProperty(the, aTag, the->valueID);
		if (mxIsReference(aProperty)) {
			aProperty = aProperty->value.reference->next;
			while (aProperty) {
				anAttribute = aProperty->value.reference;
				aName = fxGetOwnProperty(the, anAttribute, the->nameID);
				aValue = fxGetOwnProperty(the, anAttribute, the->valueID);
				aString = aName->value.string;
				if (thePrefix) {
					if (c_strncmp(aString, "xmlns:", 6) == 0) 
						if ((int)c_strlen(aString + 6) == theLength) 
							if (c_strncmp(aString + 6, thePrefix, theLength) == 0)
								return (txID)aValue->value.integer;
				}
				else {
					if (c_strcmp(aString, "xmlns") == 0)
						return (txID)aValue->value.integer;
				}
				aProperty = aProperty->next;
			}
		}
		aReference = fxGetOwnProperty(the, aTag, the->parentID);
	}
	if (thePrefix)
		fxReportGrammarError(the, "prefix not found: %s", thePrefix);
	return XS_NO_ID;
}
