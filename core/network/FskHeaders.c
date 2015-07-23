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
#include "FskHeaders.h"
#include "FskUtilities.h"

static FskResponseCode HTTP_Responses[]
= {
  { 100, "Continue" },
  { 101, "Switching Protocols" },
  { 200, "OK" },
  { 201, "Created" },
  { 202, "Accepted" },
  { 203, "Non-Authoritative Information" },
  { 204, "No Content" },
  { 205, "Reset Content" },
  { 206, "Partial Content" },
  { 207, "Multi-Status" },
  { 300, "Multiple Choices" },
  { 301, "Moved Permanently" },
  { 302, "Found" },
  { 303, "See Other" },
  { 304, "Not Modified" },
  { 305, "Use Proxy" },
  { 307, "Temporary Redirect" },
  { 400, "Bad Request" },
  { 401, "Unauthorized" },
  { 402, "Payment Required" },
  { 403, "Forbidden" },
  { 404, "Not Found" },
  { 405, "Method Not Allowed" },
  { 406, "Not Acceptable" },
  { 407, "Proxy Authentication Required" },
  { 408, "Request Timeout" },
  { 409, "Conflict" },
  { 410, "Gone" },
  { 411, "Length Required" },
  { 412, "Precondition Failed" },
  { 413, "Request Entity Too Large" },
  { 414, "Request-URI Too Long" },
  { 415, "Unsupported Media Type" },
  { 416, "Requested Range Not Satisfiable" },
  { 417, "Expectation Failed" },
  { 500, "Internal Server Error" },
  { 501, "Not Implemented" },
  { 502, "Bad Gateway" },
  { 503, "Service Unavailable" },
  { 504, "Gateway Timeout" },
  { 505, "HTTP Version Not Supported" },
  { -1, "" }
};

// ------------------------------------------------------------------------
FskErr FskHeaderStructNew(FskHeaders **headers) {
	FskErr		err;
	FskHeaders	*ret;

	*headers = NULL;
	err = FskMemPtrNewClear(sizeof(FskHeaders), &ret);
	if (err != kFskErrNone)
		return err;
	
	ret->theHeaders = FskAssociativeArrayNew();

	*headers = ret;
	return kFskErrNone;
}

// ------------------------------------------------------------------------
void FskHeaderStructDispose(FskHeaders *headers)
{
	if (headers == NULL)
		return;

	FskMemPtrDispose(headers->URI);
	FskMemPtrDispose(headers->filename);
	FskMemPtrDispose(headers->method);
	FskMemPtrDispose(headers->protocol);
	FskMemPtrDispose(headers->responseReasonPhrase);
	FskMemPtrDispose(headers->lastName);
	
	FskAssociativeArrayDispose(headers->parameters);
	FskAssociativeArrayDispose(headers->theHeaders);

	FskMemPtrDispose(headers);
}

