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
#include "kprApplication.h"
#include "kprURL.h"

static FskErr KprQueryDecode(char* src, char** dst);

FskErr KprURLJoin(KprURLParts parts, char** result)
{
	FskErr err = kFskErrNone;
	UInt32 length;
	char* target;

	length = 0;
	if (parts->scheme)
		length += parts->schemeLength + 1;
	if (parts->authority)
		length += 2 + parts->authorityLength;
	if (parts->path)
		length += parts->pathLength;
	if (parts->query)
		length += 1 + parts->queryLength;
	if (parts->fragment)
		length += 1 + parts->fragmentLength;
	length++;
	bailIfError(FskMemPtrNew(length, &target));
	length = 0;
	if (parts->scheme) {
		FskMemMove(target + length, parts->scheme, parts->schemeLength);
		length += parts->schemeLength;
		target[length] = ':';
		length++;
	}
	if (parts->authority) {
		target[length] = '/';
		length++;
		target[length] = '/';
		length++;
		FskMemMove(target + length, parts->authority, parts->authorityLength);
		length += parts->authorityLength;
	}
	if (parts->path) {
		char *src = parts->path;
		UInt32 srcLength = parts->pathLength;
		char *start = target + length;
		char *dst = start;
		SInt32 dotCount = -1;
		while (srcLength) {
			if (*src == '/') {
				while ((dst > start) && (dotCount > 0)) {
					dst--;
					if (*dst == '/')
						dotCount--;
					length--;
				}
				dotCount = 0;
			}
			else if (*src == '.') {
				if (dotCount == 0)
					dotCount = 1;
				else if (dotCount == 1)
					dotCount = 2;
				else
					dotCount = -1;
			}
			else
				dotCount = -1;
			*dst = *src;
			src++;
			dst++;
			srcLength--;
			length++;
		}
	}
	if (parts->query && parts->queryLength) {
		target[length] = '?';
		length++;
		FskMemMove(target + length, parts->query, parts->queryLength);
		length += parts->queryLength;
	}
	if (parts->fragment) {
		target[length] = '#';
		length++;
		FskMemMove(target + length, parts->fragment, parts->fragmentLength);
		length += parts->fragmentLength;
	}
	target[length] = 0;
	*result = target;
bail:
	return err;
}

FskErr KprURLMerge(char* base, char* reference, char** result)
{
	FskErr err = kFskErrNone;
	KprURLPartsRecord parts;
	KprURLPartsRecord baseParts;
	char* path = NULL;
	UInt32 length;

	KprURLSplit(reference, &parts);
	if (!parts.scheme) {
		KprURLSplit(base, &baseParts);
		parts.scheme = baseParts.scheme;
		parts.schemeLength = baseParts.schemeLength;
		if (!parts.authority) {
			parts.authority = baseParts.authority;
			parts.authorityLength = baseParts.authorityLength;
			if (!parts.pathLength) {
				if (baseParts.authority && !baseParts.pathLength) {
					bailIfError(FskMemPtrNew(1, &path));
					FskMemMove(path, "/", 1);
					parts.path = path;
					parts.pathLength = 1;
				}
				else {
					parts.path = baseParts.path;
					parts.pathLength = baseParts.pathLength;
				}
				if (!parts.query) {
					parts.query = baseParts.query;
					parts.queryLength = baseParts.queryLength;
				}
			}
			else if (*parts.path != '/') {
				if (baseParts.authority && !baseParts.pathLength) {
					bailIfError(FskMemPtrNew(1 + parts.pathLength, &path));
					FskMemMove(path, "/", 1);
					FskMemMove(path + 1, parts.path, parts.pathLength);
					parts.path = path;
					parts.pathLength++;
				}
				else if (baseParts.name) {
					length = baseParts.name - baseParts.path;
					bailIfError(FskMemPtrNew(length + parts.pathLength, &path));
					FskMemMove(path, baseParts.path, length);
					FskMemMove(path + length, parts.path, parts.pathLength);
					parts.path = path;
					parts.pathLength += length;
				}
			}
		}
	}
	bailIfError(KprURLJoin(&parts, result));
bail:
	FskMemPtrDispose(path);
	return err;
}

