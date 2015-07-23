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
#include "FskEnvironment.h"
#include "FskTextConvert.h"
#include "FskPlatformImplementation.h"
#include "FskEndian.h"
#include "FskFiles.h"

FskErr loadGrammar(const char *xsbPath, txGrammar *theGrammar)
{
	FskErr err;
	FskFile fref = NULL;
	UInt32 atom[2];

	err = FskFileOpen(xsbPath, kFskFilePermissionReadOnly, &fref);
	if (err) goto bail;

	err = FskFileRead(fref, sizeof(atom), atom, NULL);
	if (err) goto bail;

	atom[0] = FskEndianU32_BtoN(atom[0]);
	atom[1] = FskEndianU32_BtoN(atom[1]);
	if (atom[1] == 'XS11') {
		SInt32 totalSize = (SInt32)atom[0] - sizeof(atom);
		while (totalSize > 0) {
			UInt32 blockSize;
			char *block;

			err = FskFileRead(fref, sizeof(atom), atom, NULL);
			if (err) break;
			atom[0] = FskEndianU32_BtoN(atom[0]);
			atom[1] = FskEndianU32_BtoN(atom[1]);

			totalSize -= atom[0];

			blockSize = atom[0] - sizeof(atom);
			err = FskMemPtrNew(blockSize, &block);
			if (err) break;

			err = FskFileRead(fref, blockSize, block, NULL);
			if (err) break;

			switch (atom[1]) {
				case 'SYMB':
					theGrammar->symbols = block;
					theGrammar->symbolsSize = blockSize;
					break;

				case 'CODE':
					theGrammar->code = block;
					theGrammar->codeSize = blockSize;
					break;

				default:
					FskMemPtrDispose(block);
					err = kFskErrBadData;
					break;
			}
		}
	}
	else
		err = kFskErrBadData;

bail:
	FskFileClose(fref);
	return err;
}

#if mxWindows

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
    tp->tv_sec = (long)(tb.time);
    tp->tv_usec = tb.millitm * 1000;
  }
  if (tzp != 0) {
    tzp->tz_minuteswest = tb.timezone;
    tzp->tz_dsttime = tb.dstflag;
  }
  return (0);
}

#endif


#if mxLinuxArm
	unsigned long infinity[2]={0x00000000, 0x7ff00000};
#endif

#if mxWindows && _MSC_VER < 1800
	#define snprintf _snprintf
#endif

