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
txBoolean fxSerializeContent(txMachine* the, txSerializer* theSerializer, txSlot* theProperty, 
		txSlot* theNode, txID theParent)
{
	txSlot* aNode;
	
	if (theNode->kind == XS_NODE_KIND) {
		mxZeroSlot(--the->stack);
		the->stack->kind = theNode->kind;
		the->stack->value = theNode->value;
		aNode = theNode->value.node.link->next;
		while (aNode) {
			if (fxSerializeContent(the, theSerializer, theProperty, aNode, theParent)) {
				fxSerializeStop(the, theSerializer);
				the->stack++;
				return 1;
			}
			aNode = aNode->next;
		}
		the->stack++;
	}
	else if (theParent != XS_NO_ID) {
		if ((theNode->kind == XS_JUMP_RULE) && (theNode->value.rule.data->alias == theParent)) {
			fxSerializeNode(the, theSerializer, theProperty, theNode, 1);
			return 1;
		}
	}
	else {
		if (theNode->kind != XS_JUMP_RULE) {
			fxSerializeNode(the, theSerializer, theProperty, theNode, 1);
			return 1;
		}
	}
	return 0;
}

txBoolean fxSerializeContents(txMachine* the, txSerializer* theSerializer, txSlot* theProperty, 
		txSlot* theNode)
{
	txSlot* aSlot;
	txID aParent;
	txSlot* aNode;
	
	aSlot = theNode->value.rule.data->link->next;
	if (mxIsReference(theProperty)) {
		aNode = fxGetInstance(the, theProperty);
		aParent = aNode->ID;
		aNode = theNode->next;
		while (aNode != aSlot) {
			if (fxSerializeContent(the, theSerializer, theProperty, aNode, aParent))
				return 1;
			aNode = aNode->next;
		}
	}
	aNode = theNode->next;
	while (aNode != aSlot) {
		if (fxSerializeContent(the, theSerializer, theProperty, aNode, XS_NO_ID))
			return 1;
		aNode = aNode->next;
	}
	return 0;
}

void fxSerializeGrammar(txMachine* the, txSerializer* theSerializer, txSlot* theInstance, 
		txSlot* theGrammar)
{
	txSlot* aNode;
	txSlot* aProperty;
	txInteger aLength;
	txInteger anIndex;
	txSlot* anItem;
	
	aNode = theGrammar->next;
	while (aNode) {
		switch (aNode->kind) {
		case XS_ATTRIBUTE_RULE:
			aProperty = fxSerializeRule(the, theInstance, aNode);
			if (aProperty) {
				fxSerializeStart(the, theSerializer, XS_ATTRIBUTE_FLAG);
				fxSerializeText(the, theSerializer, " ");
				fxSerializeTag(the, theSerializer, aNode, -1);
				fxSerializeText(the, theSerializer, "=\"");
				fxSerializeLiteral(the, theSerializer, aProperty, aNode, XS_ATTRIBUTE_LITERAL);
				fxSerializeText(the, theSerializer, "\"");
			}
			break;
		}
		aNode = aNode->next;
	}
	
	aNode = theGrammar->next;
	while (aNode) {
		switch (aNode->kind) {
		case XS_NODE_KIND:			
			mxZeroSlot(--the->stack);
			the->stack->kind = aNode->kind;
			the->stack->value = aNode->value;
			fxSerializeGrammar(the, theSerializer, theInstance, aNode->value.node.link);
			fxSerializeStop(the, theSerializer);
			the->stack++;
			break;

		case XS_DATA_RULE:
			aProperty = fxSerializeRule(the, theInstance, aNode);
			if (aProperty) {
				fxSerializeStart(the, theSerializer, XS_DATA_FLAG);
				fxSerializeLiteral(the, theSerializer, aProperty, aNode, XS_DATA_LITERAL);
			}
			break;
		case XS_PI_RULE:
			aProperty = fxSerializeRule(the, theInstance, aNode);
			if (aProperty) {
				fxSerializeStart(the, theSerializer, XS_CONTENTS_FLAG);
				fxSerializeText(the, theSerializer, "<?");
				fxSerializeTag(the, theSerializer, aNode, -1);
				fxSerializeText(the, theSerializer, " ");
				fxSerializeLiteral(the, theSerializer, aProperty, aNode, XS_PI_LITERAL);
				fxSerializeText(the, theSerializer, "?>");
			}
			break;
		case XS_JUMP_RULE:
			mxZeroSlot(--the->stack);
			the->stack->kind = aNode->kind;
			the->stack->value = aNode->value;
			
			aProperty = fxSerializeRule(the, theInstance, aNode);
			if (aProperty && mxIsReference(aProperty))
				fxSerializeGrammar(the, theSerializer, fxGetInstance(the, aProperty), 
						aNode->value.rule.data->link);
			
			fxSerializeStop(the, theSerializer);
			the->stack++;
			break;
			
			
		case XS_REFER_RULE:
			aProperty = fxSerializeRule(the, theInstance, aNode);
			if (aProperty)
				fxSerializeContents(the, theSerializer, aProperty, aNode);
			aNode = aNode->value.rule.data->link;
			break;
		case XS_REPEAT_RULE:
			aProperty = fxSerializeRule(the, theInstance, aNode);
			if (aProperty && mxIsReference(aProperty)) {
				aProperty = fxGetInstance(the, aProperty);
				if (aProperty->next->kind == XS_ARRAY_KIND) {
					aProperty = aProperty->next;
					aLength = aProperty->value.array.length;
					for (anIndex = 0; anIndex < aLength; anIndex++) {
						anItem = aProperty->value.array.address + anIndex;
						if (anItem->ID)
							fxSerializeContents(the, theSerializer, anItem, aNode);
					}
				}
				else {
					anItem = fxGetProperty(the, aProperty, the->lengthID);
					if (anItem) {
						aLength = fxToInteger(the, anItem);
						for (anIndex = 0; anIndex < aLength; anIndex++) {
							anItem = fxGetProperty(the, aProperty, (txID)anIndex);
							if (anItem)
								fxSerializeContents(the, theSerializer, anItem, aNode);
						}
					}
				}
			}
			aNode = aNode->value.rule.data->link;
			break;
		}
		aNode = aNode->next;
	}
}

