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

#include "SDP.h"
#include "FskUtilities.h"
#include "FskNetUtils.h"
#include "FskResolver.h"

FskInstrumentedSimpleType(RTSPSDP, rtspsdp);

char *nextToken(char *token, char *src);

FskErr SDPSessionDescriptionNewFromMemory(UInt8 *buffer, UInt32 bufferLen, SDPSessionDescription *descOut)
{
	FskErr err = 0;
	SDPSessionDescription desc = 0;
	UInt8 *q, *p, *end;
	char *line = 0;
	char *parts[20];
	SDPMediaDescription mediaDescription = 0;

	err = FskMemPtrNewClear(sizeof(SDPSessionDescriptionRecord), &desc);
	BAIL_IF_ERR(err);

	err = FskMemPtrNew(sizeof(SDPAttributeListRecord), (FskMemPtr*)&desc->sessionAttributes);
	BAIL_IF_ERR(err);
	desc->sessionAttributes->head = NULL;

	err = FskMemPtrNew(sizeof(SDPMediaDescriptionListRecord), (FskMemPtr*)&desc->mediaDescriptions);
	BAIL_IF_ERR(err);
	desc->mediaDescriptions->head = NULL;

	FskRTSPSDPPrintfDebug("***** SDP *****");

	// Parse-o-rama
	q = p = buffer;
	end = buffer + bufferLen;
	while (true) {
		char token[1024], *tokenPtr;
		UInt32 len;
		SDPAttribute attribute;
		UInt16 i, nParts;

		// Skip past the line ends
		while ((*q == '\r' || *q == '\n') && ((UInt32)q < (UInt32)end))
			++q;

		if ((UInt32)end == (UInt32)q)
			break;

		p = q;

		// Grab a line
		while (*q != '\r' && *q != '\n' && ((UInt32)q < (UInt32)end))
			++q;

		len = (UInt32)q - (UInt32)p;
		FskMemPtrNewClear(len + 1, &line);
		FskMemMove(line, p, len);

		FskRTSPSDPPrintfDebug(line);

		// Parse the line
		switch(line[0]) {
			case 'v':
				desc->version = FskStrToNum(&line[2]);
				break;

			case 'o':
				splitToken(&line[2], &nParts, ' ', &parts[0]);
				if (nParts > 0)
					desc->origin.username = FskStrDoCopy(parts[0]);
				if (nParts > 1)
					desc->origin.sessionID = FskStrDoCopy(parts[1]);
				if (nParts > 3)
					desc->origin.version = FskStrDoCopy(parts[2]);
				if (nParts > 4) {
					if (0 == FskStrCompare("IP4", parts[4])) {
						if (nParts > 5) {
							desc->origin.addressStr = FskStrDoCopy(parts[5]);
						
							// Don't fail if we cannot resolve this address.  In some cases (Orb), the
							// address string points at a local PC host name that cannot be resolved
							// over the net.  In the Orb case, the connection to the host has already
							// been established and therefore this address isn't used anyway
							if (0 != FskNetHostnameResolve(parts[5], (int *)&desc->origin.address)) {
							}
						}
					}
				}
				break;

			case 's':
				desc->name = FskStrDoCopy(&line[2]);
				
				// Darwin servers deliver the name prefixed with the server path
				// Detect this and truncate the name to the media file name
				if (desc->name[0] == '/') {
					char *p;
					p =	FskStrRChr(desc->name, '/');
					if (NULL != p) {
						p = FskStrDoCopy(p + 1);
						FskMemPtrDispose(desc->name);
						desc->name = p;
					}
				}
				break;

			case 't':
				splitToken(&line[2], &nParts, ' ', &parts[0]);
				desc->time.start = FskStrToNum(parts[0]);
				desc->time.stop = FskStrToNum(parts[1]);
				break;

			case 'c':
				splitToken(&line[2], &nParts, ' ', &parts[0]);
				if (0 != FskStrCompare("IP4", parts[1])) {
					err = kFskErrRTSPBadSDPParam;
					goto bail;
				}
				splitToken(parts[2], &nParts, '/', &parts[4]);
				if (0 != FskNetHostnameResolve(parts[4], (int *)&desc->connection.address)) {
					err = kFskErrNameLookupFailed;
					goto bail;
				}
				if (FskStrCompare(parts[4], "0.0.0.0") != 0)
					desc->connection.addressStr = FskStrDoCopy(parts[4]);
				if (nParts > 1)
					desc->connection.ttl = FskStrToNum(parts[5]);
				if (nParts > 2)
					desc->connection.nAddresses = FskStrToNum(parts[6]);
				break;

			case 'm':
				FskMemPtrNewClear(sizeof(SDPMediaDescriptionRecord), &mediaDescription);
				FskListAppend(&desc->mediaDescriptions->head, mediaDescription);
				mediaDescription->nPorts = 1;
				tokenPtr = nextToken(token, &line[2]);
				mediaDescription->mediaName = FskStrDoCopy(token);
				tokenPtr = nextToken(token, tokenPtr);
				splitToken(token, &nParts, '/', &parts[0]);
				mediaDescription->port = FskStrToNum(parts[0]);
				if (nParts > 1)
					mediaDescription->nPorts = FskStrToNum(parts[1]);
				tokenPtr = nextToken(token, tokenPtr);
				mediaDescription->transport = FskStrDoCopy(token);
				if (*tokenPtr) {
					err = FskMemPtrNew(sizeof(SDPMediaFormatListRecord), (FskMemPtr*)&mediaDescription->formatList);
					BAIL_IF_ERR(err);
					mediaDescription->formatList->head = NULL;
					splitToken(tokenPtr, &nParts, ' ', &parts[0]);
					for (i = 0; i < nParts; ++i) {
						SDPMediaFormat mediaFormat;
						err = FskMemPtrNewClear(sizeof(SDPMediaFormatRecord), &mediaFormat);
						BAIL_IF_ERR(err);
						mediaFormat->payloadType = FskStrToNum(parts[i]);
						FskListAppend(&mediaDescription->formatList->head, mediaFormat);
					}
				}
				break;

			case 'a':
			case 'b':
			{
				char *attrib, *value = 0, *separator;
				err = FskMemPtrNewClear(sizeof(SDPAttributeRecord), &attribute);
				BAIL_IF_ERR(err);

				// don't use the tokenizer since there's only one separator
				attrib = &line[2];
				separator = FskStrChr(attrib, ':');
				if (separator) {
					if ((UInt32)(separator + 1 - attrib) <= FskStrLen(attrib))
						value = separator + 1;
					*separator = 0;
				}
				attribute->attribute = FskStrDoCopy(attrib);
				if (value)
					attribute->value = FskStrDoCopy(value);

				if (mediaDescription) {
					// media attributes
					if (0 == mediaDescription->mediaAttributes) {
						err = FskMemPtrNew(sizeof(SDPAttributeListRecord), (FskMemPtr*)&mediaDescription->mediaAttributes);
						BAIL_IF_ERR(err);
						mediaDescription->mediaAttributes->head = NULL;
					}
					FskListAppend(&mediaDescription->mediaAttributes->head, attribute);
				}
				else {
					// session attributes
					FskListAppend(&desc->sessionAttributes->head, attribute);
				}
				break;
			}
		}

		FskMemPtrDisposeAt(&line);
	}

bail:
	FskMemPtrDispose(line);
	
	if (0 != err) {
		SDPSessionDescriptionDispose(desc);
		desc = 0;
	}
	*descOut = desc;
	return err;
}