txString fxIntegerToString(txInteger theValue, txString theBuffer, txSize theSize)
{
	snprintf(theBuffer, theSize, "%ld", theValue);
	return theBuffer;
}
/*
txString fxNumberToString(txNumber theValue, txString theBuffer, txSize theSize, txByte theMode, txInteger thePrecision)
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

#if mxWindows

txString fxStringToUpper(txMachine* the, txString theString)
{
	txString result = NULL;
	UInt16 *unicodeText;
	UInt32 unicodeBytes;
	unsigned char *utf8Text;
	UInt32 utf8Bytes;

	if (kFskErrNone != FskTextUTF8ToUnicode16NE((const unsigned char *)theString, c_strlen(theString), &unicodeText, &unicodeBytes))
		return NULL;

	CharUpperBuffW((WCHAR *)unicodeText, unicodeBytes / 2);

	if (kFskErrNone == FskTextUnicode16NEToUTF8(unicodeText, unicodeBytes, (char **)&utf8Text, &utf8Bytes)) {
		result = (txString)fxNewChunk(the, utf8Bytes + 1);
		c_memmove(result, utf8Text, utf8Bytes + 1);

		FskMemPtrDispose(utf8Text);
	}

	FskMemPtrDispose(unicodeText);

	return result;
}

txString fxStringToLower(txMachine* the, txString theString)
{
	txString result = NULL;
	int len = c_strlen(theString), i;
	Boolean needsConvert = false;
	UInt16 *unicodeText;
	UInt32 unicodeBytes;
	unsigned char *utf8Text, *p;
	UInt32 utf8Bytes;

	// scan to for high ascii and upper case
	for (utf8Text = theString, i = len; i; i--) {
		unsigned char c = *utf8Text++;
		if (c & 0x80) goto full;
		if (('A' <= c) && (c <= 'Z'))
			needsConvert = true;
	}

	if (!needsConvert) return theString;	// no change needed

	// 7 bit conversion is easy
	result = (txString)fxNewChunk(the, len + 1);
	for (utf8Text = theString, p = result, i = len + 1; i; i--) {
		unsigned char c = *utf8Text++;
		if (('A' <= c) && (c <= 'Z'))
			c += 'a' - 'A';
		*p++ = c;
	}

	return result;

full:
	if (kFskErrNone != FskTextUTF8ToUnicode16NE((const unsigned char *)theString, len, &unicodeText, &unicodeBytes))
		return NULL;

	CharLowerBuffW((WCHAR *)unicodeText, unicodeBytes >> 1);

	if (kFskErrNone == FskTextUnicode16NEToUTF8(unicodeText, unicodeBytes, (char **)&utf8Text, &utf8Bytes)) {
		result = (txString)fxNewChunk(the, utf8Bytes + 1);
		c_memmove(result, utf8Text, utf8Bytes + 1);

		FskMemPtrDispose(utf8Text);
	}

	FskMemPtrDispose(unicodeText);

	return result;
}

#elif mxMacOSX

txString fxStringChangeCase(txMachine* the, txString theString, txBoolean upperCase)
{
	txString 			result = NULL;
	CFStringRef 		cfString = NULL;
	CFMutableStringRef	mutableCFString = NULL;
	CFIndex 			numberOfUTF8Chars;
	CFRange 			range;

	cfString = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)theString, c_strlen(theString), kCFStringEncodingUTF8, false);
	if (cfString == NULL) goto bail;
	mutableCFString = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, cfString);
	if (mutableCFString == NULL) goto bail;

	if (upperCase)
		CFStringUppercase(mutableCFString, 0);
	else
		CFStringLowercase(mutableCFString, 0);

	range = CFRangeMake(0, CFStringGetLength(mutableCFString));
	CFStringGetBytes(mutableCFString, range, kCFStringEncodingUTF8, 0, false, NULL, 0, &numberOfUTF8Chars);

	result = fxNewChunk(the, numberOfUTF8Chars + 1);
	if (result == NULL) goto bail;

	CFStringGetBytes(mutableCFString, range, kCFStringEncodingUTF8, 0, false, (UInt8 *)result, numberOfUTF8Chars, NULL);
	result[numberOfUTF8Chars] = 0;

bail:
	if (cfString)
		CFRelease(cfString);

	if (mutableCFString)
		CFRelease(mutableCFString);

	if (!result) {
		result = fxNewChunk(the, 1);
		*result = 0;
	}
	
	return result;
}

txString fxStringToUpper(txMachine* the, txString theString)
{
	return fxStringChangeCase(the, theString, true);
}

txString fxStringToLower(txMachine* the, txString theString)
{
	return fxStringChangeCase(the, theString, false);
}

#else

// Latin-1 implementation

txString fxStringToUpper(txMachine* the, txString theString)
{
	txString result = NULL;
	UInt16 *unicodeText;
	UInt32 unicodeBytes;
	unsigned char *utf8Text;
	UInt32 utf8Bytes;
	UInt32 i;

	if (kFskErrNone != FskTextUTF8ToUnicode16NE((const unsigned char *)theString, c_strlen(theString), &unicodeText, &unicodeBytes))
		return NULL;

	for (i = 0; i < unicodeBytes / 2; i++) {
		UInt16 c = unicodeText[i];
		if (c < 0x080)
			unicodeText[i] = c_toupper(c);
		else if (c >= 0x0ff)
			;
		else if ((c < 0x0e0) || (0xf7 == c))
			;
		else
			unicodeText[i] = c - 0x20;
	}

	if (kFskErrNone == FskTextUnicode16NEToUTF8(unicodeText, unicodeBytes, (char **)(void*)&utf8Text, &utf8Bytes)) {
		result = fxNewChunk(the, utf8Bytes + 1);
		c_memmove(result, utf8Text, utf8Bytes + 1);

		FskMemPtrDispose(utf8Text);
	}

	FskMemPtrDispose(unicodeText);

	return result;
}

txString fxStringToLower(txMachine* the, txString theString)
{
	txString result = NULL;
	UInt16 *unicodeText;
	UInt32 unicodeBytes;
	unsigned char *utf8Text;
	UInt32 utf8Bytes;
	UInt32 i;

	if (kFskErrNone != FskTextUTF8ToUnicode16NE((const unsigned char *)theString, c_strlen(theString), &unicodeText, &unicodeBytes))
		return NULL;

	for (i = 0; i < unicodeBytes / 2; i++) {
		UInt16 c = unicodeText[i];
		if (c < 0x080)
			unicodeText[i] = c_tolower(c);
		else if ((c >= 0x0df) || (0xd7 == c))
			;
		else if (c < 0x0c0)
			;
		else
			unicodeText[i] = c + 0x20;
	}

	if (kFskErrNone == FskTextUnicode16NEToUTF8(unicodeText, unicodeBytes, (char **)(void*)&utf8Text, &utf8Bytes)) {
		result = fxNewChunk(the, utf8Bytes + 1);
		c_memmove(result, utf8Text, utf8Bytes + 1);

		FskMemPtrDispose(utf8Text);
	}

	FskMemPtrDispose(unicodeText);

	return result;
}

#endif

#include "FskFiles.h"

#ifdef mxDebug

#define __FSKECMASCRIPT_PRIV__
#define __FSKNETUTILS_PRIV__
#include "FskECMAScript.h"

#include "FskNetUtils.h"

#ifdef KPR_CONFIG

static char debugAddress[256] = "";
static Boolean debugAddressFailed = false;

static FskErr fxConnectComplete(FskSocket skt, void *refCon)
{
	txMachine *the = (txMachine *)refCon;
	if (NULL == skt || FskNetSocketGetLastError(skt)) {
		the->connection = 0;
	}
	else
		the->connection = (int)skt;
	return kFskErrNone;
}

static void fxDoConnect(txMachine *the)
{
	if (!FskStrLen(debugAddress)) {
        char *address = FskEnvironmentGet("debugger");
		if (address)
			FskStrCopy(debugAddress, address);
#if TARGET_OS_WIN32 || (TARGET_OS_MAC && !TARGET_OS_IPHONE)
		else
			FskStrCopy(debugAddress, "localhost");
#endif
	}
	if (FskStrLen(debugAddress) && !debugAddressFailed) {
		FskErr err;
		char host[64];
		char* colon;
		int port;
		FskTimeRecord expireTime, now;
		SInt32 waitMS;
		
		colon = FskStrNCopyUntil(host, debugAddress, 63, ':');
		if (*colon)
			port = FskStrToNum(colon);
		else
			port = 5002;
			
#if TARGET_OS_MAC
		waitMS = 0;
#else
		waitMS = -1;
#endif
		// Connect to the host asynchronously
		err = FskNetConnectToHostPrioritized(host, port, false, fxConnectComplete, the, 0, 0, NULL, "fxConnect");
		if (err) goto bail;
		
		// Allow two seconds for the connection to resolve and for the socket to become readable and writable
		FskTimeGetNow(&expireTime);
		FskTimeAddSecs(&expireTime, 2);
		while (!the->connection) {
			FskTimeGetNow(&now);
			if (FskTimeCompare(&expireTime, &now) > 0)
				goto bail;
			FskThreadRunloopCycle(waitMS);
		}
		while (!FskNetSocketIsReadable((FskSocket)the->connection) && !FskNetSocketIsWritable((FskSocket)the->connection)) {
			FskTimeGetNow(&now);
			if (FskTimeCompare(&expireTime, &now) > 0)
				goto bail;
			FskThreadRunloopCycle(waitMS);
		}
		
		// Make the socket blocking
		FskNetSocketMakeBlocking((FskSocket)the->connection);
		return;
		
bail:
		if (the->connection) {
			FskNetSocketClose((FskSocket)the->connection);
			the->connection = 0;
		}
		debugAddressFailed = true;
	}
}

#if TARGET_OS_MAC
static void fxConnectThreadProc(void *refcon)
{
	txMachine *the = refcon;
	fxDoConnect(the);
}
#endif

#if 0	// @@ BSF fix me...
void fxConnect(txMachine* the)
{
#if TARGET_OS_MAC
	FskThread thread = NULL;
	FskThreadCreate(&thread, fxConnectThreadProc, kFskThreadFlagsJoinable, the, "fxConnect");
#else
	fxDoConnect(the);
#endif
}
#endif


#if TARGET_OS_WIN32
#else
#include <fcntl.h>
#include <arpa/inet.h>
#endif

void fxConnect(txMachine* the)
{
	if (!FskStrLen(debugAddress)) {
        char *address = FskEnvironmentGet("debugger");
		if (address)
			FskStrCopy(debugAddress, address);
#if TARGET_OS_WIN32 || (TARGET_OS_MAC && !TARGET_OS_IPHONE)
		else
			FskStrCopy(debugAddress, "localhost");
#endif
	}
	if (FskStrLen(debugAddress)) {
		FskErr err = kFskErrNone;
		char host[64];
		char* colon;
		int port;
		UInt32 addr;
		struct sockaddr_in sockaddr;
		FskSocket skt = NULL;
		int	flag;
		
		colon = FskStrNCopyUntil(host, debugAddress, 63, ':');
		if (*colon)
			port = FskStrToNum(colon);
		else
			port = 5002;
		err = FskNetHostnameResolveQT(host, 0, &addr, NULL);
		if (err) goto bail;
		FskMemSet(&sockaddr, 0, sizeof(sockaddr));
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_port = htons(port);
		sockaddr.sin_addr.s_addr = htonl(addr);
		
		err = FskNetSocketNewTCPPrioritized(&skt, false, 0, "fxConnect");
		if (err) goto bail;
		
#if TARGET_OS_WIN32
		flag = 1;
		if (ioctlsocket(skt->platSkt, FIONBIO, &flag))
			goto bail;
#else
		flag = fcntl(skt->platSkt, F_GETFL, 0);
		if (fcntl(skt->platSkt, F_SETFL, flag | O_NONBLOCK) < 0)
			goto bail;
#endif

		if (connect(skt->platSkt, (struct sockaddr*)(void*)&sockaddr, sizeof(sockaddr)) < 0) {
			struct timeval aTimeout = { 10, 0 }; // 10 seconds, 0 micro-seconds
			fd_set aReadSet;
			fd_set aWriteSet;
#if TARGET_OS_WIN32
			fd_set aexceptSet;
			if (WSAGetLastError() != WSAEWOULDBLOCK)
				goto bail;
#else
			if (errno != EINPROGRESS)
				goto bail;
#endif
			FD_ZERO(&aReadSet);
			FD_SET(skt->platSkt, &aReadSet);
			aWriteSet = aReadSet;

#if TARGET_OS_WIN32
			// See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms740141(v=vs.85).aspx
			aexceptSet = aReadSet;
			if (select(skt->platSkt + 1, &aReadSet, &aWriteSet, &aexceptSet, &aTimeout) <= 0)
                goto bail;
            if (!(FD_ISSET(skt->platSkt, &aReadSet)) && !(FD_ISSET(skt->platSkt, &aWriteSet)) && !(FD_ISSET(skt->platSkt, &aexceptSet)))
				goto bail;
#else
			if (select(skt->platSkt + 1, &aReadSet, &aWriteSet, NULL, &aTimeout) <= 0)
                goto bail;
			if (!(FD_ISSET(skt->platSkt, &aReadSet)) && !(FD_ISSET(skt->platSkt, &aWriteSet)))
				goto bail;
#endif
            }
		
#if TARGET_OS_WIN32
		flag = 0;
		if (ioctlsocket(skt->platSkt, FIONBIO, &flag))
			goto bail;
#else
		if (fcntl(skt->platSkt, F_SETFL, flag) < 0)
			goto bail;
#endif
		
		the->connection = (txSocket)skt;
		return;
bail:		
		FskNetSocketClose(skt);
		debugAddressFailed = true;
	}
}	

void fxDisconnect(txMachine* the)
{
	if (0 == the->connection)
		return;
	FskNetSocketClose((FskSocket)the->connection);
	the->connection = 0;
}

char *fxGetAddress(txMachine* the)
{
	return debugAddress;
}

txBoolean fxGetAutomatic(txMachine* the)
{
	return 1;
}

txBoolean fxIsConnected(txMachine* the)
{
	return the->connection != 0;
}

void fxSetAddress(txMachine* the, char* theAddress)
{
	FskStrCopy(debugAddress, theAddress);
	debugAddressFailed = false;
}

void fxSetAutomatic(txMachine* the, txBoolean theAutomatic)
{
}

#else /* !KPR_CONFIG */ 

