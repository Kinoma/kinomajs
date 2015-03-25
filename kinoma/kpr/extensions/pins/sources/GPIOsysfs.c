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
#include "FskECMAScript.h"
#include "FskExtensions.h"
#include "kprPins.h"
#include "FskMemory.h"


#define bailIfError(X) { err = (X); if (err != kFskErrNone) goto bail; }
#define bailIfNoFile(X) { if ((X) == NULL) { err = kFskErrFileNotFound; goto bail; } }
#define bailIfNull(X) { if ((X) == NULL) { err = kFskErrFileNotOpen; goto bail; } }

typedef struct GPIOsysfsStruct{  //GPIO
	FILE *valueFile;
} FskGPIOSysfsRecord, *FskGPIOSysfs;

static const char* GPIOexport = "/sys/class/gpio/export";

FskErr FskGPIOPlatformSetDirection(FskGPIO gpio, GPIOdirection direction){
	FskErr err = kFskErrNone;
	FILE *f = NULL;
	char buffer[40];

	snprintf(buffer, 40, "/sys/class/gpio/gpio%d/direction", gpio->realPin);
	f = fopen(buffer, "w");
	bailIfNoFile(f);
	if (direction == in){
		fprintf(f, "in");
	}else if (direction == out){
		fprintf(f, "out");
	}
	fclose(f);

	if (  ((FskGPIOSysfs)gpio->platform)->valueFile != NULL) fclose(((FskGPIOSysfs)gpio->platform)->valueFile);
	snprintf(buffer, 40, "/sys/class/gpio/gpio%d/value", gpio->realPin);
	if (direction == in){
		f = fopen(buffer, "r+");
	}else if (direction == out){
		f = fopen(buffer, "w");
	}
	setbuf(f, NULL);
	bailIfNoFile(f);
	((FskGPIOSysfs)gpio->platform)->valueFile = f;

bail:
	if (err){
		gpio->direction = errorDir;
		return err;
	}else{
		gpio->direction = direction;
		return err;
	}
}

FskErr FskGPIOPlatformInit(FskGPIO gpio){
	FskErr err = kFskErrNone;
	FILE *f = NULL;

	bailIfError(FskMemPtrNewClear(sizeof(FskGPIOSysfsRecord), (FskMemPtr *)&gpio->platform));

	f = fopen(GPIOexport, "w");
	bailIfNoFile(f);
	fprintf(f, "%d", gpio->realPin);
	fclose(f);
bail:
	return err;
}

FskErr FskGPIOPlatformWrite(FskGPIO gpio, GPIOvalue value){
	fprintf(((FskGPIOSysfs)gpio->platform)->valueFile, "%d\n", value == on ? 1 : 0);
	return kFskErrNone;
}

GPIOvalue FskGPIOPlatformRead(FskGPIO gpio){
	int value;
	rewind(((FskGPIOSysfs)gpio->platform)->valueFile);
	int test = fscanf(((FskGPIOSysfs)gpio->platform)->valueFile, "%d", &value);
	if (test < 1) return error;
	if (value == 1) return on;
	if (value == 0) return off;
	return error;
}

FskErr FskGPIOPlatformDispose(FskGPIO gpio){
	if (((FskGPIOSysfs)gpio->platform)->valueFile) fclose(((FskGPIOSysfs)gpio->platform)->valueFile);
	if (gpio->platform) FskMemPtrDispose((FskGPIOSysfs)gpio->platform);
	return kFskErrNone;
}

