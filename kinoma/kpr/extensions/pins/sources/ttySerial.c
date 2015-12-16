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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "kprPins.h"
#include "FskMemory.h"

extern char* FskSerialIOPlatformGetDeviceFile(int ttyNum); //per-platform mechanism for getting device file name from tty number

#define bailIfError(X) { err = (X); if (err != kFskErrNone) goto bail; }
#define bailIfNoFile(X) { if ((X) == NULL) { err = kFskErrFileNotFound; goto bail; } }
#define bailIfNull(X) { if ((X) == NULL) { err = kFskErrFileNotOpen; goto bail; } }

typedef struct TTYSerialIOStruct{
	struct termios config;
	speed_t baud;
	int ttyFile;
} FskTTYSerialIORecord, *FskTTYSerialIO;

static speed_t getBaud(int baud){
	switch (baud){
	case 0:
		return B0;
	case 50:
		return B50;
	case 75:
		return B75;
	case 110:
		return B110;
	case 134:
		return B134;
	case 150:
		return B150;
	case 200:
		return B200;
	case 300:
		return B300;
	case 600:
		return B600;
	case 1200:
		return B1200;
	case 1800:
		return B1800;
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	case 2000000:
		return B2000000;
	case 2500000:
		return B2500000;
	case 3000000:
		return B3000000;
	case 3500000:
		return B3500000;
	case 4000000:
		return B4000000;
	default:
		return B0;
	}
}

//   ((FskTTYSerialIO)sio->platform)

