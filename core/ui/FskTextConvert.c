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
#include "FskTextConvert.h"

#include "FskEndian.h"
#include "FskUtilities.h"
#include "FskPlatformImplementation.h"

ftUTF8Sequence gUTF8Sequences[] = {
    {1, 0x80, 0x00, 0*6, 0x0000007F, 0x00000000},
    {2, 0xE0, 0xC0, 1*6, 0x000007FF, 0x00000080},
    {3, 0xF0, 0xE0, 2*6, 0x0000FFFF, 0x00000800},
    {4, 0xF8, 0xF0, 3*6, 0x001FFFFF, 0x00010000},
    {5, 0xFC, 0xF8, 4*6, 0x03FFFFFF, 0x00200000},
    {6, 0xFE, 0xFC, 5*6, 0x7FFFFFFF, 0x04000000},
    {0, 0, 0, 0, 0, 0},
};


#if TARGET_OS_WIN32
#if PRAGMA_MARK_SUPPORTED
#pragma mark ######## WIN32 ########
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****									WIN32								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


static FskErr textConvert(const char *text, UInt32 textLen, char **encodedText, UInt32 *encodedTextLen, UInt32 fromEncoding, UInt32 toEncoding);


/********************************************************************************
 * textConvert
 ********************************************************************************/

FskErr textConvert(const char *text, UInt32 textLen, char **encodedText, UInt32 *encodedTextLen, UInt32 fromEncoding, UInt32 toEncoding)
{
	FskErr err;
	char *wStr = NULL, *encStr = NULL;
	int wLen, encLen;

	if (encodedTextLen) *encodedTextLen = 0;

	if (0 == textLen)
		return FskMemPtrNewClear(1, (FskMemPtr *)encodedText);

	wLen = MultiByteToWideChar(fromEncoding, 0, text, textLen, NULL, 0);
	err = FskMemPtrNew(wLen* 2, (FskMemPtr *)&wStr);
	BAIL_IF_ERR(err);

	MultiByteToWideChar(fromEncoding, 0, text, textLen, (LPWSTR)wStr, wLen);

	encLen = WideCharToMultiByte(toEncoding, 0, (LPCWSTR)wStr, wLen, NULL, 0, NULL, NULL);
	if (encLen) {
		err = FskMemPtrNew(encLen + 2, (FskMemPtr *)&encStr);
		BAIL_IF_ERR(err);
		encStr[encLen] = 0;
		encStr[encLen + 1] = 0;
		WideCharToMultiByte(toEncoding, 0, (LPCWSTR)wStr, wLen, (LPSTR)encStr, encLen, NULL, NULL);
		*encodedText = encStr;
		if (encodedTextLen) *encodedTextLen = encLen;
	}

bail:
	FskMemPtrDispose(wStr);
	return err;
}


/********************************************************************************
 * FskTextToPlatform
 ********************************************************************************/

FskErr FskTextToPlatform(const char *text, UInt32 textLen, char **encodedText, UInt32 *encodedTextLen)
{
	return textConvert(text, textLen, encodedText, encodedTextLen, CP_UTF8, CP_ACP);
}


/********************************************************************************
 * FskTextToUTF8
 ********************************************************************************/

FskErr FskTextToUTF8(const char *text, UInt32 textLen, char **encodedText, UInt32 *encodedTextLen)
{
	return textConvert(text, textLen, encodedText, encodedTextLen, CP_ACP, CP_UTF8);
}


/* end TARGET_OS_WIN32 ***** WIN32 ***** WIN32 ***** WIN32 ***** WIN32 ***** WIN32 ***** WIN32 *****/



#elif TARGET_OS_MAC
#if PRAGMA_MARK_SUPPORTED
#pragma mark ######## MAC ########
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****									MAC									*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


static FskErr textConvert(const char *text, UInt32 textLen, char **encodedText, UInt32 *encodedTextLen, UInt32 fromEncoding, UInt32 toEncoding);


/********************************************************************************
 * textConvert
 ********************************************************************************/