static FskErr fxConnectComplete(FskSocket skt, void *refCon);
static Boolean connectFailed = false;
static char debugAddress[256] = "localhost:5002";

#if TARGET_OS_MAC || TARGET_OS_WIN32
	static txBoolean debugAutomatic = 1;
#elif TARGET_OS_KPL
	static txBoolean debugAutomatic = 1;	// @@ What to do here?
#else
	static txBoolean debugAutomatic = 0;
#endif

static void fxReadAddressAutomatic(txMachine* the);
static void fxWriteAddressAutomatic(txMachine* the);

FskErr fxConnectComplete(FskSocket skt, void *refCon)
{
	txMachine *the = (txMachine *)refCon;
	if (NULL == skt || FskNetSocketGetLastError(skt)) {
		connectFailed = true;
		the->connection = 0;
	}
	else
		the->connection = (int)skt;
	return kFskErrNone;
}

void fxConnect(txMachine* the)
{ 
	char *colon, name[64];
	int port;
//	if (connectFailed)
//		return;
	fxReadAddressAutomatic(the);
	colon = FskStrNCopyUntil(name, debugAddress, 63, ':');
	port = FskStrToNum(colon);
	// xsbug does not exist on localhost
#if TARGET_OS_ANDROID
	if (!c_strcmp(name, "localhost"))
		return;
#endif
	FskNetConnectToHostPrioritized(name, port, true, fxConnectComplete, the, kConnectFlagsSynchronous, 0, NULL, "fxConnect");		// priority of 0 so that this socket is invisible
}	

