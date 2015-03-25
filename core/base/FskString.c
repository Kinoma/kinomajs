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
#include "FskMemory.h"
#include "FskPlatformImplementation.h"
#include "FskTextConvert.h"
#include "FskFiles.h"
#include "FskString.h"

#include <string.h>
#include <stdlib.h>

#if TARGET_OS_LINUX
	#include <unistd.h>
	#include <wchar.h>
#endif

#define ishex(c)	((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || \
					(c >= 'a' && c <= 'f'))
#define hexval(c)	((c <= '9') ? c - '0' : ((c <= 'F') ? c - 'A' + 10 : c - 'a' + 10))


/*
	Strings
*/

// ---------------------------------------------------------------------
char *FskStrDoCopy(const char *str)
{
	char *result;

	if (NULL == str)
		return NULL;

	if (kFskErrNone != FskMemPtrNewFromData(FskStrLen(str) + 1, str, &result))
		return NULL;

	return result;
}

#if SUPPORT_MEMORY_DEBUG

char *FskStrDoCopy_Untracked(const char *str)
{
	char *result;
	UInt32 len;

	if (NULL == str)
		return NULL;

	len = FskStrLen(str) + 1;
	if (kFskErrNone != FskMemPtrNew_Untracked(len, &result))
		return NULL;

	FskMemMove(result, str, len);

	return result;
}

#endif

// ---------------------------------------------------------------------

char *FskStrDoCat(const char *strA, const char *strB)
{
	char *result;

	if (kFskErrNone != FskMemPtrNew(FskStrLen(strA) + FskStrLen(strB) + 1, &result))
		return NULL;

	FskStrCopy(result, strA);
	FskStrCat(result, strB);

	return result;
}

// ---------------------------------------------------------------------
UInt32 FskStrLen(const char *str)
{
#if TARGET_OS_WIN32 || TARGET_OS_MAC || TARGET_OS_LINUX || TARGET_OS_KPL
	return str ? (UInt32)strlen(str) : 0;
#else
	UInt32 count = 0;

	if (NULL != str) {
		while (*str++)
			count += 1;
	}

	return count;
#endif
}

UInt32 FskUnicodeStrLen(const UInt16 *str)
{
	SInt32 count = 0;

	if (NULL != str) {
#if !FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE
		if (1 & (UInt32)str) {
			unsigned char *s = (unsigned char *)str;
			while (s[0] || s[1]) {
				s += 2;
				count += 1;
			}
		}
		else
#endif /* FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE */
			while (*str++)
				count += 1;
	}

	return count;
}

FskErr FskStrListDoCopy(const char *str, char **copy)
{
	return FskMemPtrNewFromData(FskStrListLen(str), str, (FskMemPtr *)copy);
}

UInt32 FskStrListLen(const char *str)
{
	UInt32 size = 1;

	if (NULL == str)
		return 0;

	while (*str) {
		UInt32 len = FskStrLen(str) + 1;
		size += len;
		str += len;
	}

	return size;
}

SInt32 FskStrListCompare(const char *strA, const char *strB)
{
	SInt32 result;

	if (!strA || !strB)
		return -1;		// no perfect answer here

	while (true) {
		UInt32 len;

		result = FskStrCompare(strA, strB);
		if (0 != result)
			break;

		len = FskStrLen(strA);
		if (0 == len)		// done
			break;

		strA += len + 1;
		strB += len + 1;
	}

	return result;
}

// ---------------------------------------------------------------------
void FskStrCopy(char *result, const char *str)
{
	if (str == NULL ) {
		if (result != NULL)
			*result = 0;
		return;
	}
	while (*str)
		*result++ = *str++;

	*result = 0;
}

// ---------------------------------------------------------------------
void FskStrNCopy(char *result, const char *str, UInt32 count)
{
	strncpy(result, str, count);
}

// ---------------------------------------------------------------------
void FskStrCat(char *result, const char *str)
{
	if (NULL == str)
		return;

	while (*result)
		result++;

	FskStrCopy(result, str);
}

