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

#include "xsAll.h"

#if __FSK_LAYER__
// @@ makefile
	#include "xs_fsk.c"
	#if mxKpl
		#include "xs_fsk_kpl.c"
	#endif
#else

#if mxWindows

#define snprintf _snprintf

#if _MSC_VER < 1800
unsigned long nan[2]={0xffffffff, 0x7fffffff};
unsigned long infinity[2]={0x00000000, 0x7ff00000};

int fpclassify(double x)
{
	int result = FP_NORMAL;
	switch (_fpclass(x)) {
	case _FPCLASS_SNAN:
	case _FPCLASS_QNAN:
		result = FP_NAN;
		break;
	case _FPCLASS_NINF:
	case _FPCLASS_PINF:
		result = FP_INFINITE;
		break;
	case _FPCLASS_NZ:
	case _FPCLASS_PZ:
		result = FP_ZERO;
		break;
	case _FPCLASS_ND:
	case _FPCLASS_PD:
		result = FP_SUBNORMAL;
		break;
	}
	return result;
}
#endif

int gettimeofday(struct timeval *tp, struct timezone *tzp)
{
  struct _timeb tb;

  _ftime(&tb);
  if (tp != 0) {
    tp->tv_sec = (long)tb.time;
    tp->tv_usec = tb.millitm * 1000;
  }
  if (tzp != 0) {
    tzp->tz_minuteswest = tb.timezone;
    tzp->tz_dsttime = tb.dstflag;
  }
  return (0);
}

#elif mxSolaris

#if mxBigEndian
unsigned long __nan[2]={0x7fffffff, 0xffffffff};
unsigned long infinity[2]={0x7ff00000, 0x00000000};
#else
unsigned long nan[2]={0xffffffff, 0x7fffffff};
unsigned long infinity[2]={0x00000000, 0x7ff00000};
#endif

#define	kExponentMask	0x7FF00000
#define	kMantisseMask	0x000FFFFF

typedef struct {
#if mxBigEndian
	unsigned long hi;
	unsigned long lo;
#else
	unsigned long lo;
	unsigned long hi;
#endif
} int64;

int fpclassify(double x)
{
	unsigned long int anExponent;
  int64 aValue = *((int64 *)&x);

	anExponent = aValue.hi & kExponentMask;
	if (anExponent == kExponentMask) {
		if (((aValue.hi & kMantisseMask) | aValue.lo) == 0)
			return FP_INFINITE;
		else
      return FP_NAN;
	}
	else if (anExponent != 0)
		return FP_NORMAL;
	else {
		if (x == 0.0)
			return FP_ZERO;
		else
			return FP_SUBNORMAL;
	}
}

#endif

void fxErrorMessage(txMachine* the, txInteger theCode, txString theBuffer, txSize theSize)
{
#if mxWindows
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, theCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), theBuffer, theSize, NULL);
#else
	char* aMessage = strerror(theCode);
	strncpy(theBuffer, aMessage, theSize - 1);
	theBuffer[theSize - 1] = 0;
#endif
}

txString fxIntegerToString(txInteger theValue, txString theBuffer, txSize theSize)
{
	snprintf(theBuffer, theSize, "%d", (int)theValue);
	return theBuffer;
}

/*
txString fxNumberToString(txMachine* the, txNumber theValue, txString theBuffer, txSize theSize, txByte theMode, txInteger thePrecision)
{
	switch (c_fpclassify(theValue)) {
	case C_FP_INFINITE:
		c_strcpy(theBuffer, (theValue > 0) ? "Infinity" : "-Infinity");
		break;
	case C_FP_NAN:
		c_strcpy(theBuffer, "NaN");
		break;
	case C_FP_ZERO:
		c_strcpy(theBuffer, "0");
		break;
	default:
		switch (theMode) {
		case 'e':
			snprintf(theBuffer, theSize, "%.*e", (int)thePrecision, theValue);
			break;
		case 'f':
			snprintf(theBuffer, theSize, "%.*f", (int)thePrecision, theValue);
			break;
		case 'g':
			snprintf(theBuffer, theSize, "%.*g", (int)thePrecision, theValue);
			break;
		default:
			snprintf(theBuffer, theSize, "%g", theValue);
			break;
		}
		break;
	}
	return theBuffer;
}
*/


