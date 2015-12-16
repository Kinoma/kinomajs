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
#include "kprPins.h"
#include "FskECMAScript.h"
#include "FskExtensions.h"
#include "kpr.h"
#include "kprMessage.h"

#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#define PINCOUNT 66
#define PWMCOUNT 4

#define bailIfError(X) { err = (X); if (err != kFskErrNone) goto bail; }
#define bailIfNoFile(X) { if ((X) == NULL) { err = kFskErrFileNotFound; goto bail; } }
#define bailIfNull(X) { if ((X) == NULL) { err = kFskErrFileNotOpen; goto bail; } }

static FskHardwarePinRecord pins[PINCOUNT + 1];
static int initCount = 0;

static int pinConfigRegs[8] = { 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B };
static int pinGo = 0x03;

void FskHardwarePinsInit(){
	if (initCount == 0){
		pins[0] = FSKGROUND;
		//J30
		pins[1] = FSKGROUND;
		pins[2] = FSKGROUND;
		pins[3] = FSKPIN_INIT( .gpio = 90);
		pins[4] = FSKPIN_INIT( .gpio = 37);
		pins[5] = FSKPIN_INIT( .gpio = 91);
		pins[6] = FSKPIN_INIT( .gpio = 38);
		pins[7] = FSKPIN_INIT( .gpio = 92);
		pins[8] = FSKPIN_INIT( .gpio = 39);
		pins[9] = FSKPIN_INIT( .gpio = 93);
		pins[10] = FSKPIN_INIT( .gpio = 40);
		pins[11] = FSKPIN_INIT( .gpio = 95);
		pins[12] = FSKPIN_INIT( .gpio = 41);
		pins[13] = FSKGROUND;
		pins[14] = FSKGROUND;
		pins[15] = FSKPIN_INIT( .gpio = 49);
		pins[16] = FSKPIN_INIT( .gpio = 42);
		pins[17] = FSKPIN_INIT( .gpio = 50);
		pins[18] = FSKPIN_INIT( .gpio = 44);
		pins[19] = FSKPIN_INIT( .gpio = 51);
		pins[20] = FSKPIN_INIT( .gpio = 45);
		pins[21] = FSKPIN_INIT( .gpio = 54);
		pins[22] = FSKPIN_INIT( .gpio = 46);
		pins[23] = FSKPIN_INIT( .gpio = 55);
		pins[24] = FSKPIN_INIT( .gpio = 48);
		pins[25] = FSKGROUND; //3.3v
		pins[26] = FSKGROUND;
		pins[27] = FSKPIN_INIT( .i2cData = 0 );
		pins[28] = FSKPIN_INIT( .gpio = 96, .pwm = 2 );
		pins[29] = FSKPIN_INIT( .i2cClock = 0);
		pins[30] = FSKPIN_INIT( .gpio = 104, .pwm = 3 );
		pins[31] = FSKPIN_INIT( .gpio = 99, .uartTX = 2 );
		pins[32] = FSKGROUND;
		pins[33] = FSKPIN_INIT( .gpio = 98, .uartRX = 2 );
		pins[34] = FSKPIN_INIT( .gpio = 97, .pwm = 1 );
		pins[35] = FSKGROUND;
		pins[36] = FSKGROUND;
		pins[37] = FSKPIN_INIT( .device = 6 );
		pins[38] = FSKPIN_INIT( .device = 7 );
		pins[39] = FSKPIN_INIT( .device = 4 );
		pins[40] = FSKPIN_INIT( .device = 5 );
		pins[41] = FSKGROUND;
		pins[42] = FSKGROUND;
		pins[43] = FSKPIN_INIT( .device = 2 );
		pins[44] = FSKPIN_INIT( .device = 3 );
		pins[45] = FSKGROUND;
		pins[46] = FSKGROUND;
		pins[47] = FSKPIN_INIT( .device = 0 );
		pins[48] = FSKPIN_INIT( .device = 1 );
		pins[49] = FSKGROUND; //3.3v Clean
		pins[50] = FSKGROUND; //5v

		pins[51] = FSKPIN_INIT( .device = 7 );
		pins[52] = FSKPIN_INIT( .device = 6 );
		pins[53] = FSKPIN_INIT( .device = 5 );
		pins[54] = FSKPIN_INIT( .device = 4 );
		pins[55] = FSKPIN_INIT( .device = 3 );
		pins[56] = FSKPIN_INIT( .device = 2 );
		pins[57] = FSKPIN_INIT( .device = 1 );
		pins[58] = FSKPIN_INIT( .device = 0 );

		pins[59] = FSKPIN_INIT( .device = 15 );
		pins[60] = FSKPIN_INIT( .device = 14 );
		pins[61] = FSKPIN_INIT( .device = 13 );
		pins[62] = FSKPIN_INIT( .device = 12 );
		pins[63] = FSKPIN_INIT( .device = 11 );
		pins[64] = FSKPIN_INIT( .device = 10 );
		pins[65] = FSKPIN_INIT( .device = 9 );
		pins[66] = FSKPIN_INIT( .device = 8 );

		initCount++;
	}
}

