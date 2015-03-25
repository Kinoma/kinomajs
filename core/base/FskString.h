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
#ifndef __FSKSTRING__
#define __FSKSTRING__

#include "FskMemory.h"

/*
	Strings
*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

FskAPI(char *)	FskStrDoCopy(const char *str);
FskAPI(char *)	FskStrDoCat(const char *strA, const char *strB);
FskAPI(UInt32)	FskStrLen(const char *str);
FskAPI(void)	FskStrCopy(char *result, const char *str);
FskAPI(void)	FskStrNCopy(char *result, const char *str, UInt32 count);
FskAPI(void)	FskStrCat(char *result, const char *str);
FskAPI(void)	FskStrNCat(char *result, const char *str, UInt32 len);
FskAPI(SInt32)	FskStrCompare(const char *s1, const char *s2);
FskAPI(SInt32)	FskStrCompareWithLength(const char *s1, const char *s2, UInt32 n);
FskAPI(char *)	FskStrStr(const char *s1, const char *s2);
FskAPI(char *)	FskStrChr(const char *s, char c);
FskAPI(char *)	FskStrRChr(const char *string, char search);

FskAPI(UInt32)	FskUnicodeStrLen(const UInt16 *str);

FskAPI(FskErr)	FskStrListDoCopy(const char *str, char **copy);
FskAPI(UInt32)	FskStrListLen(const char *str);
FskAPI(SInt32)	FskStrListCompare(const char *strA, const char *strB);

FskAPI(void)	FskStrNumToStr(SInt32 value, char *str, UInt32 len);
FskAPI(SInt32)	FskStrToNum(const char *str);
FskAPI(void)    FskStrDoubleToStr(double value, char *str, UInt32 len, const char *format);
FskAPI(void)	FskStrNumToHex(UInt32 value, char *str, UInt32 digits);
FskAPI(unsigned long) FskStrToUL(const char *nptr, char **endptr, UInt32 base);
FskAPI(long)	FskStrToL(const char *nptr, char **endptr, UInt32 base);
FskAPI(double)	FskStrToD(const char *nptr, char **endptr);

FskAPI(FskInt64) FskStrToFskInt64Base(const char *str, UInt32 base);
FskAPI(FskInt64) FskStrToFskInt64(const char *str);

FskAPI(SInt32)	FskStrTail(const char *str, const char *tail);
FskAPI(char *)	FskStrCopyUntil(char *to, const char *from, char stopChar);
FskAPI(char *)	FskStrNCopyUntil(char *to, const char *from, int copyMax, char stopChar);
FskAPI(char *)	FskStrNChr(const char *from, UInt32 len, char c);
FskAPI(char *)	FskStrNStr(const char *from, UInt32 len, const char *str);
FskAPI(char *	)FskStrStripHeadSpace(const char *a);
FskAPI(char *)	FskStrStripTailSpace(char *a);
FskAPI(char *)	FskStrStripQuotes(char *a);
FskAPI(char *	)FskStrNCopyUntilSpace(char *to, const char *from, int copyMax);

FskAPI(SInt32)	FskStrCompareCaseInsensitive(const char *s1, const char *s2);
FskAPI(SInt32)	FskStrCompareCaseInsensitiveWithLength(const char *s1, const char *s2, UInt32 n);

FskAPI(void)	FskStrDecodeEscapedChars(const char *inPath, char *outPath);
FskAPI(UInt32)	FskStrHexToNum(const char *str, UInt32 num);

FskAPI(void)	FskCleanPath(char *a);

FskAPI(UInt32)	FskStrCountEscapedCharsToEncode(const char *inPath);
FskAPI(void)	FskStrEncodeEscapedChars(const char *inPath, char *outPath);
FskAPI(FskErr)	FskStrURLEncode(const char *inPath, char **outPath);

FskAPI(Boolean)	FskStrIsDottedQuad(char *str, int *out);


typedef struct FskStrParsedUrlRecord {
	char *data;			// copy of original url chopped into  elements below
	char *scheme;
	char *username;
	char *password;
	char *host;
	int  port;
	char *path;
	char *params;
	char *url;			// constructed from components
	Boolean valid;
	Boolean relativeUrl;	// used internally to reconstruct the URL
	Boolean specifyPort;	// used internally to reconstruct the URL
} FskStrParsedUrlRecord, *FskStrParsedUrl;

FskAPI(FskErr) FskStrParseUrl(char *url, FskStrParsedUrl *urlComponents);
FskAPI(FskErr) FskStrParsedUrlSetHost(FskStrParsedUrl urlComponents, char *host);
FskAPI(FskErr) FskStrParsedUrlDispose(FskStrParsedUrl urlComponents);

#define kFskLF 0x0a
#define kFskCR 0x0d
#define FskStrIsCRLF(x)		(((x) == kFskLF) || ((x) == kFskCR))
#define FskStrIsSpace(x)	(((x) == ' ') || ((x) == '\t'))

#if TARGET_OS_WIN32 || (TARGET_OS_KPL && defined(_MSC_VER))
	#define snprintf(to, toSize, ...)	_snprintf_s(to, toSize, _TRUNCATE, __VA_ARGS__)
#endif


/// Encode data using Base-64.
/// \param[in]		src			the data to be encoded.
/// \param[in]		srcSize		the number of bytes in the source data.
/// \param[out]		pDst		location where the resulting C-string is to be stored.
/// \param[out]		pDstSize	location where the length of the string (including terminating \0) is stored. Can be NULL.
/// \param[in]		linefeeds	if true, adds a newline into the output every 72 characters.
FskAPI(void) FskStrB64Encode(const char *src, UInt32 srcSize, char **pDst, UInt32 *pDstSize, Boolean linefeeds);

/// Encode data using a generailzed Base-64, where the code character set is supplied as a parameter.
/// \param[in]		src			the data to be encoded.
/// \param[in]		srcSize		the number of bytes in the source data.
/// \param[out]		pDst		location where the resulting C-string is to be stored.
/// \param[out]		pDstSize	location where the length of the string (including terminating \0) is stored. Can be NULL.
/// \param[in]		linefeeds	if true, adds a newline into the output every 72 characters.
/// \param[in]		codeString	[65] unique characters, with the first 64 used for encoding, and the 65th as a pad.
FskAPI(void) FskStrB64EncodeArray(const char *src, UInt32 srcSize, char **pDst, UInt32 *pDstSize, Boolean linefeeds, const char *codeString);

#if !SUPPORT_MEMORY_DEBUG
	#define FskStrDoCopy_Untracked(str) FskStrDoCopy(str)
#else /* SUPPORT_MEMORY_DEBUG */
	FskAPI(char *) FskStrDoCopy_Untracked(const char *str);
#endif /* SUPPORT_MEMORY_DEBUG */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKSTRING__ */