void KprURLSplit(char* url, KprURLParts parts)
{
	char c, *p, *q;
	FskMemSet(parts, 0, sizeof(KprURLPartsRecord));
	q = p = url;
	while ((c = *p)) {
		if ((c == ':') || (c == '/') || (c == '?') || (c == '#'))
			break;
		p++;
	}
	if (c == ':') {
		parts->scheme = q;
		parts->schemeLength = p - q;
		p++;
		q = p;
		c = *p;
	}
	if (c == '/') {
		p++;
		c = *p;
		if (c == '/') {
			char *semi = NULL;
			p++;
			parts->authority = q = p;
			while ((c = *p)) {
				if (c == '/')
					break;
				if (c == ':')
					semi = p;
				if (c == '@') {
					parts->host = p + 1;
					parts->user = q;
					if (semi) {
						parts->userLength = semi - q;
						parts->password = semi + 1;
						parts->passwordLength = p - semi - 1;
						semi = NULL;
					}
					else
						parts->userLength = p - q;
				}
				p++;
			}
			parts->authorityLength = p - q;
			if (parts->authorityLength) {
				if (!parts->host)
					parts->host = q;
				if (semi) {
					parts->hostLength = semi - parts->host;
					parts->port = FskStrToNum(semi + 1);
				}
				else {
					if (parts->schemeLength) {
						if (!FskStrCompareCaseInsensitiveWithLength(parts->scheme, "https", 5))
							parts->port = 443;
						else if (!FskStrCompareCaseInsensitiveWithLength(parts->scheme, "http", 4))
							parts->port = 80;
					}
					parts->hostLength = p - parts->host;
				}
			}
			q = p;
		}
		else if (c)
			parts->name = p;
	}
	while ((c = *p)) {
		if (c == '/')
			parts->name = p + 1;
		if ((c == '?') || (c == '#'))
			break;
		p++;
	}
	parts->path = q;
	parts->pathLength = p - q;
	if (parts->name)
		parts->nameLength = p - parts->name;
	if (c == '?') {
		p++;
		parts->query = q = p;
		while ((c = *p)) {
			if (c == '#')
				break;
			p++;
		}
		parts->queryLength = p - q;
	}
	if (c == '#') {
		p++;
		parts->fragment = q = p;
		while ((c = *p)) {
			p++;
		}
		parts->fragmentLength = p - q;
	}
}

#define mxURIXDigit(X) \
	((('0' <= (X)) && ((X) <= '9')) \
		? ((X) - '0') \
		: ((('a' <= (X)) && ((X) <= 'f')) \
			? (10 + (X) - 'a') \
			: (10 + (X) - 'A')))

static char gxURIReservedSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,0,0,1,1,0,1,0,0,0,0,1,1,0,0,1,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,1,1,0,1,0,1,	/* 3x  0123456789:;<=>?  */
	 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 	/* 7X  pqrstuvwxyz{|}~   */
};