char* FskSerialIOPlatformGetDeviceFile(int ttyNum){
	char* result = NULL;
	FskMemPtrNewClear(sizeof(char) * 15, &result);
	sprintf(result, "/dev/ttyS%d", ttyNum);
	return result;
}

void FskHardwarePinsDoMux(int pinNum, int slaveAddress, int pinType){
	int check = 1;

	FskI2CDevSetSlave(0, slaveAddress);
	FskI2CDevWriteByteDataSMB(0, pinConfigRegs[pinNum] , pinType);
	FskI2CDevWriteByteDataSMB(0, pinGo, 1);

	while (check > 0){
		check = FskI2CDevReadByteDataSMB(0, pinGo);
	}
}



int FskHardwarePinsMux(int physicalPinNum, FskHardwarePinFunction function){
	if (physicalPinNum > PINCOUNT || physicalPinNum < 0){
		if (kFskHardwarePinGPIO == function){
            switch (physicalPinNum) {
                case 1000:
                    return  0;       // charging status bit 0
                case 1001:
                    return  1;       // charging status bit 1
                case 1022:
                    return 22;       // right header voltage select
                case 1026:
                    return 26;       // left header voltage select
                case 1027:
                    return 27;       // microUSB charger detect
                default:
                    break;
                
            }
		}
		return -1;
	}
	int value = MAXPIN;
	switch (function){
	case kFskHardwarePinGPIO:
		value = pins[physicalPinNum].gpio;
		if (value == MAXPIN){
			value = pins[physicalPinNum].device;
			if (value != MAXPIN){
				value += FRONTOFFSET;
			}
		}
		break;
	case kFskHardwarePinADC:
		value = pins[physicalPinNum].device;
		break;
	case kFskHardwarePinPWM:
		value = pins[physicalPinNum].pwm;
		break;
	case kFskHardwarePinI2CClock:
		value = pins[physicalPinNum].i2cClock;
		if (value == MAXPIN){
			value = pins[physicalPinNum].device;
			if (value != MAXPIN) value += 100;
		}
		break;
	case kFskHardwarePinI2CData:
		value = pins[physicalPinNum].i2cData;
		if (value == MAXPIN){
			value = pins[physicalPinNum].device;
			if (value != MAXPIN) value += 100;
		}
		break;
	case kFskHardwarePinSSP:
		value = pins[physicalPinNum].ssp;
		break;
	case kFskHardwarePinCCIC:
		value = pins[physicalPinNum].ccic;
		break;
	case kFskHardwarePinUARTRX:
		value = pins[physicalPinNum].uartRX;
		break;
	case kFskHardwarePinUARTTX:
		value = pins[physicalPinNum].uartTX;
		break;
	case kFskHardwarePinDevice:
		break;
	}

	if (value != MAXPIN){
		return value;
	}else{
		return -1;
	}
}