static SDPAttribute SDPFindAttributeWithException(SDPAttributeList list, char *attributeName, char *exceptVal)
{
	SDPAttribute item;

	item = (SDPAttribute)list->head;
	while (0 != item) {
		// if exceptVal is specified, ensure that the value is not equal to exceptVal
		// otherwise, just return the first attribute found
		if (0 == FskStrCompare(attributeName, item->attribute) &&
			(!exceptVal || 0 != FskStrCompare(exceptVal, item->value)))
			return item;
		item = item->next;
	}
	return 0;
}

static SDPAttribute SDPFindAttribute(SDPAttributeList list, char *attributeName)
{
	return SDPFindAttributeWithException(list, attributeName, NULL);
}

SDPAttribute SDPFindSessionAttribute(SDPSessionDescription desc, char *attributeName)
{
	return SDPFindAttribute(desc->sessionAttributes, attributeName);
}

SDPAttribute SDPFindMediaAttribute(SDPMediaDescription desc, char *attributeName)
{
	return SDPFindAttribute(desc->mediaAttributes, attributeName);
}

SDPAttribute SDPFindMediaAttributeWithException(SDPMediaDescription desc, char *attributeName, char *exceptVal)
{
	return SDPFindAttributeWithException(desc->mediaAttributes, attributeName, exceptVal);
}

SDPMediaDescription SDPFindMediaDescription(SDPSessionDescription desc, char *mediaName)
{
	SDPMediaDescriptionList mediaDescriptions;
	SDPMediaDescription item;

	mediaDescriptions = desc->mediaDescriptions;
	item = (SDPMediaDescription)mediaDescriptions->head;
	while (0 != item) {
		if (0 == FskStrCompare(mediaName, item->mediaName))
			return item;
		item = item->next;
	}
	return 0;
}