FskErr textConvert(const char *text, UInt32 textLen, char **encodedText, UInt32 *encodedTextLen, UInt32 fromEncoding, UInt32 toEncoding)
{
	FskErr		err = kFskErrOperationFailed;
	CFRange		range;
	CFStringRef	cref;
	UInt32		max;
	SInt32		len;
	Ptr 		buf = NULL;

	len=0;
	*encodedText=NULL;
	cref=CFStringCreateWithBytes( kCFAllocatorDefault,(const UInt8 *)text,textLen,fromEncoding,false );
	if( cref )
	{
		max=textLen*4;
		FskMemPtrNewClear(max, (FskMemPtr *)&buf);

		if( buf )
		{
			range=CFRangeMake( 0,CFStringGetLength(cref) );
			CFStringGetBytes( cref,range,toEncoding,0,false,(UInt8 *)buf,max,&len );
			err = FskMemPtrNew( (UInt32)len + 1,(FskMemPtr *)encodedText );
			if (kFskErrNone == err) {
				memcpy(*encodedText, buf, len);
				(*encodedText)[len] = 0;
			}
			FskMemPtrDispose( buf );
			if (encodedTextLen)
				*encodedTextLen=len;
		}
		CFRelease( cref );
	}
	return err;
}


/********************************************************************************
 * FskTextToPlatform
 ********************************************************************************/

FskErr FskTextToPlatform(const char *text, UInt32 textLen,char **encodedText, UInt32 *encodedTextLen)
{
	return textConvert(text, textLen, encodedText, encodedTextLen, kCFStringEncodingUTF8, kCFStringEncodingUnicode);
}


/********************************************************************************
 * FskTextToUTF8
 ********************************************************************************/

FskErr FskTextToUTF8(const char *text, UInt32 textLen,char **encodedText, UInt32 *encodedTextLen)
{
	return textConvert(text, textLen, encodedText, encodedTextLen, kCFStringEncodingUnicode, kCFStringEncodingUTF8);
}


/* end TARGET_OS_MAC ***** MAC ***** MAC ***** MAC ***** MAC ***** MAC ***** MAC ***** MAC ***** MAC *****/



#else
#if PRAGMA_MARK_SUPPORTED
#pragma mark ##### not WIN not MAC #####
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****						not WIN32 and not MAC							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


// stubs... useful for those platforms that are UTF-8 native!


/********************************************************************************
 * FskTextToPlatform
 ********************************************************************************/

FskErr FskTextToPlatform(const char *text, UInt32 textLen, char **encodedText, UInt32 *encodedTextLen)
{
	char *encoded;
	FskErr err = FskMemPtrNew(textLen + 1, &encoded);
	if (kFskErrNone == err) {
		FskMemMove(encoded, text, textLen);
		encoded[textLen] = 0;
		*encodedText = encoded;
		if (encodedTextLen) *encodedTextLen = textLen;
	}
	else {
		*encodedText = NULL;
		if (encodedTextLen) *encodedTextLen = 0;
	}
	return err;
}


/********************************************************************************
 * FskTextToUTF8
 ********************************************************************************/

FskErr FskTextToUTF8(const char *text, UInt32 textLen, char **encodedText, UInt32 *encodedTextLen)
{
	return FskTextToPlatform(text, textLen, encodedText, encodedTextLen);
}



#endif /***** !WIN32 && !MAC ***** !WIN32 && !MAC ***** !WIN32 && !MAC ***** !WIN32 && !MAC *****/

/********************************************************************************
 * FskTextLatin1ToUTF8
 ********************************************************************************/

#if TARGET_OS_ANDROID
void putCharToUTF8(unsigned char **s, UInt32 c32) {
	unsigned char c = c32 & 0xff;
	*(*s) = c;
	(*s)++;
}
#else
#define putCharToUTF8(s, c) { **(s) = (unsigned char)(c); (*(s))++; }
#endif

// characters 0x80 to 0x9F from Windows codepage 1252
static UInt32 gLatinUnusedMapping[] = {
	0xE282AC, 0x00C281, 0xE2809A, 0x00C692, 0xE2809E, 0xE280A6, 0xE280A0, 0xE280A1,
	0x00CB86, 0xE280B0, 0x00C5A0, 0xE280B9, 0x00C592, 0x00C28D, 0x00C5BD, 0x00C28F,
	0x00C290, 0xE28098, 0xE28099, 0xE2809C, 0xE2809D, 0xE280A2, 0xE28093, 0xE28094,
	0x00CB9C, 0xE284A2, 0x00C5A1, 0xE280BA, 0x00C593, 0x00C29D, 0x00C5BE, 0x00C5B8
};

static void putToUTF8(unsigned char** s, UInt32 c);