// ------------------------------------------------------------------------
static int sParseStartLine(char *startLine, UInt16 headerType, FskHeaders *headers)
{
	FskErr	err;
	int		l;
	const char *p;
	char *c = startLine;
	char *firstPart;

	// Get method or protocol
	p = c;
	c = FskStrChr(c, ' ');
	if (!c) return -1;
	
	l = (c++) - p;
	err = FskMemPtrNew(l+1, &firstPart);
	if (err != kFskErrNone)
		return -1;
	FskStrNCopy(firstPart, p, l);
	firstPart[l] = '\0';
	if (kFskHeaderTypeResponse == headerType)
		headers->protocol = firstPart;
	else 
		headers->method = firstPart;

	c = FskStrStripHeadSpace(c);	// skip over space

	headers->headerType = headerType;
	if (kFskHeaderTypeResponse == headerType) {
		// Get response code and message (if message not in HTTP_Responses)
		headers->responseCode = FskStrToNum(c);
		if (headers->flags & kFskHeadersNonStandardResponseReasonPhrase) {
			c = FskStrChr(c, ' ');
			if (c) {
				char *r, *s;
				s = FskStrStripHeadSpace(c);
				r = FskFindResponse(headers->responseCode);
				if (!r || (0 != FskStrCompareCaseInsensitiveWithLength(s, r, FskStrLen(r)))) {
					headers->responseReasonPhrase = FskStrDoCopy(s);
					if (NULL != headers->responseReasonPhrase)
						FskStrStripTailSpace(headers->responseReasonPhrase);
				}
			}
		}
	}
	else {
		char 	*s, *t = NULL;
		char	*uri = NULL;

		// Get URI
		if ((*c == '/') && !(headers->flags & kFskHeadersDoNotStripURILeadingSlash))
			c++;
		s = FskStrChr(c, ' ');
		if (!s) {
			headers->responseCode = 400;
			return -1;
		}
		headers->protocol = FskStrDoCopy(s + 1);
		if (NULL != headers->protocol)
			FskStrStripTailSpace(headers->protocol);

		BAIL_IF_ERR(FskMemPtrNew((s-c)+1, &uri));
		BAIL_IF_ERR(FskMemPtrNew((s-c)+1, &t));
		FskMemCopy(uri, c, s-c);
		uri[s-c] = '\0';
		s = FskStrChr(uri, '?');
        if (s) *s = 0;
        FskStrDecodeEscapedChars(uri, t);
        if (s) {
            *s = '?';
            FskStrCat(t, s);
        }
		headers->URI = FskStrDoCopy(t);

		// Break URI into filename and parameters
		s = FskStrChr(t, '?');
		if (!s) {
			headers->filename = FskStrDoCopy(t);
		}
		else {		// URI has parameters
			*s++ = '\0';	// cap off the filename
			headers->filename = FskStrDoCopy(t);
			
			headers->parameters = FskAssociativeArrayNew();
			while (s) {
				char *name = s;		
				char *value = FskStrChr(name, '=');
				if (!value)
					break;
				s = FskStrChr(value, '&');
				*value++ = '\0';		// cap off the name
				if (s)
					*s++ = '\0';		// cap off the value
				FskAssociativeArrayElementSetString(headers->parameters, name, value);
			}
		}		

bail:
		FskMemPtrDispose(uri);
		FskMemPtrDispose(t);

	}

	return headers->headerType;
}