FskErr KprURLToPath(char* url, char** result)
{
	FskErr err = kFskErrNone;
	unsigned char* src;
	SInt32 size;
	unsigned char c, d;
	unsigned char* dst;

	if (FskStrCompareWithLength(url, "file://", 7)) {
#if TARGET_OS_WIN32
		if (url[0] == '/')
			*result = FskStrDoCopy(url + 1);
		else
			*result = FskStrDoCopy(url);
#else
		*result = FskStrDoCopy(url);
#endif
		bailIfNULL(*result);
		goto bail;
	}
#if TARGET_OS_WIN32
	if ((url[7] == '/') && (url[8] == '/'))
		url += 5;
	else if (url[7] == '/')
		url += 8;
	else
		url += 7; // wrong file://C:
#else
	if (url[7] == '/')
		url += 7;
	else
		return kFskErrInvalidParameter;
#endif
	src = (unsigned char*)url;
	size = 0;
	while ((c = *src++)) {
		if (c == '%') {
			c = *src++;
			if (c == 0)
				return kFskErrInvalidParameter;
			d = mxURIXDigit(c) << 4;
			c = *src++;
			if (c == 0)
				return kFskErrInvalidParameter;
			d += mxURIXDigit(c);
			if ((d < 128) && (gxURIReservedSet[(int)d]))
				size += 3;
			else
				size += 1;
		}
		else
			size += 1;
	}
	size += 1;
	bailIfError(FskMemPtrNew(size, result));
	src = (unsigned char*)url;
	dst = (unsigned char*)*result;
	while ((c = *src++)) {
		if (c == '%') {
			c = *src++;
			d = mxURIXDigit(c) << 4;
			c = *src++;
			d += mxURIXDigit(c);
			if ((d < 128) && (gxURIReservedSet[(int)d])) {
				*dst++ = *(src - 3);
				*dst++ = *(src - 2);
				*dst++ = *(src - 1);
			}
			else
				*dst++ = d;
		}
		else
			*dst++ = c;
	}
	*dst = 0;
bail:
	return err;
}

static char gxURIReservedAndUnescapedSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,	/* 2x   !"#$%&'()*+,-./  */
	 1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,	/* 3x  0123456789:;<=>?  */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static char* gxURIHexa = "0123456789ABCDEF";

FskErr KprPathToURL(char* path, char** result)
{
	FskErr err = kFskErrNone;
	unsigned char* src;
	SInt32 size;
	unsigned char c;
	unsigned char* dst;

	src = (unsigned char*)path;
#if TARGET_OS_WIN32
	if ((path[0] == '/') && (path[1] == '/'))
		size = 5;
	else if (path[1] == ':')
		size = 8;
	else
		return kFskErrInvalidParameter;
#else
	if (path[0] == '/')
		size = 7;
	else
		return kFskErrInvalidParameter;
#endif
	while ((c = *src++)) {
		if ((c < 128) && (gxURIReservedAndUnescapedSet[(int)c]))
			size += 1;
		else
			size += 3;
	}
	size += 1;
	bailIfError(FskMemPtrNew(size, result));
	src = (unsigned char*)path;
	dst = (unsigned char*)*result;
#if TARGET_OS_WIN32
	if ((path[0] == '/') && (path[1] == '/')) {
		FskStrCopy(*result, "file:");
		dst += 5;
	}
	else if (path[1] == ':') {
		FskStrCopy(*result, "file:///");
		dst += 8;
	}
#else
	FskStrCopy(*result, "file://");
	dst += 7;
#endif
	while ((c = *src++)) {
		if ((c < 128) && (gxURIReservedAndUnescapedSet[(int)c]))
			*dst++ = c;
		else {
			*dst++ = '%';
			*dst++ = gxURIHexa[c >> 4];
			*dst++ = gxURIHexa[c & 15];
		}
	}
	*dst = 0;
bail:
	return err;
}

FskErr KprQueryDecode(char* src, char** dst)
{
	FskErr err = kFskErrNone;
	unsigned char *p, *q, c, d;
	SInt32 length;

	p = (unsigned char*)src;
	length = 0;
	while ((c = *p++)) {
		if (c == '%') {
			c = *p++;
			if (c == 0)
				return kFskErrInvalidParameter;
			c = *p++;
			if (c == 0)
				return kFskErrInvalidParameter;
		}
		length++;
	}
    length++;
	if (length == (p - (unsigned char*)src))
		*dst = src;
	else {
		bailIfError(FskMemPtrNew(length, dst));
		p = (unsigned char*)src;
		q = (unsigned char*)*dst;
		while ((c = *p++)) {
			if (c == '%') {
				c = *p++;
				d = mxURIXDigit(c) << 4;
				c = *p++;
				d += mxURIXDigit(c);
				*q++ = d;
			}
			else
				*q++ = c;
		}
		*q = 0;
	}
bail:
	return err;
}

