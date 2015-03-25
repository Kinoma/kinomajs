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
#ifndef __FSKTEXTCONVERT__
#define __FSKTEXTCONVERT__

#include "Fsk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    SInt32 size;
    UInt32 cmask;
    UInt32 cval;
    SInt32 shift;
    UInt32 lmask;
    UInt32 lval;
} ftUTF8Sequence;

FskAPI(FskErr) FskTextToPlatform(const char *text, UInt32 textLen, char **encodedText, UInt32 *encodedTextLen);
FskAPI(FskErr) FskTextToUTF8(const char *text, UInt32 textLen, char **encodedText, UInt32 *encodedTextLen);
FskAPI(FskErr) FskTextLatin1ToUTF8(const char *text, UInt32 textLen, char **encodedText, UInt32 *encodedTextLen);
FskAPI(FskErr) FskTextUnicode16BEToUTF8(const UInt16 *text, UInt32 textByteCount, char **encodedText, UInt32 *encodedTextByteCount);
FskAPI(FskErr) FskTextUnicode16LEToUTF8(const UInt16 *text, UInt32 textByteCount, char **encodedText, UInt32 *encodedTextByteCount);
FskAPI(FskErr) FskTextUTF8ToUnicode16LE(const unsigned char *text, UInt32 textByteCount, UInt16 **encodedText, UInt32 *encodedTextByteCount);
FskAPI(FskErr) FskTextUTF8ToUnicode16BE(const unsigned char *text, UInt32 textByteCount, UInt16 **encodedText, UInt32 *encodedTextByteCount);
FskAPI(FskErr) FskTextUTF8ToUnicode16NE(const unsigned char *text, UInt32 textByteCount, UInt16 **encodedText, UInt32 *encodedTextByteCount);
FskAPI(void)  FskTextCharCodeToUTF8(UInt32 charCode, unsigned char *utf8);

/** Convert UTF-8 to Unicode (UTF-16) in the native endian format, using memory supplied by the caller.
 **	\param[in]		text					The UTF-8 source text.
 **	\param[in]		length					The length, in bytes, of the UTF-8 source text.
 **	\param[in, out]	encodedText				Pointer to a place to store the Unicode-encoded text, without a terminating NULL character.
 **											If this is NULL, then no encoded text is returned.
 **	\param[in, out]	encodedTextByteCount	Pointer to a place to store the number of bytes needed to represent text as Unicode,
 **											regardless of whether or not encodedText was NULL or not. In this way, it is possible to query the amount of memory
 **											needed for the converted text, allocate it, then call it again to do the conversion.
 **	\return			kFskErrNone				if the operation was successful.
 **/
FskAPI(FskErr) FskTextUTF8ToUnicode16NENoAlloc(const unsigned char *text, UInt32 length, UInt16 *encodedText, UInt32 *encodedTextByteCount);

FskAPI(void)   FskTextUTF8DefaultWhitespaceProcessing(unsigned char *str);	/* Remove newlines, convert tabs into spaces, strip off leading and trailing spaces, consolidate contiguous spaces */
FskAPI(Boolean) FskTextUTF8IsWhitespace(const unsigned char *text, UInt32 textLen, UInt32 *whiteSize);
FskAPI(void)   FskTextUnicode16SwapBytes(const UInt16 *fr, UInt16 *to, UInt32 nChars);	/* Note: number of characters, not number of bytes */

FskAPI(UInt32) FskTextUTF8StrLen(const unsigned char *text);
FskAPI(UInt32) FskTextUnicodeToUTF8Offset(const unsigned char *text, UInt32 theOffset);
FskAPI(Boolean) FskTextUTF8IsValid(const unsigned char *theString, SInt32 theSize);
FskAPI(SInt32) FskTextUTF8Advance(const unsigned char *theString, UInt32 theOffset, SInt32 direction);

FskAPI(FskErr) FskTextUTF8ToUpper(const unsigned char *text, UInt32 textByteCount, unsigned char **upperText, UInt32 *upperTextByteCount);
FskAPI(FskErr) FskTextUTF8ToLower(const unsigned char *text, UInt32 textByteCount, unsigned char **lowerText, UInt32 *lowerTextByteCount);

#if TARGET_RT_LITTLE_ENDIAN
	#define FskTextUnicode16NEToUTF8 FskTextUnicode16LEToUTF8
#else
	#define FskTextUnicode16NEToUTF8 FskTextUnicode16BEToUTF8
#endif

#ifdef __cplusplus
}
#endif

#endif
