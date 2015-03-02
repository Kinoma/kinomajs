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
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "kprPins.h"
#include "FskExtensions.h"
#include "FskECMAScript.h"

static uint8_t a2dAddr1 = 0x20;
static uint8_t a2dAddr2 = 0x21;

static int initCount = 0;

#if SUPPORT_INSTRUMENTATION
   FskInstrumentedSimpleType(A2D, "A2D");
   // gA2DTypeInstrumentation
   #define         DBG_A2D(...)   do { FskInstrumentedTypePrintfDebug (&gA2DTypeInstrumentation, __VA_ARGS__); } while(0);
#else
    #define         DBG_A2D(format, ...) fprintf(stderr, format, ##__VA_ARGS__)
#endif  // SUPPORT_INSTRUMENTATION

static int a2dpins[8] = { 0x16, 0x18, 0x1A, 0x1C, 0x1E, 0x20, 0x22, 0x24 };

FskErr FskA2DPlatformInit(FskA2D a2d){
	int slaveAddy = 0;
	int pinNum = 0;

	FskErr err = kFskErrNone;
	initCount++;
	if (initCount == 1){
		FskI2CDevOpen(0);
	}


	pinNum = a2d->realPin;
	if (pinNum < 8){
		slaveAddy = a2dAddr1;
	}else{
		pinNum -= 8;
		slaveAddy = a2dAddr2;
	}
	FskHardwarePinsDoMux(pinNum, slaveAddy, 3);
	return err;
}

double FskA2DPlatformRead(FskA2D a2d){
  int a2d_Pin = a2d->realPin;
  double result = 0;
  int value = 0;
  if (a2d_Pin < 8){
	  FskI2CDevSetSlave(0, a2dAddr1);
  }else{
	  FskI2CDevSetSlave(0, a2dAddr2);
	  a2d_Pin -= 8;
  }


  value = FskI2CDevReadWordDataSMB(0, a2dpins[a2d_Pin]);

  result = ((double)value) / 1023.0;
  return result;
}

FskErr FskA2DPlatformDispose(FskA2D a2d) {
  FskErr err = kFskErrNone;
  return err;
}