// ------------------------------------------------------------------------
int FskHeadersParseChunk(char *blob, int blobSize, UInt16 headerType, FskHeaders *headers)
{
	char	*line = NULL;
	int		lineMax;
	int		copySize, lineSize;
	char 	*blobPtr = blob, *pos;
	int		consumed = 0, leftoverSize = 0;
	char	*name, *value;
	Boolean withComma;
	int		consumedSize = 0;
	
	if (headers->headersParsed)
		return 0;

	if (headers->leftover) {
		leftoverSize = FskStrLen(headers->leftover);
		lineMax = blobSize + leftoverSize + 1;
		if (kFskErrNone != FskMemPtrRealloc(lineMax, &line)) {
			consumedSize = -1;
			goto bail;
		}
		FskStrCopy(line, headers->leftover);	
		FskMemPtrDisposeAt((void**)(void*)&headers->leftover);
	}
	else {
		lineMax = blobSize + 1;
		if (kFskErrNone != FskMemPtrNew(lineMax, &line)) {
			consumedSize = -1;
			goto bail;
		}
		line[0] = '\0';
	}
	
	lineSize = FskStrLen(line);
	while (blobPtr) {
		copySize = blobSize;
		pos = FskStrNChr(blobPtr, copySize, kFskLF);
		if (pos) {
			int i;
			copySize = (pos - blobPtr) + 1;
			for (i=0; i<copySize; i++) {
				if (blobPtr[i] & 0x80) {
					headers->headersParsed = true;
					consumedSize = consumed - leftoverSize;
					goto bail;
				}
				if (':' == blobPtr[i])
					break;			// high ascii allowed after the colon
			}
		}
		FskStrNCopy(&line[lineSize], blobPtr, copySize);
		line[lineSize+copySize] = '\0';

		blobPtr += copySize;
		blobSize -= copySize;
		lineSize = FskStrLen(line);

		if (((2 == lineSize) && (kFskCR == line[0]) && (kFskLF == line[1])) || ((1 == lineSize) && (kFskLF == line[0]))) {		// the LF only case is to handle known bug with yimg.com (Yahoo Image server)
			consumed += lineSize;
			headers->headersParsed = true;
			consumedSize = consumed - leftoverSize;
			goto bail;
		}

		if (!pos) {
			if (lineSize) {
				headers->leftover = FskStrDoCopy(line);
				consumed += lineSize;
			}
			consumedSize = consumed - leftoverSize;
			goto bail;
		}
		consumed += lineSize;

		if (NULL == headers->protocol) {
			if ((-1 == sParseStartLine(line, headerType, headers) || (NULL == headers->protocol))) {
				consumedSize = -1;
				goto bail;
			}
			lineSize = 0;
			line[0] = '\0';
			continue;
		}

		withComma = true;
		if (FskStrIsSpace(*line)) {
			withComma = false;
			name = headers->lastName;
			value = line;
		}
		else {
			name = FskStrStripHeadSpace(line);
			value = FskStrChr(line, ':');
		}
		if (value) {
			*value++ = '\0';
			value = FskStrStripHeadSpace(value);
			FskStrStripTailSpace(value);
		}
		if (NULL == name) {
			consumedSize = -1;
			goto bail;
		}

		FskStrStripTailSpace(name);

		if (headers->flags & kFskHeadersDoNotMergeDuplicates) { // raw headers
			FskAssociativeArrayNameList	list;
			SInt32 nameLen = FskStrLen(name) + 1;
			SInt32 valueType = kFskStringType;
			UInt32 valueSize = FskStrLen(value) + 1;

			if (NULL == value) {
				consumedSize = -1;
				goto bail;
			}

			//bailIfError(FskMemPtrNew(sizeof(FskAssociativeArrayNameListRec) + nameLen + valueSize, &list));
			if (kFskErrNone == FskMemPtrNew(sizeof(FskAssociativeArrayNameListRec) + nameLen + valueSize, &list)) {
				unsigned char *d = list->data;

				// name
				FskMemMove(d, name, nameLen);
				list->name = (char *)d;
				d += nameLen;

				// value
				FskMemMove(d, value, valueSize);
				list->value = (char *)d;
				list->valueSize = valueSize;
				list->valueType = valueType;

				list->next = NULL;
				FskListPrepend(headers->theHeaders, list);
			}
		}
		else
			FskAssociativeArrayElementCatenateString(headers->theHeaders, name, value, withComma);
		if (name != headers->lastName) {
			FskMemPtrDispose(headers->lastName);
			headers->lastName = FskStrDoCopy(name);
		}
		line[0] = '\0';
		lineSize = 0;
	}

	if (lineSize) {
		headers->leftover = FskStrDoCopy(line);
		consumed += lineSize;
	}

	consumedSize = consumed - leftoverSize;

bail:
	if (line)
		FskMemPtrDispose(line);

	return consumedSize;
}

// ------------------------------------------------------------------------
char *FskHeaderFind(char *headerName, FskHeaders *headers)
{
	return FskAssociativeArrayElementGetString(headers->theHeaders, headerName);
}

// ------------------------------------------------------------------------
void FskHeaderAddString(char *headerName, char *headerVal, FskHeaders *headers)
{
	FskAssociativeArrayElementCatenateString(headers->theHeaders, headerName, headerVal, true);
}

// ------------------------------------------------------------------------
void FskHeaderAddInteger(char *headerName, SInt32 headerVal, FskHeaders *headers)
{
	char	foo[32];

	snprintf(foo, 31, "%ld", headerVal);
	FskAssociativeArrayElementCatenateString(headers->theHeaders, headerName, foo, true);
}

// ------------------------------------------------------------------------
void FskHeaderRemove(char *headerName, FskHeaders *headers)
{
	FskAssociativeArrayElementDispose(headers->theHeaders, headerName);
}

// ------------------------------------------------------------------------
FskHeaderIterator FskHeaderIteratorNew(FskHeaders *headers) {
	return FskAssociativeArrayIteratorNew(headers->theHeaders);
}

