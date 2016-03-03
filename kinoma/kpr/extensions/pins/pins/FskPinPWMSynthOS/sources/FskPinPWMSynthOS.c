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

#include "FskPin.h"

#include "kprPins.h"

#define FRONTPWMDEFAULTPERIOD 20
#define FRONTPWMCOUNT 3

static Boolean createFrontPWMCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber);
static FskErr createFrontPWMNew(FskPinPWM *pin, SInt32 number, const char *name);
void createFrontPWMDispose(FskPinPWM pin);
static FskErr createFrontPWMSetDutyCycle(FskPinPWM pin, double value);
static FskErr createFrontPWMGetDutyCycle(FskPinPWM pin, double *value);
static FskErr createFrontPWMSetDutyCycleAndPeriod(FskPinPWM pin, double dutyCycle, double period);
static FskErr createFrontPWMGetDutyCycleAndPeriod(FskPinPWM pin, double *dutyCycle, double *period);

//Front PWM

static int dutyCycleRegister[FRONTPWMCOUNT] = {0x48, 0x4A, 0x4C};
static int periodRegister[FRONTPWMCOUNT] = {0x47, 0x49, 0x4B};

FskPinPWMDispatchRecord gCreateFrontPWM = {
    createFrontPWMCanHandle,
    createFrontPWMNew,
    createFrontPWMDispose,
    createFrontPWMSetDutyCycle,
    createFrontPWMGetDutyCycle,
    createFrontPWMSetDutyCycleAndPeriod,
    createFrontPWMGetDutyCycleAndPeriod
};

typedef struct createFrontPWMRecord{
    FskPinPWMRecord                 pd;

    FskPinI2C                       i2c;
    UInt8                           position;
    UInt8                           slaveAddress;
    UInt16                          pinNum;

    struct createFrontPWMRecord     *mirror;
} createFrontPWMRecord, *createFrontPWM;

static createFrontPWM leftPWMs[FRONTPWMCOUNT] = { NULL };
static createFrontPWM rightPWMs[FRONTPWMCOUNT] = { NULL };
static UInt8 leftCount = 0;
static UInt8 rightCount = 0;


Boolean createFrontPWMCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber){
    if (NULL == name){
        int pinNum = FskHardwarePinsMux(number, kFskHardwarePinPWM);
        if (pinNum >= FRONTOFFSET){
            *remappedNumber = pinNum - FRONTOFFSET;
            return true;
        }
    }

    return false;
}

static void addPWMToArray(createFrontPWM cfpwm, createFrontPWM array[]){
    int i = 0;
    Boolean placed = false;
    UInt8 lower = 0;

    for (i = 0; i < FRONTPWMCOUNT; i++){
        if (placed == false && array[i] == NULL){
            array[i] = cfpwm;
            placed = true;
        }else if(array[i] != NULL){
            if (array[i]->pinNum > cfpwm->pinNum){
                (array[i]->position)++;
                if (array[i]->mirror != NULL){
                    (array[i]->mirror->position)++;
                }
            }else{
                lower++;
            }
        }
    }
    cfpwm->position = lower;
}

static Boolean removePWMFromArray(createFrontPWM cfpwm, createFrontPWM array[]){
    int i = 0;
    Boolean found = false;

    if (cfpwm->mirror != NULL){
        for (i = 0; i < FRONTPWMCOUNT; i++){
            if (array[i] == cfpwm){
                array[i] = cfpwm->mirror;
                cfpwm->mirror->mirror = NULL;
            }
        }
        return false;
    }

    for (i = 0; i < FRONTPWMCOUNT; i++){
        if (array[i] == cfpwm){
            array[i] = NULL;
            found = true;
        }
        if (array[i] != NULL && array[i]->pinNum > cfpwm->pinNum){
            (array[i]->position)--;
            if (array[i]->mirror) (array[i]->mirror->position)--;
        }
    }
    return found;
}

static createFrontPWM checkForMirror(UInt16 pinNum, createFrontPWM array[]){
    int i = 0;

    for (i = 0; i < FRONTPWMCOUNT; i++){
        if (array[i] != NULL && array[i]->pinNum == pinNum) return array[i];
    }

    return NULL;
}