FskErr KprQueryParse(char* query, FskAssociativeArray* it)
{
	FskErr err = kFskErrNone;
	FskAssociativeArray array;
	char *name, *decodedName;
	char *value, *decodedValue;
	char c;

	array = FskAssociativeArrayNew();
	bailIfNULL(array);
	name = query;
	value = NULL;
	while ((c = *query++)) {
		if (c == '=') {
			value = query;
		}
		else if (c == '&') {
			if (value)
				*(value - 1) = 0;
			*(query - 1) = 0;
			if (kFskErrNone == KprQueryDecode(name, &decodedName)) {
				if (value) {
					if (kFskErrNone == KprQueryDecode(value, &decodedValue)) {
						FskAssociativeArrayElementSet(array, (const char*)decodedName, (const char*)decodedValue, 0, kFskStringType);
						if (decodedValue != value)
							FskMemPtrDispose(decodedValue);
					}
				}
				else
					FskAssociativeArrayElementSet(array, (const char*)decodedName, "", 0, kFskStringType);
				if (decodedName != name)
					FskMemPtrDispose(decodedName);
			}
			*(query - 1) = '&';
			if (value)
				*(value - 1) = '=';
			name = query;
			value = NULL;
		}
	}
	if (value)
		*(value - 1) = 0;
	if (kFskErrNone == KprQueryDecode(name, &decodedName)) {
		if (value) {
			if (kFskErrNone == KprQueryDecode(value, &decodedValue)) {
				FskAssociativeArrayElementSet(array, (const char*)decodedName, (const char*)decodedValue, 0, kFskStringType);
				if (decodedValue != value)
					FskMemPtrDispose(decodedValue);
			}
		}
		else
			FskAssociativeArrayElementSet(array, (const char*)decodedName, "", 0, kFskStringType);
		if (decodedName != name)
			FskMemPtrDispose(decodedName);
	}
	if (value)
		*(value - 1) = '=';
	*it = array;
bail:
	return err;
}

FskErr KprQuerySerialize(FskAssociativeArray array, char** it)
{
	FskErr err = kFskErrNone;
	FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(array);
	SInt32 length = 0;
	char* p;
	while (iterate) {
		if (length)
			length++;
		length += FskStrLen(iterate->name);
		length++;
		length += FskStrLen(iterate->value);
	}
	length++;
	FskAssociativeArrayIteratorDispose(iterate);
	iterate = NULL;
	bailIfError(FskMemPtrNew(length, it));
	iterate = FskAssociativeArrayIteratorNew(array);
	p = *it;
	*p = 0;
	while (iterate) {
		if (*p)
			*p++ = '&';
		FskStrCopy(p, iterate->name);
		p += FskStrLen(iterate->name);
		*p++ = '=';
		FskStrCopy(p, iterate->value);
		p += FskStrLen(iterate->value);
	}
	*p = 0;
	FskAssociativeArrayIteratorDispose(iterate);
bail:
	return err;
}

FskErr KprAuthorityReverse(char* id, char** di)
{
	FskErr err = kFskErrNone;
	char *p, *q;
	SInt32 c = FskStrLen(id), i;
	bailIfError(FskMemPtrNew(c + 1, di));
	p = id + c;
	q = *di;
	i = 0;
	while (c) {
		c--;
		p--;
		if (*p == '.') {
			p++;
			FskMemCopy(q, p, i);
			q += i;
			*q++ = '.';
			p--;
			i = 0;
		}
		else
			i++;
	}
	FskMemCopy(q, p, i);
	q += i;
	*q = 0;
bail:
	return err;
}

/* ECMASCRIPT */
#ifndef KPR_NO_GRAMMAR

void KPR_launchURI(xsMachine *the)
{
	FskErr err;
	xsStringValue uri = xsToString(xsArg(0));
	xsStringValue path = NULL;
	if (FskStrCompareWithLength(uri, "file://", 7))
		err = FskLauncherOpenDocument(uri, 1);
	else {
		err = KprURLToPath(uri, &path);
		if (!err) {
			err = FskLauncherOpenDocument(path, 0);
			FskMemPtrDispose(path);
		}
	}
	xsThrowIfFskErr(err);
}

