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
#ifndef __KPRLIBRARY__
#define __KPRLIBRARY__

#define KPR_NO_GRAMMAR 1
#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KprPinsStruct KprPinsRecord, *KprPins;
struct KprPinsStruct {
	FskList listeners;
	FskInstrumentedItemDeclaration
};

typedef struct KprPinsListenerStruct KprPinsListenerRecord, *KprPinsListener;
struct KprPinsListenerStruct {
	KprPinsListener next;
	FskList pollers;
#ifdef DEVICE
	char* name;
#endif
	xsSlot pins;
	char* referrer;
	xsMachine* the;
	FskInstrumentedItemDeclaration
};

typedef struct KprPinsPollerStruct KprPinsPollerRecord, *KprPinsPoller;
struct KprPinsPollerStruct {
	KprPinsPoller next;
	KprPinsListener listener;
	xsSlot function;
	int interval;
	xsSlot instance;
	xsSlot parameters;
	int parametersAlways;
	char* path;
	xsSlot timer;
	FskTimeCallBack timeCallback;
	char* url;
	FskInstrumentedItemDeclaration
};

extern void KprPinsPollerRun(KprPinsPoller self);

#define MAXPIN 255
#define FRONTOFFSET 200
#define FSKGROUND (FskHardwarePinRecord){ .gpio = MAXPIN, .adc = MAXPIN, .pwm = MAXPIN, .i2cClock = MAXPIN, .i2cData = MAXPIN, .ssp = MAXPIN, .ccic = MAXPIN, .uartRX = MAXPIN, .uartTX = MAXPIN, .device = MAXPIN }
#define FSKPIN_INIT(...) (FskHardwarePinRecord){ .gpio = MAXPIN, .adc = MAXPIN, .pwm = MAXPIN, .i2cClock = MAXPIN, .i2cData = MAXPIN, .ssp = MAXPIN, .ccic = MAXPIN, .uartRX = MAXPIN, .uartTX = MAXPIN, .device = MAXPIN, ## __VA_ARGS__ }

typedef enum {
	kFskHardwarePinGPIO = 1,
	kFskHardwarePinADC,
	kFskHardwarePinPWM,
	kFskHardwarePinI2CClock,
	kFskHardwarePinI2CData,
	kFskHardwarePinSSP,
	kFskHardwarePinCCIC,
	kFskHardwarePinUARTRX,
	kFskHardwarePinUARTTX,
	kFskHardwarePinDevice
} FskHardwarePinFunction;

struct FskHardwarePinRecord{
	UInt8			gpio;
	UInt8			adc;
	UInt8			pwm;
	UInt8			i2cClock;
	UInt8			i2cData;
	UInt8			ssp;
	UInt8			ccic;
	UInt8			uartRX;
	UInt8			uartTX;
	UInt8			device;
};
typedef struct FskHardwarePinRecord FskHardwarePinRecord, *FskHardwarePin;

void FskHardwarePinsInit();
//This function will be called once when the HardwarePins extension loads.  Do any generic, one-time initialization necessary here.

int FskHardwarePinsMux(int physicalPinNum, FskHardwarePinFunction function);
// FskHardwarePinMux has two functions.  1) It converts physical pin numbers to platform-specific reference IDs per hardware function.  Return -1 if the physical pin does not provide that function.
// 2) If there is any pre-initialization work to be done to select the functionality of the pin (to mux an MFP, for example) do so here.  This function will always be called for a pin before the pin-type-specific Init is called.

void FskHardwarePinsDoMux(int pinNum, int slaveAddress, int pinType);


#if USEGPIO

typedef enum{in, out, undefined, errorDir} GPIOdirection;
typedef enum{on = 1, off = 0, error = -1, uninit = -2} GPIOvalue;

typedef struct GPIOstruct{  //GPIO
	struct GPIOstruct *next;

	int pinNum;
	int realPin;
	GPIOdirection direction;

	void *platform;

    KprPinsPoller       poller;
    int                 pollerValue;
    FskThread           thread;
} FskGPIORecord, *FskGPIO;

FskErr FskGPIOPlatformInit(FskGPIO gpio);	//Initialize platform-specific gpio elements.  Allocate and use gpio->platform as needed.
FskErr FskGPIOPlatformSetDirection(FskGPIO gpio, GPIOdirection direction);  // Set the direction of the GPIO.
FskErr FskGPIOPlatformGetDirection(FskGPIO gpio, GPIOdirection *direction);  // Get the direction of the GPIO.
FskErr FskGPIOPlatformWrite(FskGPIO gpio, GPIOvalue value);  //Write a value to the GPIO
GPIOvalue FskGPIOPlatformRead(FskGPIO gpio);	//Read a value from the GPIO
FskErr FskGPIOPlatformDispose(FskGPIO gpio);	//Dispose of platform-specific gpio elements.  If used, deallocate gpio->platform here.

#endif

#if USEPWM

typedef struct PWMstruct{  //PWM
	uint8_t busNum;
	UInt16 pinNum;

	void *platform;
	struct PWMstruct *next;
} FskPWMRecord, *FskPWM;