void fxDisconnect(txMachine* the)
{
	if (0 == the->connection)
		return;

	FskNetSocketClose((FskSocket)the->connection);
	the->connection = 0;
}

char *fxGetAddress(txMachine* the)
{
	fxReadAddressAutomatic(the);
	return debugAddress;
}

txBoolean fxGetAutomatic(txMachine* the)
{
#ifdef mxSDK
	//	SSL, kp5, MIME.diskThread, FskCore, sslroot
	if (c_strcmp(the->name, "kp5"))
		return false;
#endif

	fxReadAddressAutomatic(the);
	return debugAutomatic;
}

txBoolean fxIsConnected(txMachine* the)
{
	return the->connection != 0;
}

void fxReadAddressAutomatic(txMachine* the)
{
	/* read debugAddress and debugAutomatic from a file... */
#if TARGET_OS_ANDROID
	char *folder = NULL;
	char *file = FskStrDoCat(folder, "/sdcard/debug.txt");
	{
#else
	char *folder;
	if (kFskErrNone == FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeApplicationPreference, true, NULL, &folder)) {
		char *file = FskStrDoCat(folder, "debug.txt");
#endif
		char *data;

		if (kFskErrNone == FskFileLoad(file, (FskMemPtr *)&data, NULL)) {
			char *lf = FskStrChr(data, 10);
			if ((NULL != lf) && (FskStrLen(data) < sizeof(debugAddress))) {
				*lf = 0;
				FskStrCopy(debugAddress, data);
				debugAutomatic = '1' == lf[1];
			}
			FskMemPtrDispose(data);
		}
		else {
			FskECMAScriptGetDebug(&data, NULL, NULL);
			if (data) {
				FskStrCopy(debugAddress, data);
				FskStrCat(debugAddress, ":5002");
				debugAutomatic = 1;
			}
		}

		FskMemPtrDispose(file);
		FskMemPtrDispose(folder);
	}
}