void fxSerializeEntity(txMachine* the, txSerializer* theSerializer, char* theText, txFlag theFlag)
{
	char *aStart, *aStop;
	char aBuffer[7];
	unsigned char aChar;
	static char sEscape[256] = {
	/*  0 1 2 3 4 5 6 7 8 9 A B C D E F */
		0,0,0,0,0,0,0,0,0,2,2,0,0,2,0,0,	/* 0x                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
		1,1,2,1,1,1,0,2,1,1,1,1,1,1,1,1,	/* 2x   !"#$%&'()*+,-./  */
		1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,	/* 3x  0123456789:;<=>?  */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 	/* 7X  pqrstuvwxyz{|}~   */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 8X                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 9X                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* AX                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* BX                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* CX                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* FX                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* EX                    */
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 	/* FX                    */
	};
	aStart = aStop = theText;
	while ((aChar = *((unsigned char *)theText))) {
		theText++;
		switch (sEscape[aChar]) {
		case 1:
			aStop++;
			break;
		case 2:
			if (!theFlag) {
				aStop++;
				break;
			}
			/* continue */
		default:
			if (aStop > aStart) {
				*aStop = '\0';
				fxSerializeText(the, theSerializer, aStart);
				*((unsigned char *)aStop) = aChar;
			}
			switch (aChar) {
			case '"':
				fxSerializeText(the, theSerializer, "&quot;");
				break;
			case '&':
				fxSerializeText(the, theSerializer, "&amp;");
				break;
			case '\'':
				fxSerializeText(the, theSerializer, "&apos;");
				break;
			case '<':
				fxSerializeText(the, theSerializer, "&lt;");
				break;
			case '>':
				fxSerializeText(the, theSerializer, "&gt;");
				break;
			default:
				aStart = aBuffer;
				*(aStart++) = '&';
				*(aStart++) = '#';
				if (aChar >= 10)
					*(aStart++) = '0' + (aChar / 10);
				aChar %= 10;
				*(aStart++) = '0' + aChar;
				*(aStart++) = ';';
				*aStart = 0;
				fxSerializeText(the, theSerializer, aBuffer);
				break;
			}
			aStart = ++aStop;
			break;
		}
	}			
	if (aStop > aStart)
		fxSerializeText(the, theSerializer, aStart);
}