void FskTextCharCodeToUTF8(UInt32 charCode, unsigned char *utf8)
{
    putToUTF8(&utf8, charCode);
    *utf8++ = 0;
}

void putToUTF8(unsigned char** s, UInt32 c)
{
	if (c < 0x80) {
		putCharToUTF8 (s, c);
	}
	else if (c < 0xA0) {
		c = gLatinUnusedMapping[c - 0x80];
		if (c & 0xFF0000)
			putCharToUTF8(s, (c>>16) & 0xFF);
		putCharToUTF8(s, (c>>8) & 0xFF);
		putCharToUTF8(s, c & 0xFF);
	}
	else if (c < 0x800) {
		putCharToUTF8 (s, 0xC0 | (c>>6));
		putCharToUTF8 (s, 0x80 | (c & 0x3F));
	}
	else if (c < 0x10000) {
		putCharToUTF8 (s, 0xE0 | (c>>12));
		putCharToUTF8 (s, 0x80 | ((c>>6) & 0x3F));
		putCharToUTF8 (s, 0x80 | (c & 0x3F));
	}
	else if (c < 0x200000) {
		putCharToUTF8 (s, 0xF0 | (c>>18));
		putCharToUTF8 (s, 0x80 | ((c>>12) & 0x3F));
		putCharToUTF8 (s, 0x80 | ((c>>6) & 0x3F));
		putCharToUTF8 (s, 0x80 | (c & 0x3F));
	}
}

#define SMALL_STRING	256
FskErr FskTextLatin1ToUTF8(const char *text, UInt32 textLen, char **encodedText, UInt32 *encodedTextLen)
{
	FskErr err;
	UInt32 i, outLen;
	unsigned char *temp = NULL, *out;
	unsigned char sTemp[SMALL_STRING];

	if ((textLen * 3 + 1) < SMALL_STRING)
		temp = sTemp;
	else {
		err = FskMemPtrNew((textLen * 3) + 1, &temp);
		BAIL_IF_ERR(err);
	}


	out = temp;
	for (i=0; i<textLen; i++) {
		unsigned char aChar = *text++;
		putToUTF8(&out, (UInt32)aChar);
	}

	outLen = out - temp;
	if (encodedTextLen)
		*encodedTextLen = outLen;

	*out++ = 0;

	err = FskMemPtrNewFromData(outLen + 1, temp, (FskMemPtr *)encodedText);
	BAIL_IF_ERR(err);

bail:
	if (temp != sTemp)
		FskMemPtrDispose(temp);

	return err;
}

#if PRAGMA_MARK_SUPPORTED
#pragma mark ##### CROSS PLATFORM #####
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****						Platform-Independent Code						*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskTextUnicode16BEToUTF8
 ********************************************************************************/

FskErr FskTextUnicode16BEToUTF8(const UInt16 *text, UInt32 textByteCount, char **encodedTextOut, UInt32 *encodedTextByteCount)
{
	FskErr err;
	char *encodedText;
	UInt32 encodeByteCount = 0;
	UInt32 characterCount = textByteCount >> 1;

	err = FskMemPtrNew(1 + ((characterCount << 1) * 3), &encodedText);
	BAIL_IF_ERR(err);

	*encodedTextOut = encodedText;

	while (characterCount--) {
		UInt16 c = FskMisaligned16_GetBtoN(text);

		text += 1;

		if (0 == (c & ~0x007f)) {
			*encodedText++ = (char)c;
			encodeByteCount += 1;
		}
		else
		if (0 == (c & ~0x07ff)) {
			*encodedText++ = (char)(0xc0 | (c >> 6));
			*encodedText++ = (char)(0x80 | (c & 0x3f));
			encodeByteCount += 2;
		}
		else {
			*encodedText++ = (char)(0xe0 | (c >> 12));
			*encodedText++ = (char)(0x80 | ((c >> 6) & 0x3f));
			*encodedText++ = (char)(0x80 | (c & 0x3f));
			encodeByteCount += 3;
 		}
	}

	*encodedText++ = 0;
	if (encodedTextByteCount) *encodedTextByteCount = encodeByteCount;

bail:
	return err;
}


/********************************************************************************
 * FskTextUnicode16LEToUTF8
 ********************************************************************************/