void fxSetAddress(txMachine* the, char* theAddress)
{
	c_strcpy(debugAddress, theAddress);
	fxWriteAddressAutomatic(the);
}

void fxSetAutomatic(txMachine* the, txBoolean theAutomatic)
{
	debugAutomatic = theAutomatic;
	fxWriteAddressAutomatic(the);
}

void fxWriteAddressAutomatic(txMachine* the)
{
	/* write debugAddress and debugAutomatic to a file... */
#if TARGET_OS_ANDROID
	char *folder = NULL;
	char *file = FskStrDoCat(folder, "/sdcard/debug.txt");
	{
#else
	char *folder;
	if (kFskErrNone == FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeApplicationPreference, true, NULL, &folder)) {
		char *file = FskStrDoCat(folder, "debug.txt");
#endif
		FskFile fref;

		FskFileDelete(file);
		FskFileCreate(file);
		if (kFskErrNone == FskFileOpen(file, kFskFilePermissionReadWrite, &fref)) {
			char *data = FskStrDoCat(debugAddress, debugAutomatic ? "\n1\n" : "\n0\n");
			if (data) {
				FskFileWrite(fref, FskStrLen(data), data, NULL);
				FskFileClose(fref);
				FskMemPtrDispose(data);
			}
		}
	}
}