void fxSerializeLiteral(txMachine* the, txSerializer* theSerializer, txSlot* theProperty, txSlot* theNode, txFlag theFlag)
{
	mxZeroSlot(--the->stack);
	the->stack->value = theProperty->value;
	the->stack->kind = theProperty->kind;
	mxZeroSlot(--the->stack);
	the->stack->value.reference = theSerializer->root;
	the->stack->kind = XS_REFERENCE_KIND;
	mxZeroSlot(--the->stack);
	the->stack->value.integer = theFlag;
	the->stack->kind = XS_INTEGER_KIND;
	mxZeroSlot(--the->stack);
	the->stack->value.integer = 3;
	the->stack->kind = XS_INTEGER_KIND;
	mxZeroSlot(--the->stack);
	the->stack->value.alias = theNode->value.rule.data->alias;
	the->stack->kind = XS_ALIAS_KIND;
	fxCallID(the, the->serializeID);
	if (the->stack->kind == XS_STRING_KIND) {
		switch (theFlag) {
		case XS_DATA_LITERAL:
			if (c_strncmp(the->stack->value.string, "<![CDATA[", 9) == 0)
				fxSerializeText(the, theSerializer, the->stack->value.string);
			else
				fxSerializeEntity(the, theSerializer, the->stack->value.string, theFlag);
			break;
		case XS_ATTRIBUTE_LITERAL:
			fxSerializeEntity(the, theSerializer, the->stack->value.string, theFlag);
			break;
		case XS_PI_LITERAL:
			fxSerializeText(the, theSerializer, the->stack->value.string);
			break;
		}
	}
	else if (mxIsReference(the->stack)) {

		txSlot* instance = fxGetInstance(the, the->stack);

		if (mxIsChunk(instance)) {

			static char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			txSlot* aSlot;
			txInteger aLength;
			txU1* src;
			txU1* dst;
			txU1* str;
			txU1 byte0, byte1, byte2;

			aSlot = instance->next;
			src = (txU1 *)aSlot->value.host.data;
			aSlot = aSlot->next;
			aLength = aSlot->value.integer;
			dst = c_malloc((((aLength + 2) / 3) * 4) + 1);
			str = dst;
			while (aLength > 2) {
				byte0 = *src++;
				byte1 = *src++;
				byte2 = *src++;
				*dst++ = base64[((byte0 & 0xFC) >> 2)];
				*dst++ = base64[(((byte0 & 0x03) << 4) | ((byte1 & 0xF0) >> 4))];
				*dst++ = base64[(((byte1 & 0x0F) << 2) | ((byte2 & 0xC0) >> 6))];
				*dst++ = base64[(byte2 & 0x3F)];
				aLength -= 3;
			}
			if (aLength == 2) {
				byte0 = *src++;
				byte1 = *src++;
				*dst++ = base64[((byte0 & 0xFC) >> 2)];
				*dst++ = base64[(((byte0 & 0x03) << 4) | ((byte1 & 0xF0) >> 4))];
				*dst++ = base64[((byte1 & 0x0F) << 2)];
				*dst++ = '=';
			}
			else if (aLength == 1) {
				byte0 = *src++;
				*dst++ = base64[((byte0 & 0xFC) >> 2)];
				*dst++ = base64[((byte0 & 0x03) << 4)];
				*dst++ = '=';
				*dst++ = '=';
			}
			*dst = 0;

			fxSerializeText(the, theSerializer, (txString)str);
			c_free(str);
		}
	}
	else
		fxSerializeText(the, theSerializer, "?");
	the->stack++;
}

