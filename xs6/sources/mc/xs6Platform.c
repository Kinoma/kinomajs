/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#include "xs6All.h"
#include "mc_file.h"
#include "mc_event.h"
#include "mc_env.h"

void fxErrorMessage(txMachine* the, txInteger theCode, txString theBuffer, txSize theSize)
{
	char* aMessage = strerror(theCode);
	strncpy(theBuffer, aMessage, theSize - 1);
	theBuffer[theSize - 1] = 0;
}

#define mxEndian16_Swap(a)         \
	((((txU1)a) << 8)      |   \
	(((txU2)a) >> 8))

#if mxLittleEndian
	#define mxMisaligned16_GetN(a)         \
		(((txU2)((txU1*)(a))[1] << 8) |  \
		((txU2)((txU1*)(a))[0] << 0))
	#define mxEndianU16_LtoN(a) (a)
#else
	#define mxMisaligned16_GetN(a)         \
		(((txU2)((txU1*)(a))[0] << 8) |  \
		((txU2)((txU1*)(a))[1] << 0))
	#define mxEndianU16_LtoN(a) ((txU2)mxEndian16_Swap(a))
#endif

txU2* TextUTF8ToUnicode16NE(const unsigned char *text, txU4 textByteCount, txU4 *encodedTextByteCount)
{
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
			const txUTF8Sequence *aSequence;
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

static void fxReaderCallback(int s, unsigned int flags, void *closure)
{
	txMachine *the = closure;

	if (flags & MC_SOCK_READ)
		fxDebugCommand(the);
	else {
		mc_event_unregister(the->connection);
		fxDisconnect(the);
	}
}

void fxAddReadableCallback(txMachine* the)
{
	mc_event_register(the->connection, MC_SOCK_READ, fxReaderCallback, the);
}

txBoolean fxIsReadable(txMachine* the)
{
#if mxMC
	unsigned short n;
#else
	int n;
#endif

	return the->connection >= 0 && ioctl(the->connection, FIONREAD, &n) == 0 && n > 0;
}

void fxRemoveReadableCallback(txMachine* the)
{
	mc_event_unregister(the->connection);
}

void fxConnect(txMachine* the)
{
	int aSocketFlag = 0;
	char* aColon;
	int aPort;
	struct hostent *aHost;
	struct sockaddr_in anAddress;
	char aName[HOST_NAME_MAX + 7];	/* hostname:port */

	the->connection = socket(AF_INET, SOCK_STREAM, 0);
	if (the->connection < 0)
		goto bail;
	aSocketFlag = fcntl(the->connection, F_GETFL, 0);

	aColon = (char *)mc_env_get_default("XSBUG_HOST");
	if (aColon) {
		strncpy(aName, aColon, sizeof(aName));
		aName[sizeof(aName) - 1] = 0;
		aColon = strchr(aName, ':');
		if (aColon == NULL)
			aPort = 5002;
		else {
			*aColon = 0;
			aColon++;
			aPort = strtol(aColon, NULL, 10);
		}
	}
	else
		goto bail;
	aHost = gethostbyname(aName);
	if (!aHost) {
		mc_log_debug("fxConnect: gethostbyname failed\n");
		goto bail;
	}
	memcpy(&(anAddress.sin_addr), aHost->h_addr, aHost->h_length);
	anAddress.sin_family = AF_INET;
	anAddress.sin_port = htons(aPort);

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
			socklen_t aLength = sizeof(anError);
			if (getsockopt(the->connection, SOL_SOCKET, SO_ERROR, &anError, &aLength) < 0)
				goto bail; /* Solaris pending error */
			if (anError)
				goto bail;
		}
		else
			goto bail;
	}

	fcntl(the->connection, F_SETFL, aSocketFlag);
	return;
bail:
	mc_log_debug("fxConnect: error %d\n", errno);
	if (the->connection >= 0)
		fcntl(the->connection, F_SETFL, aSocketFlag);
	fxDisconnect(the);
}

static void
fxEventCallback(txMachine *the, void *closure)
{
	xsBeginHost(the);
	xsVars(2);
	xsVar(0) = xsGet(xsGlobal, xsID("require"));
	xsVar(1) = xsString("debug");
	xsResult = xsCall1(xsVar(0), xsID("weak"), xsVar(1));
	xsCall0_noResult(xsResult, xsID("onDisconnect"));
	xsEndHost(the);
}

static void
fxClose(txMachine* the)
{
	if (the->connection >= 0) {
		fxDisconnect(the);
		mc_event_thread_call(fxEventCallback, NULL, MC_CALL_ASYNC);
	}
}

void fxDisconnect(txMachine* the)
{
	if (the->connection >= 0) {
		mc_event_unregister(the->connection);
		closesocket(the->connection);
		the->connection = -1;
	}
}

char *fxGetAddress(txMachine* the)
{
	return NULL;
}

txBoolean fxGetAutomatic(txMachine* the)
{
	return 0;
}

txBoolean fxIsConnected(txMachine* the)
{
	return (the->connection >= 0) ? 1 : 0;
}

void fxReadBreakpoints(txMachine* the)
{
}

void fxReceive(txMachine* the)
{
	int aCount;

	the->echoOffset = 0;
	if (the->connection >= 0) {
	again:
		aCount = read(the->connection, the->echoBuffer, the->echoSize - 1);
		if (aCount < 0) {
			if (errno == EINTR)
				goto again;
			else
				fxClose(the);
		}
		else
			the->echoOffset = aCount;
	}
	the->echoBuffer[the->echoOffset] = 0;
}

void fxSend(txMachine* the)
{
	if (the->connection >= 0) {
	again:
		if (write(the->connection, the->echoBuffer, the->echoOffset) <= 0) {
			if (errno == EINTR)
				goto again;
			else
				fxClose(the);
		}
	}
}

void fxSetAddress(txMachine* the, char* theAddress)
{
	mc_env_set_default("XSBUG_HOST", theAddress);
}

void fxSetAutomatic(txMachine* the, txBoolean theAutomatic)
{
}

void fxWriteBreakpoints(txMachine* the)
{
}

#endif /* mxDebug */



/* PROFILE */

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
	(void)strcpy(thePath, mc_get_special_dir("temporaryDirectory"));
	return 1;
}

void fxOpenProfileFile(txMachine* the, char* theName)
{
	char aPath[PATH_MAX];

	mc_check_stack();

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
