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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "kprPins.h"
#include "FskECMAScript.h"
#include "FskExtensions.h"
#include "FskMemory.h"


#define bailIfError(X) { err = (X); if (err != kFskErrNone) goto bail; }
#define bailIfNoFile(X) { if ((X) == NULL) { err = kFskErrFileNotFound; goto bail; } }
#define bailIfNull(X) { if ((X) == NULL) { err = kFskErrFileNot; goto bail; } }

typedef struct GPIOsysfsStruct{  //GPIO
	int valueFile;
} FskGPIOSysfsRecord, *FskGPIOSysfs;

static const char* GPIOexport = "/sys/class/gpio/export";

static int I2CBUS = 0;
static const int FFDIGITALIN = 0x14;
static const int FFDIGITALOUT = 0x15;


FskErr FskGPIOPlatformSetDirection(FskGPIO gpio, GPIOdirection direction){
	if (gpio->realPin >= FRONTOFFSET){
		int pin = gpio->realPin - FRONTOFFSET;
		int slaveAddy = 0;
		if (pin < 8){
			slaveAddy = 0x20;
		}else{
			slaveAddy = 0x21;
			pin -= 8;
		}
		if (direction == in){
			FskHardwarePinsDoMux(pin, slaveAddy, 4);
		}else if(direction == out){
			FskHardwarePinsDoMux(pin, slaveAddy, 5);
		}
		gpio->direction = direction;
		return kFskErrNone;
	}else{
		FskErr err = kFskErrNone;
		FILE *f = NULL;
		int fd = -1;
		char buffer[50];

        if (undefined != direction) {
            snprintf(buffer, sizeof(buffer), "/sys/class/gpio/gpio%d/direction", gpio->realPin);
            f = fopen(buffer, "w+");
            bailIfNoFile(f);
			fprintf(f, (in == direction) ? "in" : "out");
            fclose(f);
            f = NULL;
        }

		snprintf(buffer, sizeof(buffer), "/sys/class/gpio/gpio%d/edge", gpio->realPin);
		fd = open(buffer, O_WRONLY);
		if (fd >= 0) {
			gpio->canInterrupt = write(fd, "both", 5) > 0;
			close(fd);
		}
		fd = -1;
		if (  ((FskGPIOSysfs)gpio->platform)->valueFile) {
			close(((FskGPIOSysfs)gpio->platform)->valueFile);
			((FskGPIOSysfs)gpio->platform)->valueFile = 0;		//@@
		}
		snprintf(buffer, sizeof(buffer), "/sys/class/gpio/gpio%d/value", gpio->realPin);
		fd = open(buffer, O_RDWR);
		if (fd <= 0) {
			err = kFskErrFileNotFound;
			goto bail;
		}
		((FskGPIOSysfs)gpio->platform)->valueFile = fd;

bail:
		if (err){
			gpio->direction = errorDir;
			return err;
		}else{
			gpio->direction = direction;
			return err;
		}
	}
}

FskErr FskGPIOPlatformGetDirection(FskGPIO gpio, GPIOdirection *direction)
{
	FskErr err = kFskErrNone;
	FILE *f = NULL;
	char buffer[50];
    signed char existingDirection = -1;

	if (gpio->realPin >= FRONTOFFSET) {
		*direction = gpio->direction;
		return kFskErrNone;
	}

	snprintf(buffer, sizeof(buffer), "/sys/class/gpio/gpio%d/direction", gpio->realPin);

    f = fopen(buffer, "r+");
    bailIfNoFile(f);
    rewind(f);
    fscanf(f, "%c", &existingDirection);
    rewind(f);
	fclose(f);

    if ('i' == existingDirection)
        *direction = in;
    else if ('o' == existingDirection)
        *direction = out;
    else
        *direction = undefined;
bail:
    return err;
}

FskErr FskGPIOPlatformInit(FskGPIO gpio){
	FskErr err = kFskErrNone;
	FILE *f = NULL;

	if (gpio->realPin < FRONTOFFSET){
		bailIfError(FskMemPtrNewClear(sizeof(FskGPIOSysfsRecord), (FskMemPtr *)&gpio->platform));
		f = fopen(GPIOexport, "w");
		bailIfNoFile(f);
		fprintf(f, "%d", gpio->realPin);
		fclose(f);
	}else{
		FskI2CDevOpen(I2CBUS);
	}
bail:
	return err;
}

FskErr FskGPIOPlatformWrite(FskGPIO gpio, GPIOvalue value){
	if (gpio->realPin < FRONTOFFSET)
		write(((FskGPIOSysfs)gpio->platform)->valueFile, (on == value) ? "1\n" : "0\n", 2);
	else {
		int32_t b = 0;
		int pin = gpio->realPin - FRONTOFFSET;

		if (pin < 8){
			FskI2CDevSetSlave(I2CBUS, 0x20);
		}else{
			FskI2CDevSetSlave(I2CBUS, 0x21);
			pin -= 8;
		}

		b = FskI2CDevReadByteDataSMB(I2CBUS, FFDIGITALOUT);

		if (value == on){
			b |= (1 << (pin));
		}else if (value == off){
			b &= ~(1 << (pin));
		}
		FskI2CDevWriteByteDataSMB(I2CBUS, FFDIGITALOUT, b);
	}
	return kFskErrNone;
}

GPIOvalue FskGPIOPlatformRead(FskGPIO gpio){
	if (gpio->realPin < FRONTOFFSET){
		int result, value = -1;
		char buffer[6];

		lseek(((FskGPIOSysfs)gpio->platform)->valueFile, 0, SEEK_SET);
		result = read(((FskGPIOSysfs)gpio->platform)->valueFile, buffer, sizeof(buffer) - 1);
		if (result < 1) return error;
		buffer[result - 1] = 0;
		value = FskStrToNum(buffer);
		if (value == 1) return on;
		if (value == 0) return off;
	}else{
		int32_t b = 0;
		int pin = gpio->realPin - FRONTOFFSET;
		if (pin < 8){
			FskI2CDevSetSlave(I2CBUS, 0x20);
		}else{
			FskI2CDevSetSlave(I2CBUS, 0x21);
			pin -= 8;
		}
		b = FskI2CDevReadByteDataSMB(I2CBUS, FFDIGITALIN);

		if (b < 0) return error;
		if (b & (1 << (pin))){
			return on;
		}else{
			return off;
		}
	}
	return error;
}

int FskGPIOPlatformGetFD(FskGPIO gpio)
{
	return gpio->canInterrupt ? ((FskGPIOSysfs)gpio->platform)->valueFile : -1;
}

FskErr FskGPIOPlatformDispose(FskGPIO gpio){
	if (gpio->realPin < FRONTOFFSET){
		if (gpio->platform){
			if (((FskGPIOSysfs)gpio->platform)->valueFile) close(((FskGPIOSysfs)gpio->platform)->valueFile);
			FskMemPtrDispose((FskGPIOSysfs)gpio->platform);
		}
	}

    return kFskErrNone;
}