#endif /* !KPR_CONFIG */ 

void fxReadBreakpoints(txMachine* the)
{
	char *breakpoints;
	FskMemPtr file = NULL;
	FskInt64 fileSize;
	char *p;

	FskECMAScriptGetDebug(NULL, &breakpoints, NULL);

	if ((NULL == breakpoints) || (0 == *breakpoints)) goto bail;

	if (kFskErrNone != FskFileLoad(breakpoints, &file, &fileSize))
		goto bail;

	p = (char *)file;
	while (true) {
		char *path = p;
		char *line = FskStrChr(p, '\t');
		char *cr;

		if (NULL == line)
			break;
		cr = FskStrChr(line, '\n');
		if (NULL == cr)
			break;
		*line++ = 0;
		*cr = 0;

		fxSetBreakpoint(the, path, line);

		p = cr + 1;
	}

bail:
	FskMemPtrDispose(file);
}

void fxReceive(txMachine* the) 
{
	FskErr err;
	int amountRead;

	if (0 == the->connection) {
		the->echoOffset = 0;
		return;
	}

again:
	err = FskNetSocketRecvTCP((FskSocket)the->connection, the->echoBuffer, the->echoSize - 1, &amountRead);
	if (kFskErrNoData == err)
		goto again;

	if (kFskErrNone == err) {
		the->echoOffset = amountRead;
		the->echoBuffer[the->echoOffset] = 0;
	}
	else
		fxDisconnect(the);
}