FskErr SDPSessionDescriptionDispose(SDPSessionDescription desc)
{
	FskErr err = 0;

	if (NULL == desc) goto bail;

	FskMemPtrDispose(desc->name);
	FskMemPtrDispose(desc->information);
	FskMemPtrDispose(desc->origin.username);
	FskMemPtrDispose(desc->origin.sessionID);
	FskMemPtrDispose(desc->origin.version);
	FskMemPtrDispose(desc->origin.addressStr);
	FskMemPtrDispose(desc->connection.addressStr);
	if (NULL != desc->sessionAttributes) {
		SDPAttribute attribute;
		while ((attribute = FskListRemoveFirst(&desc->sessionAttributes->head)) != NULL) {
			FskMemPtrDispose(attribute->attribute);
			FskMemPtrDispose(attribute->value);
			FskMemPtrDispose(attribute);
		}
		FskMemPtrDispose(desc->sessionAttributes);
	}

	if (NULL != desc->mediaDescriptions) {
		SDPMediaDescription mediaDescription;
		while ((mediaDescription = FskListRemoveFirst(&desc->mediaDescriptions->head)) != NULL) {
			FskMemPtrDispose(mediaDescription->mediaName);
			FskMemPtrDispose(mediaDescription->connection);
			FskMemPtrDispose(mediaDescription->transport);
			if (NULL != mediaDescription->formatList) {
				SDPMediaFormat mediaFormat;
				while ((mediaFormat = FskListRemoveFirst(&mediaDescription->formatList->head)) != NULL) {
					FskMemPtrDispose(mediaFormat);
				}
				FskMemPtrDispose(mediaDescription->formatList);
			}
			if (NULL != mediaDescription->mediaAttributes) {
				SDPAttribute attribute;
				while ((attribute = FskListRemoveFirst(&mediaDescription->mediaAttributes->head)) != NULL) {
					FskMemPtrDispose(attribute->attribute);
					FskMemPtrDispose(attribute->value);
					FskMemPtrDispose(attribute);
				}
				FskMemPtrDispose(mediaDescription->mediaAttributes);
			}
			FskMemPtrDispose(mediaDescription);
		}
		FskMemPtrDispose(desc->mediaDescriptions);
	}
	
	FskMemPtrDispose(desc);

bail:
	return err;
}

// Split token
void splitToken(char *token, UInt16 *nParts, char splitChar, char **parts)
{
	char *tmp, *p;
	UInt16 n = 0;

	p = token;
	
	while (0 != (tmp = FskStrChr(p, splitChar))) {
		*tmp = 0;
		if (*p) // handle consecutive separators
			parts[n++] = p;
		p = tmp + 1;
	}
	if (*p)
		parts[n++] = p;
	*nParts = n;
}

char *copyAttributeValue(char *hdr, char *name, char **value)
{
	char *p, *start, *found = 0;

	// Fast way, case sensitive
	p = FskStrStr(hdr, name);

	// Slow way, case insensitive
	if (0 == p) {
		char *q, *name2 = FskStrDoCopy(name);
		UInt16 i, hdrLen = (UInt16)FskStrLen(hdr), nameLen = (UInt16)FskStrLen(name);
		for (i = 0, q = hdr; i < hdrLen; ++i, ++q) {
			if (0 == FskStrCompareCaseInsensitiveWithLength(q, name2, nameLen)) {
				p = q;
				break;
			}
		}
		FskMemPtrDispose(name2);
	}

	if (0 != p) {
		UInt16 len;
		
		// Some attributes don't have values, in which case just return "1"
		p += FskStrLen(name);
		if (!*p || (*p == ' ')) {
			BAIL_IF_ERR(FskMemPtrNew(2, &found));
			found[0] = '1';
			found[1] = 0;
			goto bail;
		}
		++p;
		start = p;
		while (*p && (*p != ';'))
			++p;
		len = (UInt16)(p - start);
		
		BAIL_IF_ERR(FskMemPtrNew(len + 1, &found));
		FskStrNCopy(found, start, len);
		found[len] = 0;
	}
	
bail:
	*value = found;
	return found;
}

// Next token
char *nextToken(char *token, char *src)
{
	return FskStrCopyUntil(token, src, ' ');
}

// Cheap bit reader
unsigned long getBits(bitStream bits, int count)
{
	unsigned long result = 0;

	while (count--) {
		result <<= 1;
		if (*(bits->data) & (1 << (7 - bits->position)))
			result |= 1;
		bits->position += 1;
		if (8 == bits->position) {
			bits->position = 0;
			bits->data += 1;
		}
	}

	return result;
}