FskErr FskPWMPlatformInit(FskPWM pwm);	//Initialize platform-specific pwm elements.  Allocate and use pwm->platform as needed.
FskErr FskPWMPlatformSetDutyCycle(FskPWM pwm, double value);	//Set the pwm duty cycle.  Value will be [0,1] -- scale as needed for your platform.
FskErr FskPWMPlatformGetDutyCycle(FskPWM pwm, double* value);
FskErr FskPWMPlatformDispose(FskPWM pwm);	//Dispose of platform-specific pwm elements.  If used, deallocate pwm->platform here.

#endif

#if USESERIAL

typedef struct serialStruct{  //Serial IO
	uint8_t rxNum;
	uint8_t txNum;
	uint8_t ttyNum;

	void *platform;
	struct serialStruct *next;
} FskSerialIORecord, *FskSerialIO;

FskErr FskSerialIOPlatformInit(FskSerialIO sio, int baud);
FskErr FskSerialIOPlatformDispose(FskSerialIO sio);
FskErr FskSerialIOPlatformWriteChar(FskSerialIO sio, char c);
FskErr FskSerialIOPlatformWriteString(FskSerialIO sio, char* str, int len);
FskErr FskSerialIOPlatformReadCharBlocking(FskSerialIO sio, char* c);
FskErr FskSerialIOPlatformReadCharNonBlocking(FskSerialIO sio, char* c);
FskErr FskSerialIOPlatformRead(FskSerialIO sio, char** str, int* count, int maxCount);
FskErr FskSerialIOPlatformGetByteCount(FskSerialIO sio, int* count);
FskErr FskSerialIOPlatformClearBuffer(FskSerialIO sio);

#endif

#if USEI2C

typedef enum{PLAIN, SMB} I2C_BusType;
FskErr FskI2COpen(int busNum); //Use this if you need to set the default bus
FskErr FskI2CDevOpen(int busNum);				  //Use this otherwise.
FskErr FskI2CDevClose(int busNum);
int32_t FskI2CDevReadByte(int busNum, I2C_BusType i2cBusType);
FskErr FskI2CDevWriteByte(int busNum, I2C_BusType i2cBusType, uint8_t data);
FskErr FskI2CDevWriteQuickSMB(int busNum, uint8_t value);
FskErr FskI2CDevWriteWordDataSMB(int busNum, uint8_t reg, uint16_t value);
int32_t FskI2CDevProcessCallSMB(int busNum, uint8_t reg, uint16_t value);
int FskI2CDevReadBlockDataSMB(int busNum, uint8_t reg, uint8_t length, uint8_t *values);
FskErr FskI2CDevWriteBlockDataSMB(int busNum, uint8_t reg, uint8_t length, uint8_t *values);
FskErr FskI2CDevSetSlave(int busNum, uint8_t addr);
FskErr FskI2CDevWriteByteDataSMB(int busNum, uint8_t reg, uint8_t data);
int32_t FskI2CDevReadByteDataSMB(int busNum, uint8_t reg);
int32_t FskI2CDevReadWordDataSMB(int busNum, uint8_t reg);
FskErr FskI2CDevReadBlock(int busNum, I2C_BusType i2cBusType, SInt32 *count, uint8_t *values);
FskErr FskI2CDevWriteBlock(int busNum, I2C_BusType i2cBusType, uint8_t count, const uint8_t *values);

#endif

#if USEA2D

typedef struct a2dStruct{
	uint8_t pinNum;
	uint8_t realPin;

	void *platform;
	struct a2dStruct *next;
} FskA2DRecord, *FskA2D;

FskErr FskA2DPlatformInit(FskA2D a2d);  //Initialize platform-specific a2d elements.  Allocate and use a2d->platform as needed.
double FskA2DPlatformRead(FskA2D a2d);	//Read the analog value of a pin and return it as a double in the range [0,1]
FskErr FskA2DPlatformDispose(FskA2D a2d);  //Dispose of platform-specific a2d elements.  If used, deallocate a2d->platform here.
#endif

#if USESPI

typedef struct spiStruct{
	uint8_t busNum;
	uint8_t cs;
	int spiMode;
	unsigned int bitsPerWord;
	unsigned int speed;

	void *platform;
	struct spiStruct *next;
} FskSPIRecord, *FskSPI;

FskErr FskSPIPlatformInit(FskSPI spi);  //Initialize platform-specific SPI elements.  Allocate and use spi->platform as needed.
FskErr FskSPIPlatformRead(FskSPI spi, unsigned char *data, int length);  //Read length bytes into data.  Return kFskErrOperationFailed on failure.
FskErr FskSPIPlatformWrite(FskSPI spi, unsigned char *data, int length);  //Write length bytes from data onto the SPI bus.  Return kFskErrOperationFailed on failure.
FskErr FskSPIPlatformFDXTransfer(FskSPI spi, unsigned char *data, int length);  //Do a full duplex transfer of length bytes between data and the SPI bus.  Return kFskErrOperationFailed on failure.
FskErr FskSPIPlatformDispose(FskSPI spi);  //Dispose of platform-specific SPI elements.  If used, deallocate spi->platform here.
#endif

#ifdef __cplusplus
}
#endif

#endif