txU2* TextUTF8ToUnicode16NE(const unsigned char *text, txU4 textByteCount, txU4 *encodedTextByteCount)
{
	txError err;
	txU4  length      = 0;
	const txU1 *p = text;
	txU2* out;
	txU2* encodedText;

	while (textByteCount--) {                                       /* Convert from byte count to number of characters */
		unsigned c = *p++;
		if ((c & 0xC0) != 0x80)
			length++;
		}

	encodedText = c_malloc((length + 1) * 2); /* Allocate Unicode16 memory, including a NULL terminator */
	if(!encodedText)
		return C_NULL;
	if (encodedTextByteCount) *encodedTextByteCount = length * 2;   /* Set output byte count, if count was requested */
 
	out = encodedText;
	while (length--) {
		txU2 uc;
		uc = *text++;
		if (0x0080 & uc) {                                            /* non-ASCII */
			txUTF8Sequence *aSequence;
			for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
				if ((uc & aSequence->cmask) == aSequence->cval)
					break;
			}
			if (0 != aSequence->size) {
				txU4 aSize = aSequence->size - 1;
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
	*out = 0; /* terminate string */
	return encodedText;
}


txU1* TextUnicode16LEToUTF8(const txU2 *text, txU4 textByteCount, txU4 *encodedTextByteCount)
{
		txU1 *encodedText = C_NULL;
		txU4 encodeByteCount = 0;
		txU4 characterCount = textByteCount >> 1;
		txU1 *encodedTextOut;
		txU2 c;

		encodedText = c_malloc(1 + ((characterCount << 1) * 3));
		if(!encodedText)
			return C_NULL;
		encodedTextOut = encodedText;
		while (characterCount--) {
			c = mxMisaligned16_GetN(text);
			text++;
			c = mxEndianU16_LtoN(c);
		
			if (0 == (c & ~0x007f)) {
				*encodedText++ = (txU1)c;
				encodeByteCount += 1;
			}
			else
				if (0 == (c & ~0x07ff)) {
					*encodedText++ = (txU1)(0xc0 | (c >> 6));
					*encodedText++ = (txU1)(0x80 | (c & 0x3f));
					encodeByteCount += 2;
				}
				else {
					*encodedText++ = (txU1)(0xe0 | (c >> 12));
					*encodedText++ = (txU1)(0x80 | ((c >> 6) & 0x3f));
					*encodedText++ = (txU1)(0x80 | (c & 0x3f));
					encodeByteCount += 3;
				}
		}
		*encodedText++ = 0;
		if (encodedTextByteCount) *encodedTextByteCount = encodeByteCount;
		return encodedTextOut;
}

txString fxStringToUpper(txMachine* the, txString theString)
{
	txString result = NULL;
	txU2 *unicodeText;
	txU4 unicodeBytes;
	txU1 *utf8Text;
	txU4 utf8Bytes;
	txU4 i;
	txU2 c;
	unicodeText = TextUTF8ToUnicode16NE((const txU1 *)theString, c_strlen(theString), &unicodeBytes);
	if(!unicodeText)
		return C_NULL;
	for (i = 0; i < unicodeBytes / 2; i++) {
		c = unicodeText[i];
		if (c < 0x080) {
			unicodeText[i] = c_toupper(c);
		}
		/*according to http://www.unicode.org/charts*/
		else if ((c >= 0xff41) && (c<=0xff5a))
			unicodeText[i] = c - 0x20;
		else if ( c >= 0x0561 && c < 0x0587 ) 
			unicodeText[i] = c - 0x30;
	}
 
	utf8Text = TextUnicode16LEToUTF8(unicodeText, unicodeBytes, &utf8Bytes);
	if(!utf8Text)
		return C_NULL;
	result = fxNewChunk(the, utf8Bytes + 1);
	c_memmove(result, utf8Text, utf8Bytes + 1);
	c_free(utf8Text);
	c_free(unicodeText);
 	return result;
}
txString fxStringToLower(txMachine* the, txString theString)
{
	txString result = NULL;
	txU2 *unicodeText;
	txU4 unicodeBytes;
	txU1 *utf8Text;
	txU4 utf8Bytes;
	txU4 i;
	unicodeText = TextUTF8ToUnicode16NE((const txU1 *)theString, c_strlen(theString), &unicodeBytes);
	if(!unicodeText)
		return C_NULL;

	for (i = 0; i < unicodeBytes / 2; i++) {
		txU2 c = unicodeText[i];
		if (c < 0x080) {
			unicodeText[i] = c_tolower(c);
		}
		/*according to http://www.unicode.org/charts*/
		else if ( c >= 0x0531 && c <= 0x0556 ) 
			unicodeText[i] = c + 0x30;
		else if ((c >= 0xff21) && (c<=0xff3a))
			unicodeText[i] = c + 0x20;

	}
 
	utf8Text = TextUnicode16LEToUTF8(unicodeText, unicodeBytes, &utf8Bytes);
	if(!utf8Text)
		return C_NULL;
	result = fxNewChunk(the, utf8Bytes + 1);
	c_memmove(result, utf8Text, utf8Bytes + 1);
	c_free(utf8Text);
	c_free(unicodeText);
 	return result;
}

#ifdef mxDebug

static txBoolean fxGetBreakpointsPath(txMachine* the, char* thePath);

void fxConnect(txMachine* the)
{
#if mxWindows
	WSADATA wsaData;
	char* aColon;
	char aName[256];
	int aPort;
	struct hostent *aHost;
	struct sockaddr_in anAddress;

	the->connection = INVALID_SOCKET;
	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
		goto bail;
	the->connection = socket(AF_INET, SOCK_STREAM, 0);
	if (the->connection == INVALID_SOCKET)
		goto bail;
	aColon = getenv("XSBUG_HOST");
	if (aColon) {
		strncpy(aName, aColon, 255);
		aName[255] = 0;
		aColon = strchr(aName, ':');
		if (aColon == NULL)
			aPort = 5002;
		else {
			*aColon = 0;
			aColon++;
			aPort = strtol(aColon, NULL, 10);
		}
	}
	else {
		strcpy(aName, "localhost");
		aPort = 5002;
	}
	aHost = gethostbyname(aName);
	if (!aHost)
		goto bail;
	memcpy(&(anAddress.sin_addr), aHost->h_addr, aHost->h_length);
  	anAddress.sin_family = AF_INET;
  	anAddress.sin_port = htons(aPort);
	if (connect(the->connection, (struct sockaddr*)&anAddress, sizeof(anAddress)) == SOCKET_ERROR)
		goto bail;
	return;
bail:
	fxDisconnect(the);
#else
	int	aSocketFlag;
	char aName[256];
	char* aColon;
	int aPort;
	struct hostent *aHost;
	struct sockaddr_in anAddress;

	the->connection = socket(AF_INET, SOCK_STREAM, 0);
	if (the->connection <= 0)
		goto bail;
	aSocketFlag = fcntl(the->connection, F_GETFL, 0);

	aColon = getenv("XSBUG_HOST");
	if (aColon) {
		strncpy(aName, aColon, 255);
		aName[255] = 0;
		aColon = strchr(aName, ':');
		if (aColon == NULL)
			aPort = 5002;
		else {
			*aColon = 0;
			aColon++;
			aPort = strtol(aColon, NULL, 10);
		}
	}
	else {
		c_strcpy(aName, "localhost");
		aPort = 5002;
	}
	aHost = gethostbyname(aName);
	if (!aHost)
		goto bail;
	memcpy(&(anAddress.sin_addr), aHost->h_addr, aHost->h_length);
	anAddress.sin_family = AF_INET;
	anAddress.sin_port = htons(aPort);

	signal(SIGPIPE, SIG_IGN);
	fcntl(the->connection, F_SETFL, aSocketFlag | O_NONBLOCK);
	if (connect(the->connection, (struct sockaddr*)&anAddress, sizeof(anAddress))) {
		fd_set aReadSet;
		fd_set aWriteSet;
		struct timeval aTimeout = { 2, 0 }; // 2 seconds, 0 micro-seconds
		if (errno != EINPROGRESS)
			goto bail;
		FD_ZERO(&aReadSet);
		FD_SET(the->connection, &aReadSet);
		aWriteSet = aReadSet;
		if (select(the->connection + 1, &aReadSet, &aWriteSet, NULL, &aTimeout) == 0)
			goto bail;
		if (FD_ISSET(the->connection, &aReadSet) || FD_ISSET(the->connection, &aWriteSet)) {
			int anError = 0;
			unsigned int aLength = sizeof(anError);
			if (getsockopt(the->connection, SOL_SOCKET, SO_ERROR, &anError, &aLength) < 0)
				goto bail; /* Solaris pending error */
			if (anError)
				goto bail;
		}
		else
			goto bail;
	}

	fcntl(the->connection, F_SETFL, aSocketFlag);
	signal(SIGPIPE, SIG_DFL);
	return;
bail:
	if (the->connection > 0)
		fcntl(the->connection, F_SETFL, aSocketFlag);
	fxDisconnect(the);
	signal(SIGPIPE, SIG_DFL);
#endif
}

void fxDisconnect(txMachine* the)
{
#if mxWindows
	if (the->connection != INVALID_SOCKET) {
		closesocket(the->connection);
		the->connection = INVALID_SOCKET;
	}
	WSACleanup();
#else
	if (the->connection > 0) {
		close(the->connection);
		the->connection = 0;
	}
#endif
}

char *fxGetAddress(txMachine* the)
{
	static char anAddress[256];
	char* aColon;

	aColon = getenv("XSBUG_HOST");
	if (aColon) {
		strncpy(anAddress, aColon, 255);
		anAddress[255] = 0;
	}
	else
		c_strcpy(anAddress, "localhost:5002");
	return anAddress;
}

txBoolean fxGetAutomatic(txMachine* the)
{
	return 1;
}

txBoolean fxGetBreakpointsPath(txMachine* the, char* thePath)
{
#if mxWindows
	char aDirectory[1024];

	GetCurrentDirectory(1024, aDirectory);
	strcpy(thePath, aDirectory);
	strcat(thePath, "\\");
#else
	struct passwd* a_passwd;

	a_passwd = getpwuid(getuid());
	if (a_passwd == NULL)
		return 0;
	strcpy(thePath, a_passwd->pw_dir);
	strcat(thePath, "/");
#endif
	if (the->name)
		strcat(thePath, the->name);
	else
		strcat(thePath, "xslib");
	strcat(thePath, ".breakpoints");
	return 1;
}

txBoolean fxIsConnected(txMachine* the)
{
#if mxWindows
	return (the->connection != INVALID_SOCKET) ? 1 : 0;
#else
	return (the->connection > 0) ? 1 : 0;
#endif
}

void fxReadBreakpoints(txMachine* the)
{
	char aPath[1024];
	FILE* aFile;
	char* aLine;

	if (fxGetBreakpointsPath(the, aPath)) {
		aFile = fopen(aPath, "r");
		if (aFile) {
			while (fgets(aPath, sizeof(aPath), aFile)) {
				aLine = strrchr(aPath, '#');
				if (aLine != NULL) {
					*aLine = 0;
					aLine++;
					fxSetBreakpoint(the, aPath, aLine);
				}
			}
			fclose(aFile);
		}
	}
}

void fxReceive(txMachine* the)
{
	int aCount;

	the->echoOffset = 0;
#if mxWindows
	if (the->connection != INVALID_SOCKET) {
		aCount = recv(the->connection, the->echoBuffer, the->echoSize - 1, 0);
		if (aCount < 0)
			fxDisconnect(the);
		else
			the->echoOffset = aCount;
	}
#else
	if (the->connection > 0) {
	again:
		aCount = read(the->connection, the->echoBuffer, the->echoSize - 1);
		if (aCount < 0) {
			if (errno == EINTR)
				goto again;
			else
				fxDisconnect(the);
		}
		else
			the->echoOffset = aCount;
	}
#endif
	the->echoBuffer[the->echoOffset] = 0;
}

void fxSend(txMachine* the)
{
#if mxWindows
	if (the->connection != INVALID_SOCKET)
		if (send(the->connection, the->echoBuffer, the->echoOffset, 0) <= 0)
			fxDisconnect(the);
#else
	if (the->connection > 0) {
	again:
		if (write(the->connection, the->echoBuffer, the->echoOffset) <= 0) {
			if (errno == EINTR)
				goto again;
			else
				fxDisconnect(the);
		}
	}
#endif
}

void fxSetAddress(txMachine* the, char* theAddress)
{
}

void fxSetAutomatic(txMachine* the, txBoolean theAutomatic)
{
}

void fxWriteBreakpoints(txMachine* the)
{
	char aPath[1024];
	FILE* aFile;
	txSlot* aBreakpoint = C_NULL;
	char aLine[32];

	if (fxGetBreakpointsPath(the, aPath)) {
		aFile = fopen(aPath, "w");
		if (aFile) {
			while ((aBreakpoint = fxGetNextBreakpoint(the, aBreakpoint, aPath, aLine))) {
				fputs(aPath, aFile);
				fputs("#", aFile);
				fputs(aLine, aFile);
				fputs("\n", aFile);
			}
			fclose(aFile);
		}
	}
}

#endif /* mxDebug */

#ifdef mxProfile

static txBoolean fxGetProfilePath(txMachine* the, char* thePath);

void fxCloseProfileFile(txMachine* the)
{
	if (the->profileFile) {
		fclose(the->profileFile);
		the->profileFile = NULL;
	}
}

txBoolean fxGetProfilePath(txMachine* the, char* thePath)
{
#if mxWindows
	char aDirectory[1024];

	GetCurrentDirectory(1024, aDirectory);
	strcpy(thePath, aDirectory);
	strcat(thePath, "\\");
#else
	struct passwd* a_passwd;

	a_passwd = getpwuid(getuid());
	if (a_passwd == NULL)
		return 0;
	strcpy(thePath, a_passwd->pw_dir);
	strcat(thePath, "/");
#endif
	return 1;
}

void fxOpenProfileFile(txMachine* the, char* theName)
{
	char aPath[1024];

	if (fxGetProfilePath(the, aPath)) {
		strcat(aPath, theName);
		the->profileFile = fopen(aPath, "wb");
	}
	else
		the->profileFile = NULL;
}

void fxWriteProfileFile(txMachine* the, void* theBuffer, txInteger theSize)
{
	if (the->profileFile)
		fwrite(theBuffer, theSize, 1, the->profileFile);
}

#endif /* mxProfile */

#endif