FskErr FskTextUnicode16LEToUTF8(const UInt16 *text, UInt32 textByteCount, char **encodedTextOut, UInt32 *encodedTextByteCount)
{
	FskErr err;
	char *encodedText;
	UInt32 encodeByteCount = 0;
	UInt32 characterCount = textByteCount >> 1;

	err = FskMemPtrNew(1 + ((characterCount << 1) * 3), &encodedText);
	BAIL_IF_ERR(err);

	*encodedTextOut = encodedText;

	while (characterCount--) {
		UInt16 c = FskMisaligned16_GetN(text);

		text++;
		c = FskEndianU16_LtoN(c);
		if (0 == (c & ~0x007f)) {
			*encodedText++ = (char)c;
			encodeByteCount += 1;
		}
		else
		if (0 == (c & ~0x07ff)) {
			*encodedText++ = (char)(0xc0 | (c >> 6));
			*encodedText++ = (char)(0x80 | (c & 0x3f));
			encodeByteCount += 2;
		}
		else {
			*encodedText++ = (char)(0xe0 | (c >> 12));
			*encodedText++ = (char)(0x80 | ((c >> 6) & 0x3f));
			*encodedText++ = (char)(0x80 | (c & 0x3f));
			encodeByteCount += 3;
		}
	}

	*encodedText++ = 0;
	if (encodedTextByteCount) *encodedTextByteCount = encodeByteCount;

bail:
	return err;
}


/********************************************************************************
 * FskTextUnicode16SwapBytes
 * this works in-place
 ********************************************************************************/

void FskTextUnicode16SwapBytes(register const UInt16 *fr, register UInt16 *to, register UInt32 nChars)
{
	for ( ; nChars--; fr++, to++) {
		UInt16 c = FskMisaligned16_GetN(fr);
		c = FskEndian16_Swap(c);
		FskMisaligned16_PutN(&c, to);
	}
}


/********************************************************************************
 * FskTextUTF8ToUnicode16NE
 * Convert UTF-8 to natural endian 16-bit Unicode
 ********************************************************************************/

FskErr FskTextUTF8ToUnicode16NE(const unsigned char *text, UInt32 textByteCount, UInt16 **encodedText, UInt32 *encodedTextByteCount)
{
	FskErr				err;
	UInt32				length		= 0;
	const unsigned char	*p			= text;
	UInt16				*out;

	while (textByteCount--) {										/* Convert from byte count to number of characters */
		unsigned c = *p++;
		if ((c & 0xC0) != 0x80)
			length++;
	}

	err = FskMemPtrNew((length + 1) * 2, (FskMemPtr *)encodedText);	/* Allocate Unicode16 memory, including a NULL terminator */
	if (err) {														/* Can't allocate memory */
		*encodedText = NULL;										/* Nullify output text pointer */
		if (encodedTextByteCount)	*encodedTextByteCount = 0;		/* Set output byte count to zero, if count was requested */
		return err;
	}

	if (encodedTextByteCount) *encodedTextByteCount = length * 2;	/* Set output byte count, if count was requested */

	out = *encodedText;

	while (length--) {
		UInt16 uc;

		uc = *text++;
		if (0x80 & uc) {											/* non-ASCII */
		    ftUTF8Sequence *aSequence;

			for (aSequence = gUTF8Sequences; aSequence->size; aSequence++) {
				if ((uc & aSequence->cmask) == aSequence->cval)
					break;
			}
			if (0 != aSequence->size) {
				UInt32 aSize = aSequence->size - 1;
				while (aSize) {
					aSize--;
					uc = (uc << 6) | (*text++ & 0x3F);
				}
				uc &= aSequence->lmask;
			}
			else
				uc = '?';
		}
		*out++ = uc;
	}
	*out = 0;														/* terminate string */

	return err;
}

