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
#include "i2c-dev.h"
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "kprPins.h"

#include "FskECMAScript.h"


static int i2c_fd[10] = {0,0,0,0,0,0,0,0,0,0};	//NB: This will go poorly on devices with more than 10 I2C buses.
static int i2c_fd_count[10] = {0,0,0,0,0,0,0,0,0,0};	//NB: This will go poorly on devices with more than 10 I2C buses.
static int i2c_slave[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};	//NB: This will go poorly on devices with more than 10 I2C buses.


#if SUPPORT_INSTRUMENTATION
   FskInstrumentedSimpleType(I2C, "I2C");
   // gI2CTypeInstrumentation
   #define         DBG_I2C(...)   do { FskInstrumentedTypePrintfDebug (&gI2CTypeInstrumentation, __VA_ARGS__); } while(0);
#else
    #define         DBG_I2C(format, ...)
#endif  // SUPPORT_INSTRUMENTATION

FskErr FskI2CDevOpen(int busNum)
{
    FskErr err = kFskErrNone;
    char i2cDev[80];

    if (i2c_fd[busNum]) {
        i2c_fd_count[busNum] += 1;
        return kFskErrNone;
    }

    sprintf(i2cDev, "%s%d", "/dev/i2c-", busNum);
    if ((i2c_fd[busNum] = open(i2cDev, O_RDWR)) < 0){
        DBG_I2C("FskI2COpen: Failed to open i2c port at %s\n", i2cDev);
        err = kFskErrOperationFailed;
    }
    else {
        i2c_slave[busNum] = -1;
        i2c_fd_count[busNum] = 1;
    }

    return err;
}

FskErr FskI2CDevClose(int busNum)
{
    FskErr err = kFskErrNone;

    i2c_fd_count[busNum] -= 1;
    if (i2c_fd_count[busNum] > 0)
        return kFskErrNone;

    if (close(i2c_fd[busNum]) < 0){
        DBG_I2C("FskI2CClose: Failed to close i2c port at bus %d\n", busNum);
        err = kFskErrOperationFailed;
    }
    else
        i2c_fd[busNum] = 0;

    return err;
}

FskErr FskI2CDevSetSlave(int busNum, uint8_t addr)
{
    FskErr err = kFskErrNone;

    if (i2c_slave[busNum] == addr)
        return kFskErrNone;
 
     if ((ioctl(i2c_fd[busNum], I2C_SLAVE, addr)) < 0) {
        DBG_I2C("FskI2CSetSlave: Failed to set slave address at %#x\n", addr);
        err = kFskErrOperationFailed;
     }
     else
        i2c_slave[busNum] = addr;

     return err;
}

typedef union i2c_smbus_data i2c_smbus_data;
struct i2c_smbus_ioctl_dataFSK {
    char                    read_write;
    __u8                    command;
    int                     size;
    void                    *data;      // i2c_smbus_data
};
typedef struct i2c_smbus_ioctl_dataFSK i2c_smbus_ioctl_dataFSK;

static __s32 doI2C(int file, char read_write, __u8 command, int size, void *data)
{
    i2c_smbus_ioctl_dataFSK parameters = {read_write, command, size, data};
    return ioctl(file, I2C_SMBUS, &parameters);
}

int32_t FskI2CDevReadByte(int busNum, I2C_BusType i2cBusType)
{
    uint8_t readBuf[2];

    if (i2cBusType == PLAIN) {
        if (read(i2c_fd[busNum], readBuf, 1) != 1) {
         DBG_I2C("FskI2CReadByte: Failed to read byte using plain IC2\n");
         return -1;
        }else{
        	return (int32_t)readBuf[0];
        }
    }
    else { // SMB i2c_smbus_read_byte
        union i2c_smbus_data data;
        int result = doI2C(i2c_fd[busNum], I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);		//@@
        if (result) {
            DBG_I2C("FskI2CReadByte: Failed to read byte using SMB\n");
            return -1;
        }

        return data.byte & 0x0ff;
    }
}

FskErr FskI2CDevReadBlock(int busNum, I2C_BusType i2cBusType, SInt32 *count, uint8_t *values)
{
    FskErr err = kFskErrNone;

    if (PLAIN == i2cBusType) {
        int result;

        result = read(i2c_fd[busNum], values, *count);
        if (result >= 0) {
            *count = result;
        	DBG_I2C("FskI2CDevReadBlock: Did read %d bytes to bus %d.\n", (int)*count, busNum);
        }
        else {
            DBG_I2C("FskI2CDevReadBlock: Failed to read using plain IC2. errno is %d which is %s\n", count, errno, strerror(errno));
            err = kFskErrOperationFailed;
        }
    }
    return err;
}

FskErr FskI2CDevWriteBlock(int busNum, I2C_BusType i2cBusType, uint8_t count, const uint8_t *values)
{
    FskErr err = kFskErrNone;

    if (PLAIN == i2cBusType) {
        if (write(i2c_fd[busNum], values, count) != count) {
            DBG_I2C("FskI2CWriteBytes: Failed to write %d bytes using plain IC2. errno is %d which is %s\n", count, errno, strerror(errno));
            err = kFskErrOperationFailed;
        }else{
        	DBG_I2C("FskI2CWriteBytes: Did write %d bytes to bus %d.\n", count, busNum);
        }
    }
    return err;
}