FskErr createFrontPWMNew(FskPinPWM *pin, SInt32 number, const char *name){
    FskErr err;
    createFrontPWM cfpwm;
    UInt16 pinNum;
    UInt16 slaveAddress;
    UInt8 *counter;
    createFrontPWM *addTo = NULL;
    createFrontPWM mirror = NULL;

    if (number < 8){
        pinNum = number;
        slaveAddress = 0x20;
        counter = &leftCount;
        addTo = leftPWMs;
    }else{
        pinNum = number - 8;
        slaveAddress = 0x21;
        counter = &rightCount;
        addTo = rightPWMs;
    }

    mirror = checkForMirror(pinNum, addTo);

    if (mirror == NULL) if (*counter >= FRONTPWMCOUNT) return kFskErrTooMany;

    err = FskMemPtrNewClear(sizeof(createFrontPWMRecord), &cfpwm);
    if (err) return err;

    err = FskPinI2CNew(&cfpwm->i2c, 0, 0, 0);
    if (err){
        FskMemPtrDispose(cfpwm);
        return err;
    }

	cfpwm->pinNum = pinNum;
	cfpwm->slaveAddress = slaveAddress;

    if (mirror == NULL){
        (*counter)++;
        addPWMToArray(cfpwm, addTo);
    }else{
        cfpwm->mirror = mirror;
        mirror->mirror = cfpwm;
    }

    *pin = (FskPinPWM)cfpwm;
    return err;
}

void createFrontPWMDispose(FskPinPWM pin){
    Boolean decrement;
    createFrontPWM cfpwm = (createFrontPWM)pin;
    //createFrontPWMSetDutyCycle(pin, 0);
    FskPinI2CDispose(cfpwm->i2c);
    if (cfpwm->slaveAddress == 0x20){
        decrement = removePWMFromArray(cfpwm, leftPWMs);
        if (decrement) leftCount--;
    }
    if (cfpwm->slaveAddress == 0x21){
        decrement = removePWMFromArray(cfpwm, rightPWMs);
        if (decrement) rightCount--;
    }

    FskMemPtrDispose(cfpwm);
}

static FskErr createFrontPWMSetDutyCycle(FskPinPWM pin, double value){
    double dutyCycle;

    dutyCycle = FRONTPWMDEFAULTPERIOD * value;
    return createFrontPWMSetDutyCycleAndPeriod(pin, dutyCycle, FRONTPWMDEFAULTPERIOD);
}

static FskErr createFrontPWMGetDutyCycle(FskPinPWM pin, double *value){
    double dutyCycle, period;
    FskErr err;

    err = createFrontPWMGetDutyCycleAndPeriod(pin, &dutyCycle, &period);
    *value = dutyCycle / period;
    return err;
}

static FskErr createFrontPWMSetDutyCycleAndPeriod(FskPinPWM pin, double dutyCycle, double period){
    createFrontPWM cfpwm = (createFrontPWM)pin;
    FskErr err;
	UInt8 dutyCycleToWrite, periodToWrite;

	if (dutyCycle > period) dutyCycle = period;

	dutyCycleToWrite = (UInt8)( (dutyCycle * (1000.0/128.0)) + 0.5);
	periodToWrite = (UInt8)( (period * (1000.0/128.0)) + 0.5);

    err = FskPinI2CSetAddress(cfpwm->i2c, cfpwm->slaveAddress);
    if (err != kFskErrNone) return err;
    err = FskPinI2CWriteDataByte(cfpwm->i2c, periodRegister[cfpwm->position], periodToWrite);
    if (err != kFskErrNone) return err;
    err = FskPinI2CWriteDataByte(cfpwm->i2c, dutyCycleRegister[cfpwm->position], dutyCycleToWrite);
    return err;
}

static FskErr createFrontPWMGetDutyCycleAndPeriod(FskPinPWM pin, double *dutyCycle, double *period){
    createFrontPWM cfpwm = (createFrontPWM)pin;
    FskErr err;
	UInt8 dutyCycleReg, periodReg;

    err = FskPinI2CSetAddress(cfpwm->i2c, cfpwm->slaveAddress);
    if (err != kFskErrNone) return err;
    err = FskPinI2CReadDataByte(cfpwm->i2c, periodRegister[cfpwm->position], &periodReg);
    if (err != kFskErrNone) return err;
    err = FskPinI2CReadDataByte(cfpwm->i2c, dutyCycleRegister[cfpwm->position], &dutyCycleReg);

	*dutyCycle = dutyCycleReg * (128.0/1000.0);
	*period = periodReg * (128.0/1000.0);

	return err;
}


/*
	Extension
*/

FskExport(FskErr) FskPinPWMSynthOS_fskLoad(FskLibrary library)
{
    return FskExtensionInstall(kFskExtensionPinPWM, &gCreateFrontPWM);
}

FskExport(FskErr) FskPinPWMSynthOS_fskUnload(FskLibrary library)
{
    return FskExtensionUninstall(kFskExtensionPinPWM, &gCreateFrontPWM);
}