// ---------------------------------------------------------------------
void FskStrNCat(char *result, const char *str, UInt32 len)
{
	if (NULL == str)
		return;

	while (*result)
		result++;

	while (len-- && *str)
		*result++ = *str++;

	*result = 0;
}

// ---------------------------------------------------------------------
SInt32 FskStrCompare(const char *s1, const char *s2)
{
	if ((NULL == s1) || (NULL == s2)) {
		if (s1 == s2)
			return 0;
		else
			return 1;
	}

	return strcmp(s1, s2);
}

// ---------------------------------------------------------------------
SInt32 FskStrCompareWithLength(const char *s1, const char *s2, UInt32 n)
{
	if ((NULL == s1) || (NULL == s2)) {
		if (s1 == s2)
			return 0;
		else
			return 1;
	}
	return strncmp(s1, s2, n);
}

// ---------------------------------------------------------------------
SInt32 FskStrCompareCaseInsensitive(const char *s1, const char *s2)
{
	if ((NULL == s1) || (NULL == s2)) {
		if (s1 == s2)
			return 0;
		else
			return 1;
	}
#if TARGET_OS_WIN32
	return _stricmp(s1, s2);
#elif TARGET_OS_LINUX || TARGET_OS_MACOSX || TARGET_OS_MAC || TARGET_OS_KPL
	return strcasecmp(s1, s2);
#endif
}

// ---------------------------------------------------------------------
SInt32 FskStrCompareCaseInsensitiveWithLength(const char *s1, const char *s2, UInt32 n)
{
	if ((NULL == s1) || (NULL == s2)) {
		if (s1 == s2)
			return 0;
		else
			return 1;
	}
#if TARGET_OS_WIN32
	return _strnicmp(s1, s2, n);
#elif TARGET_OS_LINUX || TARGET_OS_MACOSX || TARGET_OS_MAC || TARGET_OS_KPL
	return strncasecmp(s1, s2, n);
#endif
}

// ---------------------------------------------------------------------
char* FskStrStr(const char *s1, const char *s2)
{
	char *foo;

	foo = strstr((char *)s1, (char *)s2);

	return foo;
}

// ---------------------------------------------------------------------
char* FskStrChr(const char *s, char c)
{
	return (char *)strchr(s, c);
}

char *FskStrRChr(const char *string, char search)
{
	char *result	= (char*)string;
	char *next		= (char*)string;

	while (true) {
		char *temp = FskStrChr(next, search);
		if (!temp) break;
		result = temp;
		next = temp + 1;
	}

	if (next == string)
		return NULL;
	else
		return result;
}

// ---------------------------------------------------------------------
void FskStrNumToStr(SInt32 value, char *str, UInt32 len)
{
#if TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MACOSX || TARGET_OS_MAC || TARGET_OS_KPL
	snprintf(str, len, "%d", (int)value);
#else
	itoa(value, str, len);
#endif
}

// ---------------------------------------------------------------------
SInt32 FskStrToNum(const char *str)
{
	return atoi(str);
}

void FskStrDoubleToStr(double value, char *str, UInt32 len, const char *format)
{
    snprintf(str, len, format ? format : "%f", value);
}

