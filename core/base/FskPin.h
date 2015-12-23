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
#ifndef __FSKPIN__
#define __FSKPIN__

#include "FskExtensions.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
	Digital Pin
*/

typedef struct FskPinDigitalRecord FskPinDigitalRecord;
typedef FskPinDigitalRecord *FskPinDigital;

typedef enum {
	kFskPinDigitalDirectionIn = 1,
	kFskPinDigitalDirectionOut,
	kFskPinDigitalDirectionUndefined,
	kFskPinDigitalDirectionError
} FskPinsDigitalDirection;

typedef void (*FskPinDigitalRepeatTriggerProc)(FskPinDigital pin, void *refCon);

typedef struct {
	Boolean		(*doCanHandle)(SInt32 number, const char *name, SInt32 *remappedNumber);

	FskErr		(*doNew)(FskPinDigital *pin, SInt32 number, const char *name, FskPinsDigitalDirection direction);
	void		(*doDispose)(FskPinDigital pin);

	FskErr		(*doSetDirection)(FskPinDigital pin, FskPinsDigitalDirection direction);
	FskErr		(*doGetDirection)(FskPinDigital pin, FskPinsDigitalDirection *direction);

	FskErr		(*doRead)(FskPinDigital pin, Boolean *value);
	FskErr		(*doWrite)(FskPinDigital pin, Boolean value);

	FskErr		(*doRepeat)(FskPinDigital pin, FskPinDigitalRepeatTriggerProc triggered, void *refCon);
} FskPinDigitalDispatchRecord, *FskPinDigitalDispatch;

struct FskPinDigitalRecord {
	FskPinDigitalDispatch			dispatch;
};

FskAPI(FskErr) FskPinDigitalNew(FskPinDigital *pin, SInt32 number, const char *name, FskPinsDigitalDirection direction);

#define FskPinDigitalDispose(pin) ((pin && (pin)->dispatch->doDispose) ? ((pin)->dispatch->doDispose)(pin) : kFskErrUnimplemented)

#define FskPinDigitalSetDirection(pin, direction) (pin->dispatch->doSetDirection ? (pin->dispatch->doSetDirection)(pin, direction) : kFskErrUnimplemented)
#define FskPinDigitalGetDirection(pin, direction) (pin->dispatch->doGetDirection ? (pin->dispatch->doGetDirection)(pin, direction) : kFskErrUnimplemented)
#define FskPinDigitalRead(pin, value) (pin->dispatch->doRead ? (pin->dispatch->doRead)(pin, value) : kFskErrUnimplemented)
#define FskPinDigitalWrite(pin, value) (pin->dispatch->doWrite ? (pin->dispatch->doWrite)(pin, value) : kFskErrUnimplemented)
#define FskPinDigitalRepeat(pin, triggered, refCon) (pin->dispatch->doRepeat ? (pin->dispatch->doRepeat)(pin, triggered, refCon) : kFskErrUnimplemented)

/*
	Analog Pin
*/

typedef struct FskPinAnalogRecord FskPinAnalogRecord;
typedef FskPinAnalogRecord *FskPinAnalog;

typedef struct {
	Boolean		(*doCanHandle)(SInt32 number, const char *name, SInt32 *remappedNumber);

	FskErr		(*doNew)(FskPinAnalog *pin, SInt32 number, const char *name);
	void		(*doDispose)(FskPinAnalog pin);

	FskErr		(*doRead)(FskPinAnalog pin, double *value);
} FskPinAnalogDispatchRecord, *FskPinAnalogDispatch;

struct FskPinAnalogRecord {
	FskPinAnalogDispatch			dispatch;
};

FskAPI(FskErr) FskPinAnalogNew(FskPinAnalog *pin, SInt32 number, const char *name);

#define FskPinAnalogDispose(pin) ((pin && (pin)->dispatch->doDispose) ? ((pin)->dispatch->doDispose)(pin) : kFskErrUnimplemented)

#define FskPinAnalogRead(pin, value) (pin->dispatch->doRead ? (pin->dispatch->doRead)(pin, value) : kFskErrUnimplemented)

/*
	I2C Pin
*/

typedef struct FskPinI2CRecord FskPinI2CRecord;
typedef FskPinI2CRecord *FskPinI2C;