FskErr FskTextUTF8ToUnicode16NENoAlloc(const unsigned char *text, UInt32 length, UInt16 *encodedText, UInt32 *encodedTextByteCount)
{
	SInt32 bytesFree = encodedText ? *encodedTextByteCount : 0;		/* If encodedText==NULL, just count the number of characters needed */
	UInt16 *out = encodedText;

	while (length-- && (bytesFree >= 2)) {
		UInt16 uc;

		uc = *text++;
		if (0x80 & uc) {											/* non-ASCII */
		    ftUTF8Sequence *aSequence;

			for (aSequence = gUTF8Sequences; aSequence->size; aSequence++) {
				if ((uc & aSequence->cmask) == aSequence->cval)
					break;
			}
			if (0 != aSequence->size) {
				UInt32 aSize = aSequence->size - 1;
				if (aSize > length) {
					length = kFskUInt32Max;
					break;
				}
				length -= aSize;
				while (aSize) {
					aSize--;
					uc = (uc << 6) | (*text++ & 0x3F);
				}
				uc &= aSequence->lmask;
			}
			else
				uc = '?';
		}
		*out++ = uc;
		bytesFree -= 2;
	}

	if (kFskUInt32Max == length) {
		*encodedTextByteCount = (out - encodedText) * 2;
		return kFskErrNone;
	}

	// buffer too small, count the rest
	length += 1;													/* Is this counting a terminating NULL? */
	while (length--) {
		unsigned c = *text++;
		if ((c & 0xC0) != 0x80)										/* MSB=1 indicates continuation, ... */
			out += 1;												/* MSB=0 indicates the end of the character */
	}

	*encodedTextByteCount = (out - encodedText) * 2;
	return kFskErrMemFull;
}


/********************************************************************************
 * FskTextUTF8ToUnicode16LE
 * Convert UTF-8 to little endian 16-bit Unicode
 ********************************************************************************/

FskErr FskTextUTF8ToUnicode16LE(const unsigned char *text, UInt32 textByteCount, UInt16 **encodedText, UInt32 *encodedTextByteCount)
{
	UInt32 myEncodedTextByteCount;
	FskErr err;

	err = FskTextUTF8ToUnicode16NE(text, textByteCount, encodedText, &myEncodedTextByteCount);
	#if TARGET_RT_BIG_ENDIAN
		if (!err)
			FskTextUnicode16SwapBytes(*encodedText, *encodedText, myEncodedTextByteCount / 2);
	#endif /* TARGET_RT_BIG_ENDIAN */
	if (encodedTextByteCount)
		*encodedTextByteCount = myEncodedTextByteCount;

	return err;
}


/********************************************************************************
 * FskTextUTF8ToUnicode16BE
 * Convert UTF-8 to big endian 16-bit Unicode
 ********************************************************************************/

FskErr FskTextUTF8ToUnicode16BE(const unsigned char *text, UInt32 textByteCount, UInt16 **encodedText, UInt32 *encodedTextByteCount)
{
	UInt32 myEncodedTextByteCount;
	FskErr err;

	err = FskTextUTF8ToUnicode16NE(text, textByteCount, encodedText, &myEncodedTextByteCount);
	#if TARGET_RT_LITTLE_ENDIAN
		if (!err)
			FskTextUnicode16SwapBytes(*encodedText, *encodedText, myEncodedTextByteCount / 2);
	#endif /* TARGET_RT_LITTLE_ENDIAN */
	if (encodedTextByteCount)
		*encodedTextByteCount = myEncodedTextByteCount;

	return err;
}


/********************************************************************************
 * FskTextUTF8StrLen
 ********************************************************************************/

UInt32 FskTextUTF8StrLen(const unsigned char *theString)
{
	const UInt8	*p			= (const UInt8*)theString;
	UInt32		anIndex		= 0;
	UInt8		c;

	while ((c = *p++)) {
		if ((c & 0xC0) != 0x80)
			anIndex++;
	}
	return anIndex;
}


/********************************************************************************
 * FskTextUTF8DefaultWhitespaceProcessing
 ********************************************************************************/

void FskTextUTF8DefaultWhitespaceProcessing(unsigned char *str)
{
	unsigned char *s, *t, c;

	/* Remove newlines */
	for (s = t = str; (*t = c = *s) != 0; s++)
		if ((c != '\n') && (c != '\r'))
			t++;

	/* Convert tabs into space */
	for (s = t = str; (*t = c = *s) != 0; s++, t++)
		if (c == '\t')
			*t = ' ';

	/* Strip off leading and trailing spaces */
	for (s = str; *s == ' '; s++)		/* Move s to the first non-space */
		;
	if (*s == 0) {						/* Empty string */
		str[0] = 0;
		return;
	}
	for (t = s; *t != 0; t++)			/* Move t to the end */
		;
	while (*--t == ' ')					/* Clear out spaces at the end */
		*t = 0;
	for (t = str; (*t++ = *s++) != 0; )	/* Clear out spaces in the beginning */
		;

	/* Consolidate all contiguous space characters */
	for (s = t = str; (*t++ = c = *s++) != 0; )
		if (c == ' ')
			while (*s == ' ')
				s++;
}