// ---------------------------------------------------------------------
static int baseVal(char a, UInt32 base) {
	char convChar[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	UInt32 i;
	char up;
	up = toupper(a);
	for (i=0; i<base; i++) {
		if (convChar[i] == up)
			return i;
	}
	return -1;
}

FskInt64 FskStrToFskInt64Base(const char *str, UInt32 base) {
	int len, i, neg = 0, n;
	FskInt64 accum = 0;
	char *start;

	start = FskStrStripHeadSpace(str);
	if (start[0] == '-') {
		neg = 1;
		start++;
	}
	else if (start[0] == '+')
		start++;
	len = FskStrLen(start);
	for (i=0; i<len; i++) {
		n = baseVal(start[i], base);
		if (n == -1)
			break;
		accum *= base;
		accum += n;
	}

	if (neg)
		return -accum;
	else
		return accum;
}

FskInt64 FskStrToFskInt64(const char *str) {
	return FskStrToFskInt64Base(str, 10);
}

// ---------------------------------------------------------------------
void FskStrNumToHex(UInt32 value, char *str, UInt32 digits)
{
	UInt32 mask = 0xf, i, v;

	for (i=0; i<digits; i++) {
		v = (value & mask) >> (4 * i);
		mask <<= 4;
		v = v > 9 ? (v - 10) + 'A' : v + '0';
		str[digits - (i+1)] = (char)v;
	}
	str[digits] = '\0';
}

long FskStrToL(const char *nptr, char **endptr, UInt32 base)
{
	return strtol(nptr, endptr, base);
}

unsigned long FskStrToUL(const char *nptr, char **endptr, UInt32 base)
{
	return strtoul(nptr, endptr, base);
}

double FskStrToD(const char *nptr, char **endptr)
{
	return strtod(nptr, endptr);
}

// ---------------------------------------------------------------------
SInt32 FskStrTail(const char *str, const char *tail)
{
	int tailLen, strLen, val;

	strLen = (int)strlen(str);
	tailLen =(int)strlen(tail);

	if (strLen < tailLen)
		return -1;
	while (tailLen) {
		if ((val = (tail[tailLen-1] - str[strLen-1])) != 0)
			return val;
		tailLen--;
		strLen--;
	}
	return 0;
}

// ---------------------------------------------------------------------
char *FskStrCopyUntil(char *to, const char *from, char stopChar)
{
	char *foo = (char*)from;
	while (*foo && (*foo != stopChar))
		*to++ = *foo++;
	*to = '\0';
	if (*foo)
		foo++;
	return foo;
}

// ---------------------------------------------------------------------
char *FskStrNCopyUntilSpace(char *to, const char *from, int max)
{
	char *foo = (char*)from;
	while (max && *foo && !FskStrIsSpace(*foo)) {
		*to++ = *foo++;
		max--;
	}
	*to = '\0';
	if (*foo)
		foo++;
	return foo;
}

// ---------------------------------------------------------------------
char *FskStrNCopyUntil(char *to, const char *from, int copyMax, char stopChar)
{
	char *foo = (char*)from;
	while (*foo && copyMax && (*foo != stopChar))
		*to++ = *foo++, copyMax--;
	*to = '\0';
	if (*foo)
		foo++;
	return foo;
}

// ---------------------------------------------------------------------
char *FskStrNChr(const char *from, UInt32 len, char c)
{
	UInt32 i;
	const char *foo = from;
	for (i=0; i < len; i++)
		if (foo[i] == c)
			return (char *)(foo + i);

	return 0L;
}

// ---------------------------------------------------------------------
char *FskStrNStr(const char *from, UInt32 len, const char *str)
{
	char c = *str++;
	if (c != '\0') {
		UInt32 n = FskStrLen(str);
		const char *start = from, *stop;
		while ((stop = FskStrNChr(start, len, c)) != NULL) {
			if ((len -= stop + 1 - start) < n)
				break;
			if ((n == 0) || (FskStrCompareWithLength(stop + 1, str, n) == 0))
				return (char *)stop;
			start = stop + 1;
		}
	}
	return 0L;
}

// ---------------------------------------------------------------------
char *FskStrStripQuotes(char *a)
{
	int amt;
	amt = FskStrLen(a);
	if ((amt > 2) && (*a == '"')) {
		if (a[amt-1] == '"') {
			FskMemMove(a, a+1, amt-1);
			a[amt-2] = '\0';
		}
	}


	return (char *)a;
}

// ---------------------------------------------------------------------
char *FskStrStripHeadSpace(const char *a)
{
	while (FskStrIsSpace(*a))
		a++;

	return (char *)a;
}

// ---------------------------------------------------------------------
char *FskStrStripTailSpace(char *a)
{
	char *end = NULL, *p;

	p = a;
	while (*p) {
		if (!FskStrIsSpace(*p) && !FskStrIsCRLF(*p))
			end = p;
		p++;
	}
	if (end)
		*++end = '\0';
	else
		*a = '\0';

	return a;
}

// ---------------------------------------------------------------------
UInt32 FskStrHexToNum(const char *str, UInt32 num)
{
	UInt32 i, v = 0;
	char c;
	for (i=0; i<num; i++) {
		c = str[i];
		if (!ishex(c))
			return v;
		v <<= 4;
		v += hexval(c);
	}
	return v;
}

// ---------------------------------------------------------------------
void FskStrDecodeEscapedChars(const char *inPath, char *outPath)
{
	if (!inPath || !outPath)
		return;
	while (*inPath) {
		if (*inPath == '%') {
			char c;
			inPath++;
            if (inPath[0] && inPath[1]) {
                c = (char)FskStrHexToNum(inPath, 2);
                inPath += 2;
                *outPath++ = c;
            }
            else
                *outPath++ = '%';
		}
		else
			*outPath++ = *inPath++;
	}
	*outPath = '\0';
}

// ---------------------------------------------------------------------
#define FskStrIsPathDelim(x)	(((x) == '\\') || ((x) == '/'))

void FskCleanPath(char *path)
{
	char newpath[256];
	if (!path)
		return;

#if TARGET_OS_WIN32
	(void)_fullpath(newpath, path, 256);
#else
	{
		char *c = path;
		char *p = newpath;
		while (*c) {
			if (FskStrIsPathDelim(*c) && FskStrIsPathDelim(c[1])) {
				c++;
			}
			else
				*p++ = *c++;
		}
        *p++ = '\0';
	}
#endif
	FskStrDecodeEscapedChars(newpath, path);
}

UInt32 FskStrCountEscapedCharsToEncode(const char *inPath)
{
	const char *p = inPath;
	char c;
	UInt32 size = 0;

	// figure out how big the output string will be
	while ((c = *p++) != 0) {
		switch (c) {
			case ' ':
			case '\'':
			case '"':
			case '%':
				size += 3;
				break;

			default:
				size += 1;
				break;
		}
	}

	return size;
}

void FskStrEncodeEscapedChars(const char *inPath, char *outPath)
{
	const char *p = inPath;
	char c;
	char *dest = outPath;
	static char numToHex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	// now encode it
	while ((c = *p++) != 0) {
		switch (c) {
			case ' ':
			case '\'':
			case '"':
			case '%':
				*dest++ = '%';
				*dest++ = numToHex[(c >> 4) & 0x0f];
				*dest++ = numToHex[c & 0x0f];
				break;

			default:
				*dest++ = c;
				break;
		}
	}

	*dest = 0;
}

FskErr FskStrURLEncode(const char *inPath, char **outPath)
{
	FskErr err;
	UInt32 size;
	char *encoded;

	// figure out how big the output string will be
	size = FskStrCountEscapedCharsToEncode(inPath);

	// now encode it
	err = FskMemPtrNew(size + 1, &encoded);
	BAIL_IF_ERR(err);
	*outPath = encoded;
	FskStrEncodeEscapedChars(inPath, encoded);

bail:
	return err;
}

Boolean FskStrIsDottedQuad(char *str, int *out) {
	int num[4];
	int cur = 0;
	char *p, *s;

	for (s=p=str; *p; p++) {
		if (cur == 4)
			return false;
		if (isdigit(*p))
			continue;
		if (*p == '.') {
			if ((num[cur] = FskStrToNum(s)) > 255)
				return false;
			s = p+1;
			cur++;
		}
		else
			return false;
	}
	if (*p == '\0' && cur < 4)
		if ((num[cur] = FskStrToNum(s)) > 255)
			return false;
	if (cur == 3) {
		*out = (num[0] << 24) | (num[1] << 16) | (num[2] << 8) | num[3];
		return true;
	}
	return false;
}

static FskErr sFskStrParsedUrlRebuild(FskStrParsedUrl urlComp) {
	FskErr err = kFskErrNone;
	char *path;
	int size;

	if (urlComp->relativeUrl) {
		urlComp->url = FskStrDoCopy(urlComp->path);
	}
	else {
		path = urlComp->path;
		if (path[0] == '/')
			path++;

		if (urlComp->scheme)
			size = FskStrLen(urlComp->scheme);
		else
			size = 4;
		size += 3 + FskStrLen(urlComp->host);
		if (urlComp->specifyPort) size += 1 + 5;
		size += 1 + FskStrLen(path);

		err = FskMemPtrNew(size + 1, (FskMemPtr*)(void*)&urlComp->url);
		BAIL_IF_ERR(err);
		if (urlComp->scheme) {
			if (urlComp->specifyPort)
				snprintf(urlComp->url, size + 1, "%s://%s:%d/%s", urlComp->scheme, urlComp->host, urlComp->port, path);
			else
				snprintf(urlComp->url, size + 1, "%s://%s/%s", urlComp->scheme, urlComp->host, path);
		}
		else {
			if (urlComp->specifyPort)
				snprintf(urlComp->url, size + 1, "http://%s:%d/%s", urlComp->host, urlComp->port, path);
			else
				snprintf(urlComp->url, size + 1, "http://%s/%s", urlComp->host, path);
		}
	}
bail:
	return err;
}

FskErr FskStrParseUrl(char *url, FskStrParsedUrl *urlComponents) {
	FskErr err = kFskErrNone;
	FskStrParsedUrl urlComp;
	char *p, *at, *colon, *portStr, *slash;
	int portNum = 80;
	Boolean ours = true;

	if (NULL == url || NULL == urlComponents)
		return kFskErrParameterError;

	urlComp = *urlComponents;
	if (NULL == urlComp) {
		err = FskMemPtrNew(sizeof(FskStrParsedUrlRecord), &urlComp);
		BAIL_IF_ERR(err);
	}
	else {
		ours = false;		// don't dispose urlComp on failure
		FskMemPtrDisposeAt((void**)(void*)&urlComp->data); // chuck existing
		FskMemPtrDisposeAt((void**)(void*)&urlComp->url);  // chuck existing
	}
	FskMemSet(urlComp, '\0', sizeof(FskStrParsedUrlRecord));
	urlComp->data = FskStrDoCopy(url);

	if (url[0] == '/') {
		// relative URL? - skip scheme, user/pass, host, port
		urlComp->relativeUrl = true;
		p = urlComp->data + 1;
	}
	else {
		p = FskStrStr(urlComp->data, "://");
		if (p) {
			*p++ = '\0';
			urlComp->scheme = urlComp->data;
			if (0 == FskStrCompare("https", urlComp->scheme))
				portNum = 443;
			else if (0 == FskStrCompare("rtsp", urlComp->scheme))
				portNum = 554;
		}
		else {		// no scheme specified -- relative URL?
			urlComp->scheme = NULL;
			p = urlComp->data;
		}

		while (p && *p && *p == '/')
			p++;
		colon = FskStrChr(p, ':');	// look for user:pass@ (hostname, etc.) - must occur before next slash
		at = FskStrChr(p, '@');
		slash = FskStrChr(p, '/');
		if ((colon && at) && (!slash || ((colon < slash) && (at < slash)))) {
			urlComp->username = p;
			*colon++ = '\0';
			urlComp->password = colon;
			*at++ = '\0';
			p = at;
		}

		urlComp->host = p;
		while (p && *p && *p != '/' && *p != ':' && *p != '?')
			p++;
		if (p && *p == ':') {
			*p++ = '\0';
			portStr = p;
			while (p && *p && *p != '/')
				p++;
			if (p && *p) *p++ = '\0';
			if (portStr) {
				portNum = FskStrToNum((const char*)portStr);
				urlComp->specifyPort = true;
			}
		}
		else if (p && *p == '/')
			*p++ = '\0';
	}

	urlComp->path = p;
	while (p && *p && *p != '?' /*&& *p != ' ' */)
		p++;

	if (p && *p == '?') {
		*p++ = '\0';
		urlComp->params = p;
		while (p && *p && *p != ' ')
			p++;
		if (p)
			*p++ = '\0';
	}
/*	else if (p && *p == ' ') {
		*p++ = '\0';
		urlComp->params = p;
	}
*/
	urlComp->port = portNum;

	if (NULL != urlComp->host) {
		// host is case-insensitive (RFC 1035). convert to lowercase here so Host header is lower case (or bad things can happen). This is consistent with what browsers (IE, FireFox, Safari) do.
		p = urlComp->host;
		while (*p) {
			char c = *p;
			if (('A' <= c) && (c <= 'Z'))
				*p = c + ('a' - 'A');
			p += 1;
		}
	}

	err = sFskStrParsedUrlRebuild(urlComp);

	urlComp->valid = true;

	*urlComponents = urlComp;

bail:
	if (kFskErrNone != err) {
		if (ours && urlComp) {
			FskMemPtrDispose(urlComp->url);
			FskMemPtrDispose(urlComp->data);
			FskMemPtrDispose(urlComp);
		}
	}

	return err;
}

FskErr FskStrParsedUrlSetHost(FskStrParsedUrl urlComponents, char *host) {
	FskErr err = kFskErrParameterError;
	if (urlComponents) {
		FskMemPtrDispose(urlComponents->url);
		FskMemPtrDispose(urlComponents->host);
		urlComponents->relativeUrl = false;
		urlComponents->host = FskStrDoCopy(host);
		err = sFskStrParsedUrlRebuild(urlComponents);
	}
	return err;
}

FskErr FskStrParsedUrlDispose(FskStrParsedUrl urlComponents) {
	if (urlComponents) {
		FskMemPtrDispose(urlComponents->url);
		FskMemPtrDispose(urlComponents->data);
		FskMemPtrDispose(urlComponents);
	}
	return kFskErrNone;
}


void FskStrB64EncodeArray(const char *src, UInt32 srcSize, char **pDst, UInt32 *pDstSize, Boolean linefeeds, const char *codeString)
{
	FskErr		err		= kFskErrNone;
	char		pad		= codeString[64];
	UInt32		lpos	= 0;
	UInt32		dstSize;
	char		*dst, a, b, c;

	/* Allocate memory for the base-64-encoded data */
	dstSize = (((srcSize + 2) / 3) * 4) + 1;
	if (linefeeds)
		dstSize += (dstSize / 72) + 1;
	if ((err = FskMemPtrNew(dstSize, pDst)) != kFskErrNone) {
		*pDst = NULL;
		if (pDstSize)
			*pDstSize = 0;
		return;
	}
	dst = *pDst;

	while (srcSize > 2) {
		a = *src++;
		b = *src++;
		c = *src++;
		*dst++ = codeString[((a & 0xfc) >> 2)];
		*dst++ = codeString[((a & 0x3) << 4) | ((b & 0xf0) >> 4)];
		*dst++ = codeString[((b & 0xf) << 2) | ((c & 0xc0) >> 6)];
		*dst++ = codeString[(c & 0x3f)];
		if (linefeeds) {
			lpos += 4;
			if (lpos > 72) {
				*dst++ = '\n';
				lpos = 0;
			}
		}
		srcSize -= 3;
	}

	if (srcSize == 2) {
		a = *src++;
		b = *src++;
		*dst++ = codeString[((a & 0xfc) >> 2)];
		*dst++ = codeString[((a & 0x3) << 4) | ((b & 0xf0) >> 4)];
		*dst++ = codeString[((b & 0xf) << 2)];
		*dst++ = pad;
	}
	else if (srcSize == 1) {
		a = *src++;
		*dst++ = codeString[((a & 0xfc) >> 2)];
		*dst++ = codeString[((a & 0x3) << 4)];
		*dst++ = pad;
		*dst++ = pad;
	}

	if (linefeeds)
		*dst++ = '\n';
	*dst++ = 0;								/* NULL termination, so it can be used as a C-string. */

	if (pDstSize)
		*pDstSize = (UInt32)(dst - *pDst);	/* Calculate an accurate string length */
}


void FskStrB64Encode(const char *src, UInt32 srcSize, char **dst, UInt32 *dstSize, Boolean linefeeds) {
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	FskStrB64EncodeArray(src, srcSize, dst, dstSize, linefeeds, b64);
}