typedef struct {
	Boolean		(*doCanHandle)(SInt32 sda, SInt32 sclk, SInt32 bus, SInt32 *remappedBus);

	FskErr		(*doNew)(FskPinI2C *pin, SInt32 sda, SInt32 sclk, SInt32 bus);
	void		(*doDispose)(FskPinI2C pin);

	FskErr		(*doSetAddress)(FskPinI2C pin, UInt8 address);

	FskErr		(*doReadByte)(FskPinI2C pin, UInt8 *byte);
	FskErr		(*doReadBytes)(FskPinI2C pin, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes);

	FskErr		(*doWriteByte)(FskPinI2C pin, UInt8 byte);
	FskErr		(*doWriteBytes)(FskPinI2C pin, SInt32 count, const UInt8 *bytes);

	FskErr		(*doReadDataByte)(FskPinI2C pin, UInt8 command, UInt8 *byte);
	FskErr		(*doReadDataWord)(FskPinI2C pin, UInt8 command, UInt16 *word);
	FskErr		(*doReadDataBytes)(FskPinI2C pin, UInt8 command, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes);

	FskErr		(*doWriteDataByte)(FskPinI2C pin, UInt8 command, UInt8 byte);
	FskErr		(*doWriteDataWord)(FskPinI2C pin, UInt8 command, UInt16 word);
	FskErr		(*doWriteDataBytes)(FskPinI2C pin, UInt8 command, SInt32 count, const UInt8 *bytes);

	FskErr		(*doProcessCall)(FskPinI2C pin, UInt8 command, UInt16 input, UInt16 *output);
} FskPinI2CDispatchRecord, *FskPinI2CDispatch;

#define kFskPinI2CNoBus (-1)

struct FskPinI2CRecord {
	FskPinI2CDispatch			dispatch;
};

FskAPI(FskErr) FskPinI2CNew(FskPinI2C *pin, SInt32 sda, SInt32 sclk, SInt32 bus);

#define FskPinI2CDispose(pin) ((pin && (pin)->dispatch->doDispose) ? ((pin)->dispatch->doDispose)((pin)) : kFskErrUnimplemented)

#define FskPinI2CSetAddress(pin, address) (pin->dispatch->doSetAddress ? (pin->dispatch->doSetAddress)(pin, address) : kFskErrUnimplemented)

#define FskPinI2CReadByte(pin, byte) (pin->dispatch->doReadByte ? (pin->dispatch->doReadByte)(pin, byte) : kFskErrUnimplemented)
#define FskPinI2CReadBytes(pin, bufferSize, bytesRead, bytes) (pin->dispatch->doReadBytes ? (pin->dispatch->doReadBytes)(pin, bufferSize, bytesRead, bytes) : kFskErrUnimplemented)

#define FskPinI2CWriteByte(pin, byte) (pin->dispatch->doWriteByte ? (pin->dispatch->doWriteByte)(pin, byte) : kFskErrUnimplemented)
#define FskPinI2CWriteBytes(pin, count, bytes) (pin->dispatch->doWriteBytes ? (pin->dispatch->doWriteBytes)(pin, count, bytes) : kFskErrUnimplemented)

#define FskPinI2CReadDataByte(pin, command, byte) (pin->dispatch->doReadDataByte ? (pin->dispatch->doReadDataByte)(pin, command, byte) : kFskErrUnimplemented)
#define FskPinI2CReadDataWord(pin, command, word) (pin->dispatch->doReadDataWord ? (pin->dispatch->doReadDataWord)(pin, command, word) : kFskErrUnimplemented)
#define FskPinI2CReadDataBytes(pin, command, bufferSize, bytesRead, bytes) (pin->dispatch->doReadDataBytes ? (pin->dispatch->doReadDataBytes)(pin, command, bufferSize, bytesRead, bytes) : kFskErrUnimplemented)