// ------------------------------------------------------------------------
FskHeaderIterator FskHeaderIteratorNext(FskHeaderIterator iter) {
	return FskAssociativeArrayIteratorNext(iter);
}

// ------------------------------------------------------------------------
void FskHeaderIteratorDispose(FskHeaderIterator iter) {
	FskAssociativeArrayIteratorDispose(iter);
}

// ------------------------------------------------------------------------
int FskHeaderGenerateOutputLine(FskHeaderIterator iter, char *line, int lineLength)
{
	int l;
	if (!iter)
		return 0;

	l = FskStrLen(iter->name) + FskStrLen(iter->value) + 5;

	if (lineLength < l)
		return 0;
		
	if (line)
		l = snprintf(line, lineLength, "%s: %s\r\n", iter->name, iter->value);

	return l;
}

// ------------------------------------------------------------------------
int FskHeaderGenerateOutputBlob(char *blob, int blobLength, Boolean withTerminator, FskHeaders *headers)
{
	FskHeaderIterator iter;
	int blobLoc = 0, amt;

	iter = FskHeaderIteratorNew(headers);
	if (blob) {
		while ((amt = FskHeaderGenerateOutputLine(iter, &blob[blobLoc], blobLength - blobLoc)) > 0) {
			blobLoc += amt;
			iter = FskHeaderIteratorNext(iter);
		}
		if (withTerminator)
			FskStrCat(blob, "\r\n");
	}
	else {
		while ((amt = FskHeaderGenerateOutputLine(iter, NULL, blobLength - blobLoc)) > 0) {
			blobLoc += amt;
			iter = FskHeaderIteratorNext(iter);
		}
	}
	if (withTerminator)
		blobLoc += 2;

	FskHeaderIteratorDispose(iter);
	return blobLoc;
}

// ------------------------------------------------------------------------
char *FskFindResponse(int respCode)
{
	FskResponseCode *tResp = HTTP_Responses;

	while (tResp->HTTP_code != -1) {
		if (tResp->HTTP_code == respCode)
			return tResp->HTTP_description;
		tResp++;
	}

	return NULL;
}

// ------------------------------------------------------------------------
int FskHeaderResponseCode(FskHeaders *headers)
{
	return headers->responseCode;
}

// ------------------------------------------------------------------------
char *FskHeaderResponseReasonPhrase(FskHeaders *headers)
{
	return headers->responseReasonPhrase;
}

// ------------------------------------------------------------------------
int FskHeaderType(FskHeaders *headers)
{
	return headers->headerType;
}

// ------------------------------------------------------------------------
char *FskHeaderMethod(FskHeaders *headers)
{
	if (headers->headerType == kFskHeaderTypeRequest)
		return headers->method;
	else
		return 0;
}

// ------------------------------------------------------------------------
UInt32 FskHeaderHTTPVersion(FskHeaders *headers)
{
	if (headers->protocol) {
		char *pos, *tmp;
		int major = 0, minor = 0;
		tmp = FskStrDoCopy(headers->protocol);
		pos = FskStrChr(headers->protocol, '/');
		if (pos) {
			major = FskStrToNum(pos+1);
			pos = FskStrChr(pos, '.');
			if (pos)
				minor = FskStrToNum(pos+1);
		}
		FskMemPtrDispose(tmp);
		return (major << 16) | minor;
	}
	else
		return 0;
}

// ------------------------------------------------------------------------
char *FskHeaderFilename(FskHeaders *headers)
{
	if (headers->headerType == kFskHeaderTypeRequest)
		return headers->filename;
	else
		return 0;
}

// ------------------------------------------------------------------------
FskAssociativeArray FskHeaderParameters(FskHeaders *headers)
{
	if (headers->headerType == kFskHeaderTypeRequest)
		return headers->parameters;
	else
		return 0;
}


// ------------------------------------------------------------------------
char *FskHeaderURI(FskHeaders *headers)
{
	return headers->URI;
}

// ------------------------------------------------------------------------
char *FskHeaderGetParameterValue(FskHeaders *headers, const char *requestedName)
{
	return FskAssociativeArrayElementGetString(headers->parameters, requestedName);
}