/********************************************************************************
 * FskTextUTF8IsWhitespace
 ********************************************************************************/

Boolean FskTextUTF8IsWhitespace(const unsigned char *text, UInt32 textLen, UInt32 *whiteSize)
{
	unsigned char c0;

	if (0 == textLen)
		return false;

	c0 = text[0];
	if ((32 == c0) || (9 == c0) || (0 == c0)) {
		if (whiteSize) *whiteSize = 1;
		return true;
	}

	if (textLen >= 3) {
		if ((0xe3 == c0) && (0x80 == text[1]) && (0x80 == text[2])) {	// Ideographic space
			if (whiteSize) *whiteSize = 3;
			return true;
		}
	}

	return false;
}

/********************************************************************************
 * FskTextUnicodeToUTF8Offset
 ********************************************************************************/

UInt32 FskTextUnicodeToUTF8Offset(const unsigned char *theString, UInt32 theOffset)
{
	const UInt8	*p				= (const UInt8*)theString;
	UInt32		unicodeOffset	= 0;
	UInt32		utf8Offset		= 0;
	UInt8		c;

	while ((c = *p++)) {
		if ((c & 0xC0) != 0x80) {
			if (unicodeOffset == theOffset)
				return utf8Offset;
			unicodeOffset++;
		}
		utf8Offset++;
	}
	if (unicodeOffset == theOffset)
		return utf8Offset;
	else
		return -1;
}

Boolean FskTextUTF8IsValid(const unsigned char *theString, SInt32 theSize)
{
	while (theSize > 0) {
		ftUTF8Sequence *aSequence;
		SInt32 aSize;
		UInt32 aResult = *theString++;
		theSize--;
		for (aSequence = gUTF8Sequences; aSequence->size; aSequence++) {
			if ((aResult & aSequence->cmask) == aSequence->cval)
				break;
		}
		aSize = aSequence->size - 1;
		if (aSize < 0)
			return false;
		if (aSize > theSize)
			return false;
		theString += aSize;
		theSize -= aSize;
	}
	return true;
}

SInt32 FskTextUTF8Advance(const unsigned char *theString, UInt32 theOffset, SInt32 direction)
{
	theString += theOffset;

	if (direction >= 0) {
		unsigned char aByte = (unsigned char)*theString;
		if (!(aByte & 0x80))
			return 1;
		if ((aByte & 0xe0) != 0xe0)
			return 2;
		if ((aByte & 0xf0) != 0xf0)
			return 3;
		if ((aByte & 0xf8) == 0xf0)
			return 4;
	}
	else {
		if ((theString[-1] & 0xc0) == 0x80) {
			if ((theString[-2] & 0xc0) == 0x80) {
				if ((theString[-3] & 0xc0) == 0x80) {
					if ((theString[-4] & 0xc0) == 0x80)
						return -4;
				}
				else
					return -3;
			}
			else
				return -2;
		}
		else
			return -1;
	}

	return 0;			// error
}

#if TARGET_OS_WIN32

static FskErr convertCase(const unsigned char *text, UInt32 textByteCount, unsigned char **upperText, UInt32 *upperTextByteCount, Boolean upper);

FskErr convertCase(const unsigned char *text, UInt32 textByteCount, unsigned char **upperText, UInt32 *upperTextByteCount, Boolean upper)
{
	FskErr err;
	WCHAR *unicodeText;
	UInt32 unicodeBytes;

	err = FskTextUTF8ToUnicode16NE(text, textByteCount, (UInt16 **)&unicodeText, &unicodeBytes);
	if (err) return err;

	if (upper)
		CharUpperBuffW(unicodeText, unicodeBytes / 2);
	else
		CharLowerBuffW(unicodeText, unicodeBytes / 2);

	err = FskTextUnicode16NEToUTF8((UInt16 *)unicodeText, unicodeBytes, (char **)upperText, upperTextByteCount);

	FskMemPtrDispose(unicodeText);

	return err;
}

FskErr FskTextUTF8ToUpper(const unsigned char *text, UInt32 textByteCount, unsigned char **upperText, UInt32 *upperTextByteCount)
{
	return convertCase(text, textByteCount, upperText, upperTextByteCount, true);
}

FskErr FskTextUTF8ToLower(const unsigned char *text, UInt32 textByteCount, unsigned char **upperText, UInt32 *upperTextByteCount)
{
	return convertCase(text, textByteCount, upperText, upperTextByteCount, false);
}

