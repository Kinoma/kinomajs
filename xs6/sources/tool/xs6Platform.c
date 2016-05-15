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
#ifdef mxParse
#include "xs6Script.h"
#endif
#if mxWindows

#elif mxMacOSX
	#include <CoreServices/CoreServices.h>
	#include <fcntl.h>
	#include <netdb.h>
	#include <sys/socket.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <pwd.h>
#else
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <pwd.h>
	#include <signal.h>
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

/* WINDOWS */

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
    tp->tv_sec = (long)tb.time;
    tp->tv_usec = tb.millitm * 1000;
  }
  if (tzp != 0) {
    tzp->tz_minuteswest = tb.timezone;
    tzp->tz_dsttime = tb.dstflag;
  }
  return (0);
}

char *realpath(const char *path, char *resolved_path)
{
	DWORD attributes;
	if (_fullpath(resolved_path, path, PATH_MAX) != NULL) {
		attributes = GetFileAttributes(resolved_path);
		if (attributes != 0xFFFFFFFF) {
			return resolved_path;
		}
	}
	return NULL;
}

#endif

/* DEBUG */

#ifdef mxDebug

void fxAddReadableCallback(txMachine* the)
{
}

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
#if mxMacOSX
	{
		int set = 1;
		setsockopt(the->connection, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
	}
#endif
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

txBoolean fxIsConnected(txMachine* the)
{
#if mxWindows
	return (the->connection != INVALID_SOCKET) ? 1 : 0;
#else
	return (the->connection > 0) ? 1 : 0;
#endif
}

txBoolean fxIsReadable(txMachine* the)
{
	return 0;
}

void fxReadBreakpoints(txMachine* the)
{
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

void fxRemoveReadableCallback(txMachine* the)
{
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
#if mxWindows
	SetEnvironmentVariable("XSBUG_HOST", theAddress);
#else
	setenv("XSBUG_HOST", theAddress, 1);
#endif
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