#define FskPinI2CWriteDataByte(pin, command, byte) (pin->dispatch->doWriteDataByte ? (pin->dispatch->doWriteDataByte)(pin, command, byte) : kFskErrUnimplemented)
#define FskPinI2CWriteDataWord(pin, command, word) (pin->dispatch->doWriteDataWord ? (pin->dispatch->doWriteDataWord)(pin, command, word) : kFskErrUnimplemented)
#define FskPinI2CWriteDataBytes(pin, command, count, bytes) (pin->dispatch->doWriteDataBytes ? (pin->dispatch->doWriteDataBytes)(pin, command, count, bytes) : kFskErrUnimplemented)

#define FskPinI2CProcessCall(pin, command, input, output) (pin->dispatch->doProcessCall ? (pin->dispatch->doProcessCall)(pin, command, input, output) : kFskErrUnimplemented)

/*
	PWM Pin
*/

typedef struct FskPinPWMRecord FskPinPWMRecord;
typedef FskPinPWMRecord *FskPinPWM;

typedef struct {
	Boolean		(*doCanHandle)(SInt32 number, const char *name, SInt32 *remappedNumber);

	FskErr		(*doNew)(FskPinPWM *pin, SInt32 number, const char *name);
	void		(*doDispose)(FskPinPWM pin);

	FskErr		(*doSetDutyCycle)(FskPinPWM pin, double value);
	FskErr		(*doGetDutyCycle)(FskPinPWM pin, double *value);
} FskPinPWMDispatchRecord, *FskPinPWMDispatch;

struct FskPinPWMRecord {
	FskPinPWMDispatch			dispatch;
};

FskAPI(FskErr) FskPinPWMNew(FskPinPWM *pin, SInt32 number, const char *name);

#define FskPinPWMDispose(pin) ((pin && (pin)->dispatch->doDispose) ? ((pin)->dispatch->doDispose)(pin) : kFskErrUnimplemented)

#define FskPinPWMSetDutyCycle(pin, value) (pin->dispatch->doSetDutyCycle ? (pin->dispatch->doSetDutyCycle)(pin, value) : kFskErrUnimplemented)
#define FskPinPWMGetDutyCycle(pin, value) (pin->dispatch->doGetDutyCycle ? (pin->dispatch->doGetDutyCycle)(pin, value) : kFskErrUnimplemented)

/*
	Serial Pin
*/

typedef struct FskPinSerialRecord FskPinSerialRecord;
typedef FskPinSerialRecord *FskPinSerial;

typedef void (*FskPinSerialRepeatTriggerProc)(FskPinSerial pin, void *refCon);

typedef struct {
	Boolean		(*doCanHandle)(SInt32 rxNumber, SInt32 txNumber, const char *name, char **remappedName);

	FskErr		(*doNew)(FskPinSerial *pin, SInt32 rxNumber, SInt32 txNumber, const char *name, SInt32 baud);
	void		(*doDispose)(FskPinSerial pin);

	FskErr		(*doWrite)(FskPinSerial pin, SInt32 bufferSize, const UInt8 *bytes);
	FskErr		(*doRead)(FskPinSerial pin, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes);

	FskErr		(*doDataReady)(FskPinSerial pin, FskPinSerialRepeatTriggerProc triggered, void *refCon);
} FskPinSerialDispatchRecord, *FskPinSerialDispatch;

struct FskPinSerialRecord {
	FskPinSerialDispatch			dispatch;
};

FskAPI(FskErr) FskPinSerialNew(FskPinSerial *pin, SInt32 rxNumber, SInt32 txNumber, const char *name, SInt32 baud);

#define FskPinSerialDispose(pin) ((pin && (pin)->dispatch->doDispose) ? ((pin)->dispatch->doDispose)(pin) : kFskErrUnimplemented)

#define FskPinSerialWrite(pin, bufferSize, bytes) (pin->dispatch->doWrite ? (pin->dispatch->doWrite)(pin, bufferSize, bytes) : kFskErrUnimplemented)
#define FskPinSerialRead(pin, bufferSize, bytesRead, bytes) (pin->dispatch->doRead ? (pin->dispatch->doRead)(pin, bufferSize, bytesRead, bytes) : kFskErrUnimplemented)

#define FskPinSerialDataReady(pin, triggered, refCon) (pin->dispatch->doDataReady ? (pin->dispatch->doDataReady)(pin, triggered, refCon) : kFskErrUnimplemented)


#ifdef __cplusplus
}
#endif

#endif