void fxSerializeNode(txMachine* the, txSerializer* theSerializer, txSlot* theProperty, txSlot* theNode, txBoolean exist)
{
	switch (theNode->kind) {
	case XS_NODE_KIND:
		mxZeroSlot(--the->stack);
		the->stack->kind = theNode->kind;
		the->stack->value = theNode->value;
		fxSerializeNode(the, theSerializer, theProperty, theNode->value.node.link->next, exist);
		fxSerializeStop(the, theSerializer);
		the->stack++;
		break;

	case XS_DATA_RULE:
		fxSerializeStart(the, theSerializer, XS_DATA_FLAG);
		fxSerializeLiteral(the, theSerializer, theProperty, theNode, XS_DATA_LITERAL);
		break;
	case XS_PI_RULE:
		fxSerializeStart(the, theSerializer, XS_CONTENTS_FLAG);
		fxSerializeText(the, theSerializer, "<?");
		fxSerializeTag(the, theSerializer, theNode, -1);
		fxSerializeText(the, theSerializer, " ");
		fxSerializeLiteral(the, theSerializer, theProperty, theNode, XS_PI_LITERAL);
		fxSerializeText(the, theSerializer, "?>");
		break;
	case XS_JUMP_RULE:
		mxZeroSlot(--the->stack);
		the->stack->kind = theNode->kind;
		the->stack->value = theNode->value;
		if (mxIsReference(theProperty)) {
			if (exist)
				fxSerializeStart(the, theSerializer, XS_ATTRIBUTE_FLAG);
			fxSerializeGrammar(the, theSerializer, fxGetInstance(the, theProperty), 
					theNode->value.rule.data->link);
		}
		fxSerializeStop(the, theSerializer);
		the->stack++;
		break;
	}
}