void fxSend(txMachine* the) 
{
	FskErr err = kFskErrNone;
	char *buffer = the->echoBuffer;
	int bufferSize = the->echoOffset;

	if (0 == the->connection)
		return;

	while (bufferSize > 0) {
		int amount;

again:
		err = FskNetSocketSendTCP((FskSocket)the->connection, buffer, bufferSize, &amount);
		if (kFskErrNoData == err)
			goto again;
		if (kFskErrNone != err)
			break;
		bufferSize -= amount;
		buffer += amount;
	}

	if (kFskErrNone != err)
		fxDisconnect(the);
}

void fxWriteBreakpoints(txMachine* the)
{
	char *breakpoints;
	FskFile fref = NULL;
	txSlot *aBreakpoint = NULL;
	FskInt64 size = 0;

	FskECMAScriptGetDebug(NULL, &breakpoints, NULL);

	if ((NULL == breakpoints) || (0 == *breakpoints)) goto bail;

	FskFileCreate(breakpoints);

	if (kFskErrNone != FskFileOpen(breakpoints, kFskFilePermissionReadWrite, &fref))
		goto bail;

	if (kFskErrNone != FskFileSetSize(fref, &size))
		goto bail;

	while (true) {
		char aPath[1024], aLine[32];
		aBreakpoint = fxGetNextBreakpoint(the, aBreakpoint, aPath, aLine);
		if (NULL == aBreakpoint)
			break;
		FskFileWrite(fref, FskStrLen(aPath), (void *)aPath, NULL);
		FskFileWrite(fref, 1, (void *)"\t", NULL);
		FskFileWrite(fref, FskStrLen(aLine), (void *)aLine, NULL);
		FskFileWrite(fref, 1, (void *)"\n", NULL);
	}

bail:
	FskFileClose(fref);
}

#else

void fxConnect(txMachine* the) {}
void fxDisconnect(txMachine* the) {}
char *fxGetAddress(txMachine* the) { return ""; }
txBoolean fxGetAutomatic(txMachine* the) { return false; }
txBoolean fxIsConnected(txMachine* the) { return false; }
void fxReadBreakpoints(txMachine* the) {}
void fxReceive(txMachine* the) {}
void fxSend(txMachine* the) {}
void fxSetAddress(txMachine* the, char* theAddress) { }
void fxSetAutomatic(txMachine* the, txBoolean theAutomatic) {}
void fxWriteBreakpoints(txMachine* the) {}

#endif

void fxErrorMessage(txMachine* the, txInteger theCode, txString theBuffer, txSize theSize)
{
    const char *name = FskInstrumentationGetErrorString((FskErr)theCode);
    
    if (name && ('(' != name[0]) && (1 + FskStrLen(name)) < (unsigned long)theSize)
        FskStrCopy(theBuffer, name);
    else
        FskStrNumToStr(theCode, theBuffer, theSize);
}

#ifdef mxProfile

void fxCloseProfileFile(txMachine* the)
{ 
	FskFileClose(the->profileFile);
	the->profileFile = NULL;
}

void fxOpenProfileFile(txMachine* the, char* theName)
{ 
	FskFile fref;
	char *applicationPath = NULL;
	char *profilePath = NULL;
	if (the->profileDirectory)
		profilePath = FskStrDoCat(the->profileDirectory, theName);
	else {
		applicationPath = FskEnvironmentDoApply(FskStrDoCopy("[applicationPath]"));
		profilePath = FskStrDoCat(applicationPath, theName);
	}
	FskFileDelete(profilePath);
	FskFileCreate(profilePath);
	if (kFskErrNone == FskFileOpen(profilePath, kFskFilePermissionReadWrite, &fref))
		the->profileFile = fref;
	FskMemPtrDispose(profilePath);
	FskMemPtrDispose(applicationPath);
}

void fxWriteProfileFile(txMachine* the, void* theBuffer, txInteger theSize)
{
	if (the->profileFile)
		FskFileWrite(the->profileFile, theSize, theBuffer, NULL);
}

#endif /* mxProfile */