FskErr FskI2CDevWriteByte(int busNum, I2C_BusType i2cBusType, uint8_t byte)
{
    FskErr err = kFskErrNone;

    if (i2cBusType == PLAIN) {
        if (write(i2c_fd[busNum], &byte, 1) != 1) {
            DBG_I2C("FskI2CWriteByte: Failed to write byte %#x using plain IC2.\n", byte);
            err = kFskErrOperationFailed;
        }
    }
    else { // SMB i2c_smbus_write_byte
        union i2c_smbus_data data;
        data.byte = byte;
        int result = doI2C(i2c_fd[busNum], I2C_SMBUS_WRITE, byte, I2C_SMBUS_BYTE, &data);		//@@
        if (result < 0) {
            DBG_I2C("FskI2CWriteByte: Failed to write byte %#x using SMB\n", byte);
            err = kFskErrOperationFailed;
        }
    }

    return err;
}

int32_t FskI2CDevReadByteDataSMB(int busNum, uint8_t reg)
{
    union i2c_smbus_data data;
    int result = doI2C(i2c_fd[busNum], I2C_SMBUS_READ, reg, I2C_SMBUS_BYTE_DATA, &data);
    if (result) {
        DBG_I2C("FskI2CReadByteDataSMB: Failed to read byte from register %#x using SMB\n", reg);
        return -1;
    }

    return data.byte & 0x0ff;
}

FskErr FskI2CDevWriteByteDataSMB(int busNum, uint8_t reg, uint8_t byte)
{
    FskErr err = kFskErrNone;
    union i2c_smbus_data data;
    int result;

   	data.byte = byte;
    result = doI2C(i2c_fd[busNum], I2C_SMBUS_WRITE, reg, I2C_SMBUS_BYTE_DATA, &data);
    if (result < 0) {
        DBG_I2C("FskI2CWriteByteDataSMB: Failed to write byte %#x to device register %#x using SMB\n", byte, reg);
        err = kFskErrOperationFailed;
    }

    return err;
}

FskErr FskI2CDevWriteQuickSMB(int busNum, uint8_t value)
{
    FskErr err = kFskErrNone;
    int result = doI2C(i2c_fd[busNum], value, 0, I2C_SMBUS_QUICK, NULL);
    if (result < 0) {
        DBG_I2C("FskI2CWriteQuickSMB: Failed to quick write value %#x using SMB\n", value);
        err = kFskErrOperationFailed;
    }
    
    return err;
}

int32_t FskI2CDevReadWordDataSMB(int busNum, uint8_t reg)
{
    union i2c_smbus_data data;
    int result = doI2C(i2c_fd[busNum], I2C_SMBUS_READ, reg, I2C_SMBUS_WORD_DATA, &data);
    if (result) {
        DBG_I2C("FskI2CDevReadWordDataSMB: Failed to read word from register %#x using SMB\n", reg);
        return -1;
    }

    return data.word & 0x0ffff;
}

FskErr FskI2CDevWriteWordDataSMB(int busNum, uint8_t reg, uint16_t word)
{
    FskErr err = kFskErrNone;
    union i2c_smbus_data data;
    int result;

   	data.word = word;
    result = doI2C(i2c_fd[busNum], I2C_SMBUS_WRITE, reg, I2C_SMBUS_WORD_DATA, &data);
    if (result < 0) {
        DBG_I2C("FskI2CWriteByteDataSMB: Failed to write word %#x to device register %#x using SMB\n", word, reg);
        err = kFskErrOperationFailed;
    }

    return err;
}

int32_t FskI2CDevProcessCallSMB(int busNum, uint8_t reg, uint16_t word)
{
    union i2c_smbus_data data;
    int result;
    
    data.word = word;
    result = doI2C(i2c_fd[busNum], I2C_SMBUS_WRITE, reg, I2C_SMBUS_PROC_CALL, &data);
    if (result) {
        DBG_I2C("FskI2CProcessCallSMB: Failed to process call to register %#x with value %#x using SMB\n", reg, word);
        return -1;
    }
    
    return data.word & 0x0ffff;
}

int FskI2CDevReadBlockDataSMB(int busNum, uint8_t reg, uint8_t length, uint8_t *values)
{
    union i2c_smbus_data data;
    int result;

    result = doI2C(i2c_fd[busNum], I2C_SMBUS_READ, reg, I2C_SMBUS_I2C_BLOCK_BROKEN, &data);
    if (result) {
        DBG_I2C("FskI2CReadBlockDataSMB: Failed to read block from register %#x using SMB\n", reg);
        return 0;
    }
    
    result = data.block[1];
    memmove(values, &data.block[2], result);

    return result;
}

FskErr FskI2CDevWriteBlockDataSMB(int busNum, uint8_t reg, uint8_t length, uint8_t *values)
{
    FskErr err = kFskErrNone;
    union i2c_smbus_data data;
    int result;

    data.block[0] = (length > 32) ? 32 : length;
    memmove(&data.block[1], values, data.block[0]);

    result = doI2C(i2c_fd[busNum], I2C_SMBUS_WRITE, reg, I2C_SMBUS_I2C_BLOCK_BROKEN, &data);
    if (result < 0) {
        DBG_I2C("FskI2CWriteBlockDataSMB: Failed to write block to register %#x using SMB\n", reg);
        err = kFskErrOperationFailed;
    }
    
    return err;
}