void fxSerializeRoot(txMachine* the, txSerializer* theSerializer)
{
	txSlot* aSlot;
	txSlot* grammars;
	txSlot* anInstance;
	txSlot* aNode;

	aSlot = fxGetProperty(the, mxGlobal.value.reference, the->grammarsID);
	grammars = fxGetInstance(the, aSlot);
	if (!(grammars->next))
		mxDebug0(the, XS_SYNTAX_ERROR, "serialize: no roots");

	fxToInstance(the, the->stack);
	anInstance = fxGetInstance(the, the->stack);
	aSlot = grammars->next;
	while (aSlot) {
		if (anInstance->ID == aSlot->value.reference->ID)
			break;
		aSlot = aSlot->next;
	}
	if (!aSlot)
		mxDebug0(the, XS_TYPE_ERROR, "serialize: no such root");

	theSerializer->root = aSlot->value.reference;
	aNode = theSerializer->root->next;
	fxSerializeText(the, theSerializer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	fxSerializeNode(the, theSerializer, the->stack, aNode, 1);
	fxSerializeText(the, theSerializer, "\n");
}

txSlot* fxSerializeRule(txMachine* the, txSlot* theInstance, txSlot* theRule)
{
	txSlot* aProperty;
	txInteger aCount = theRule->value.rule.data->count;
	txID* anID = theRule->value.rule.data->IDs;
	while (aCount > 1) {
		aProperty = fxGetOwnProperty(the, theInstance, *anID);
		if (aProperty && mxIsReference(aProperty))
			theInstance = fxGetInstance(the, aProperty);
		else
			break;
		aCount--;
		anID++;
	}	
	{/*
		txSlot* aSymbol = fxGetSymbol(the, *anID);
		if (aSymbol)
			fprintf(stderr, "fxSerializeRule %8.8X %8.8X %s\n", theInstance, theInstance->next, 
					aSymbol->value.symbol.string);
		else
			fprintf(stderr, "fxSerializeRule %8.8X %8.8X ?\n", theInstance, theInstance->next);
	*/}
	if ((aCount == 1) && (*anID != XS_NO_ID))
		aProperty = fxGetOwnProperty(the, theInstance, *anID);
	else
		aProperty = C_NULL;
	return aProperty;
}

void fxSerializeStart(txMachine* the, txSerializer* theSerializer, int theState)
{
	txSlot* aStack;
	int aCount;
	
	aStack = theSerializer->stack - 1;
	while (aStack > the->stack) {
		if (aStack->flag == XS_NO_FLAG) {
			fxSerializeText(the, theSerializer, "\n");
			aCount = theSerializer->stack - aStack;
			while (--aCount)
				fxSerializeText(the, theSerializer, "\t");
			fxSerializeText(the, theSerializer, "<");
			fxSerializeTag(the, theSerializer, aStack, 1);
			fxSerializeText(the, theSerializer, ">");
		}
		else if (aStack->flag == XS_ATTRIBUTE_FLAG) {
			fxSerializeText(the, theSerializer, ">");
		}
		aStack->flag = XS_CONTENTS_FLAG;
		aStack--;
	}
	if (aStack->flag == XS_NO_FLAG) {
		fxSerializeText(the, theSerializer, "\n");
		aCount = theSerializer->stack - aStack;
		while (--aCount)
			fxSerializeText(the, theSerializer, "\t");
		fxSerializeText(the, theSerializer, "<");
		fxSerializeTag(the, theSerializer, aStack, 1);
		aStack->flag = XS_ATTRIBUTE_FLAG;
	}
	if ((aStack->flag == XS_ATTRIBUTE_FLAG) && (theState != XS_ATTRIBUTE_FLAG))
		fxSerializeText(the, theSerializer, ">");
	if (theState == XS_CONTENTS_FLAG) {
		fxSerializeText(the, theSerializer, "\n");
		aCount = theSerializer->stack - the->stack + 1;
		while (--aCount)
			fxSerializeText(the, theSerializer, "\t");
	}
	aStack->flag = theState;
}

void fxSerializeStop(txMachine* the, txSerializer* theSerializer)
{
	txSlot* aStack;
	int aCount;
	
	aStack = the->stack;
	if (aStack->flag == XS_CONTENTS_FLAG) {
		fxSerializeText(the, theSerializer, "\n");
		aCount = theSerializer->stack - aStack;
		while (--aCount)
			fxSerializeText(the, theSerializer, "\t");
		fxSerializeText(the, theSerializer, "</");
		fxSerializeTag(the, theSerializer, aStack, 0);
		fxSerializeText(the, theSerializer, ">");
	}
	else if (aStack->flag == XS_DATA_FLAG) {
		fxSerializeText(the, theSerializer, "</");
		fxSerializeTag(the, theSerializer, aStack, 0);
		fxSerializeText(the, theSerializer, ">");
	}
	else if (aStack->flag == XS_ATTRIBUTE_FLAG) {
		fxSerializeText(the, theSerializer, "/>");
	}
}

void fxSerializeTag(txMachine* the, txSerializer* theSerializer, txSlot* theNode, txByte xmlns)
{
	txString aPrefix;
	txSlot* aSymbol;
	txSlot* aSlot;

	if (theNode->value.node.part.namespaceID != XS_NO_ID) {
		aPrefix = fxSearchPrefix(the, theSerializer->root, theNode->value.node.part.namespaceID);
		if (aPrefix) {
			fxSerializeText(the, theSerializer, aPrefix);
			fxSerializeText(the, theSerializer, ":");
		}
	}
	aSymbol = fxGetSymbol(the, theNode->value.node.part.nameID);
	if (aSymbol)
		fxSerializeText(the, theSerializer, aSymbol->value.symbol.string);
	else
		fxSerializeText(the, theSerializer, "?");
	if (xmlns && ((theSerializer->stack - theNode) == 1)) {
		aSlot = theSerializer->root->next->next;
		while (aSlot && (aSlot->kind == XS_PREFIX_KIND)) {
			fxSerializeText(the, theSerializer, " xmlns");
			aPrefix = aSlot->value.prefix.string;
			if (aPrefix) {
				fxSerializeText(the, theSerializer, ":");
				fxSerializeText(the, theSerializer, aPrefix);
			}
			fxSerializeText(the, theSerializer, "=\"");
			aSymbol = fxGetSymbol(the, aSlot->value.prefix.part.namespaceID);
			if (aSymbol)
				fxSerializeText(the, theSerializer, aSymbol->value.symbol.string);
			else
				fxSerializeText(the, theSerializer, "?");
			fxSerializeText(the, theSerializer, "\"");
			aSlot = aSlot->next;
		}
	}
}

void fxSerializeText(txMachine* the, txSerializer* theSerializer, txString theText)
{
	if (theSerializer->putter) 
		theSerializer->putter(theText, theSerializer->stream);
	else {
		txInteger aLength = c_strlen(theText);
		if (theSerializer->buffer && (theSerializer->offset + aLength <= theSerializer->size))
			c_memcpy(theSerializer->buffer + theSerializer->offset, theText, aLength);
		theSerializer->offset += aLength;
	}
}