FskErr FskSerialIOPlatformInit(FskSerialIO sio, int baud){
	FskErr err = kFskErrNone;
	int test = 0;

	speed_t baudRate = getBaud(baud);
	if (baudRate != B9600 && baudRate != B19200 && baudRate != B38400 && baudRate != B57600 && baudRate != B115200 && baudRate != B230400 && baudRate != B460800 && baudRate != B921600){
		printf("Invalid baud rate for Aspen.\n");
		return kFskErrOperationFailed;
	}

	bailIfError(FskMemPtrNewClear(sizeof(FskTTYSerialIORecord), (FskMemPtr *)&sio->platform));

	((FskTTYSerialIO)sio->platform)->baud = baudRate;

	if (0 == sio->path[0]) {
		char* name = FskSerialIOPlatformGetDeviceFile(sio->ttyNum);
		((FskTTYSerialIO)sio->platform)->ttyFile = open(name, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
		FskMemPtrDispose(name);
	} else {
		((FskTTYSerialIO)sio->platform)->ttyFile = open(sio->path, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
	}
	err = fcntl(((FskTTYSerialIO)sio->platform)->ttyFile, F_SETFL, O_RDWR);
	if (err != kFskErrNone) bailIfError(kFskErrUnknown);
	tcgetattr(((FskTTYSerialIO)sio->platform)->ttyFile, &((FskTTYSerialIO)sio->platform)->config);

	cfmakeraw(&((FskTTYSerialIO)sio->platform)->config);

	((FskTTYSerialIO)sio->platform)->config.c_cflag |= CREAD | CLOCAL;
	((FskTTYSerialIO)sio->platform)->config.c_cflag &= ~(CSTOPB);


	test += cfsetospeed(&((FskTTYSerialIO)sio->platform)->config, baudRate);
	test += cfsetispeed(&((FskTTYSerialIO)sio->platform)->config, baudRate);
	if (test < 0){
		printf("Invalid baud setting to cfset*speed\n");
	}

	((FskTTYSerialIO)sio->platform)->config.c_cc[VTIME] = 0;
	((FskTTYSerialIO)sio->platform)->config.c_cc[VMIN] = 1;

	tcsetattr(((FskTTYSerialIO)sio->platform)->ttyFile, TCSAFLUSH, &((FskTTYSerialIO)sio->platform)->config);

	return kFskErrNone;
bail:
	FskSerialIOPlatformDispose(sio);
	return err;
}

FskErr FskSerialIOPlatformDispose(FskSerialIO sio){
	if (sio->platform){
		if (((FskTTYSerialIO)sio->platform)->ttyFile) close(((FskTTYSerialIO)sio->platform)->ttyFile);
		FskMemPtrDispose(((FskTTYSerialIO)sio->platform));
	}
	return kFskErrNone;
}

FskErr FskSerialIOPlatformWriteChar(FskSerialIO sio, char c){
	if (sio->platform && ((FskTTYSerialIO)sio->platform)->ttyFile){
		write(((FskTTYSerialIO)sio->platform)->ttyFile, &c, 1);
		return kFskErrNone;
	}
	return kFskErrFileNotOpen;
}

FskErr FskSerialIOPlatformWriteString(FskSerialIO sio, char* str, int len){
	if (sio->platform && ((FskTTYSerialIO)sio->platform)->ttyFile){
		write(((FskTTYSerialIO)sio->platform)->ttyFile, str, len);
		return kFskErrNone;
	}
	return kFskErrFileNotOpen;
}

FskErr FskSerialIOPlatformGetByteCount(FskSerialIO sio, int* count){
	int result = -1;
	int response;
	response = ioctl(((FskTTYSerialIO)sio->platform)->ttyFile,FIONREAD,&result);
	if (result >= 0 && response >= 0){
		*count = result;
		return kFskErrNone;
	}
	return kFskErrFileNotOpen;
}

FskErr FskSerialIOPlatformRead(FskSerialIO sio, char** str, int* count, int maxCount){
	int numBytes = 0;
	int bytesRead;
	FskErr err = kFskErrNone;


	err = FskSerialIOPlatformGetByteCount(sio, &numBytes);
	if (err != kFskErrNone) return err;

    if ((maxCount > 0) && (numBytes > maxCount))
        numBytes = maxCount;

	if (numBytes == 0){
		*count = 0;
		return kFskErrNoData;
	}

	err = FskMemPtrNewClear(numBytes + 1, str);
	if (err != kFskErrNone){
		printf("Error allocating memory in ReadFullString\n");
		return err;
	}else{
		bytesRead = read(((FskTTYSerialIO)sio->platform)->ttyFile, *str, numBytes);
		if (bytesRead > 0){
			*count = bytesRead;
			return kFskErrNone;
		}else if (bytesRead == 0){
			*count = 0;
			return kFskErrNoData;
		}else{
			*count = 0;
			return kFskErrFileNeedsRecovery;
		}
	}
}

FskErr FskSerialIOPlatformClearBuffer(FskSerialIO sio){
	if(!tcflush(((FskTTYSerialIO)sio->platform)->ttyFile, TCIOFLUSH))
		return kFskErrNone;
	return kFskErrFileNeedsRecovery;
}

FskErr FskSerialIOPlatformReadCharNonBlocking(FskSerialIO sio, char* c){
	int numBytes = 0;
	char w;
	FskErr err = kFskErrNone;

	err = FskSerialIOPlatformGetByteCount(sio, &numBytes);
	if (err != kFskErrNone) return err;
	if (numBytes < 1) return kFskErrNoData;

	if (read(((FskTTYSerialIO)sio->platform)->ttyFile, &w, 1) >= 0){
		*c = w;
		return kFskErrNone;
	}

	return errno;		//@@ not an FskErr
}

FskErr FskSerialIOPlatformReadCharBlocking(FskSerialIO sio, char* c){
	char w;
	if (read(((FskTTYSerialIO)sio->platform)->ttyFile, &w, 1) >= 0){
		*c = w;
		return kFskErrNone;
	}
	
	return errno;		//@@ not an FskErr
}

FskErr FskSerialIOPlatformReadBlocking(FskSerialIO sio, UInt32 bytesToRead, void *buffer, UInt32 *bytesRead)
{
	int count = read(((FskTTYSerialIO)sio->platform)->ttyFile, buffer, bytesToRead);
	if (count <= 0)
		return (EBADF == errno) ? kFskErrFileNotOpen : kFskErrNoData;

	*bytesRead = (UInt32)count;

	return kFskErrNone;
}

int FskSerialIOPlatformGetFD(FskSerialIO sio)
{
	return ((FskTTYSerialIO)sio->platform)->ttyFile;
}

