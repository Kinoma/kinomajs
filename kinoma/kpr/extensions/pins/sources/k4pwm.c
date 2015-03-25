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

typedef struct PWMK4Struct{
	FILE *dutyCycle;
} FskPWMK4Record, *FskPWMK4;

FskErr FskPWMPlatformInit(FskPWM pwm){
	FILE *f = NULL;
	char buffer[60];
	FskErr err = kFskErrNone;

	bailIfError(FskMemPtrNewClear(sizeof(FskPWMK4Record), (FskMemPtr *)&pwm->platform));

	snprintf(buffer, 60, "/sys/class/backlight/pwm-backlight.%d/brightness", pwm->busNum);
	f = fopen(buffer, "w+");
	bailIfNoFile(f);
	setbuf(f, NULL);
	((FskPWMK4)pwm->platform)->dutyCycle = f;

	return err;
bail:
	return err;
}

FskErr FskPWMPlatformSetDutyCycle(FskPWM pwm, double value){
	int v = (int)(value * 1023.0);
	int test = fprintf(((FskPWMK4)pwm->platform)->dutyCycle, "%d\n", v);
	if (test < 0) return kFskErrUnknown;
	return kFskErrNone;
}

FskErr FskPWMPlatformGetDutyCycle(FskPWM pwm, double *value){
	int test = -1;
	int intvalue = -1;

	rewind(((FskPWMK4)pwm->platform)->dutyCycle);
	test = fscanf(((FskPWMK4)pwm->platform)->dutyCycle, "%d", &intvalue);
	if (test < 0){
		return kFskErrUnknown;
	}else{
		*value = ( ((double)intvalue) / 1023 );
		return kFskErrNone;
	}
}

FskErr FskPWMPlatformDispose(FskPWM pwm){
	FskPWMPlatformSetDutyCycle(pwm, 0);
	if (pwm->platform){
		if (((FskPWMK4)pwm->platform)->dutyCycle) fclose(((FskPWMK4)pwm->platform)->dutyCycle);
		FskMemPtrDispose((FskPWMK4)pwm->platform);
	}
	return kFskErrNone;
}