#elif TARGET_OS_MAC

static FskErr convertCase(const unsigned char *text, UInt32 textByteCount, unsigned char **upperText, UInt32 *upperTextByteCount, Boolean upper);

FskErr convertCase(const unsigned char *text, UInt32 textByteCount, unsigned char **upperText, UInt32 *upperTextByteCount, Boolean upperCase)
{
	FskErr				err = kFskErrOperationFailed;
	CFStringRef 		cfString = NULL;
	CFMutableStringRef	mutableCFString = NULL;
	CFIndex 			numberOfUTF8Chars;
	CFRange 			range;

	cfString = CFStringCreateWithBytes(kCFAllocatorDefault, text, textByteCount, kCFStringEncodingUTF8, false);
	if (cfString == NULL) goto bail;
	mutableCFString = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, cfString);
	if (mutableCFString == NULL) goto bail;

	if (upperCase)
		CFStringUppercase(mutableCFString, 0);
	else
		CFStringLowercase(mutableCFString, 0);

	range = CFRangeMake(0, CFStringGetLength(mutableCFString));
	CFStringGetBytes(mutableCFString, range, kCFStringEncodingUTF8, 0, false, NULL, 0, &numberOfUTF8Chars);

	err = FskMemPtrNew(numberOfUTF8Chars + 1, (FskMemPtr *)upperText);
	BAIL_IF_ERR(err);

	CFStringGetBytes(mutableCFString, range, kCFStringEncodingUTF8, 0, false, *upperText, numberOfUTF8Chars, NULL);
	(*upperText)[numberOfUTF8Chars] = 0;

	if (NULL != upperTextByteCount)
		*upperTextByteCount = numberOfUTF8Chars;

bail:
	if (cfString)
		CFRelease(cfString);

	if (mutableCFString)
		CFRelease(mutableCFString);

	return err;

}

FskErr FskTextUTF8ToUpper(const unsigned char *text, UInt32 textByteCount, unsigned char **upperText, UInt32 *upperTextByteCount)
{
	return convertCase(text, textByteCount, upperText, upperTextByteCount, true);
}

FskErr FskTextUTF8ToLower(const unsigned char *text, UInt32 textByteCount, unsigned char **upperText, UInt32 *upperTextByteCount)
{
	return convertCase(text, textByteCount, upperText, upperTextByteCount, false);
}

#else

// Latin-1 implementation

FskErr FskTextUTF8ToUpper(const unsigned char *text, UInt32 textByteCount, unsigned char **upperText, UInt32 *upperTextByteCount)
{
	FskErr err;
	UInt16 *unicodeText;
	UInt32 unicodeBytes;
	UInt32 i;

	err = FskTextUTF8ToUnicode16NE(text, textByteCount, &unicodeText, &unicodeBytes);
	if (err) return err;

	for (i = 0; i < unicodeBytes / 2; i++) {
		UInt16 c = unicodeText[i];
		if (c < 0x080)
			unicodeText[i] = toupper(c);
		else if (c >= 0x0ff)
			;
		else if ((c < 0x0e0) || (0xf7 == c))
			;
		else
			unicodeText[i] = c - 0x20;
	}

	err = FskTextUnicode16NEToUTF8(unicodeText, unicodeBytes, (char **)upperText, upperTextByteCount);

	FskMemPtrDispose(unicodeText);

	return err;
}

FskErr FskTextUTF8ToLower(const unsigned char *text, UInt32 textByteCount, unsigned char **lowerText, UInt32 *lowerTextByteCount)
{
	FskErr err;
	UInt16 *unicodeText;
	UInt32 unicodeBytes;
	UInt32 i;

	err = FskTextUTF8ToUnicode16NE(text, textByteCount, &unicodeText, &unicodeBytes);
	if (err) return err;

	for (i = 0; i < unicodeBytes / 2; i++) {
		UInt16 c = unicodeText[i];
		if (c < 0x080)
			unicodeText[i] = tolower(c);
		else if ((c >= 0x0df) || (0xd7 == c))
			;
		else if (c < 0x0c0)
			;
		else
			unicodeText[i] = c + 0x20;
	}

	err = FskTextUnicode16NEToUTF8(unicodeText, unicodeBytes, (char **)lowerText, lowerTextByteCount);

	FskMemPtrDispose(unicodeText);

	return err;
}

#endif