void KPR_mergeURI(xsMachine *the)
{
	xsStringValue base = xsToString(xsArg(0));
	xsStringValue url = xsToString(xsArg(1));
	xsStringValue target;
	xsThrowIfFskErr(KprURLMerge(base, url, &target));
	xsResult = xsString(target);
	FskMemPtrDispose(target);
}

void KPR_parseURI(xsMachine *the)
{
	xsStringValue url = xsToString(xsArg(0));
	KprURLPartsRecord parts;
	xsStringValue target;
	char c;
	xsThrowIfFskErr(FskMemPtrNew(FskStrLen(url) + 1, &target));
	FskStrCopy(target, url);
	KprURLSplit(target, &parts);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	if (parts.scheme) {
		c = *(parts.scheme + parts.schemeLength);
		*(parts.scheme + parts.schemeLength) = 0;
		xsNewHostProperty(xsResult, xsID_scheme, xsString(parts.scheme), xsDefault, xsDontScript);
		*(parts.scheme + parts.schemeLength) = c;
	}
	if (parts.authority) {
		c = *(parts.authority + parts.authorityLength);
		*(parts.authority + parts.authorityLength) = 0;
		xsNewHostProperty(xsResult, xsID_authority, xsString(parts.authority), xsDefault, xsDontScript);
		*(parts.authority + parts.authorityLength) = c;
	}
	if (parts.path) {
		c = *(parts.path + parts.pathLength);
		*(parts.path + parts.pathLength) = 0;
		xsNewHostProperty(xsResult, xsID_path, xsString(parts.path), xsDefault, xsDontScript);
		*(parts.path + parts.pathLength) = c;
	}
	if (parts.name) {
		c = *(parts.name + parts.nameLength);
		*(parts.name + parts.nameLength) = 0;
		xsNewHostProperty(xsResult, xsID_name, xsString(parts.name), xsDefault, xsDontScript);
		*(parts.name + parts.nameLength) = c;
	}
	if (parts.query) {
		c = *(parts.query + parts.queryLength);
		*(parts.query + parts.queryLength) = 0;
		xsNewHostProperty(xsResult, xsID_query, xsString(parts.query), xsDefault, xsDontScript);
		*(parts.query + parts.queryLength) = c;
	}
	if (parts.fragment) {
		c = *(parts.fragment + parts.fragmentLength);
		*(parts.fragment + parts.fragmentLength) = 0;
		xsNewHostProperty(xsResult, xsID_fragment, xsString(parts.fragment), xsDefault, xsDontScript);
		*(parts.fragment + parts.fragmentLength) = c;
	}
	FskMemPtrDispose(target);
}

void KPR_serializeURI(xsMachine *the)
{
	KprURLPartsRecord parts;
	xsStringValue target;
	FskMemSet(&parts, 0, sizeof(KprURLPartsRecord));
	xsEnterSandbox();
	if (xsFindString(xsArg(0), xsID_scheme, &parts.scheme))
		parts.schemeLength = FskStrLen(parts.scheme);
	if (xsFindString(xsArg(0), xsID_authority, &parts.authority))
		parts.authorityLength = FskStrLen(parts.authority);
	if (xsFindString(xsArg(0), xsID_path, &parts.path))
		parts.pathLength = FskStrLen(parts.path);
	if (xsFindString(xsArg(0), xsID_query, &parts.query))
		parts.queryLength = FskStrLen(parts.query);
	if (xsFindString(xsArg(0), xsID_fragment, &parts.fragment))
		parts.fragmentLength = FskStrLen(parts.fragment);
	xsLeaveSandbox();
	xsThrowIfFskErr(KprURLJoin(&parts, &target));
	xsResult = xsString(target);
	FskMemPtrDispose(target);
}

#endif /* KPR_NO_GRAMMAR */
