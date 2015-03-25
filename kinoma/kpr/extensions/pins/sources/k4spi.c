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
#include "FskECMAScript.h"
#include "FskExtensions.h"
#include "kprPins.h"
#include "FskMemory.h"

#define PLATFORM ((FskSPIK4)spi->platform)

extern int FskSPIDevOpen(int busNum, int chipSelect, int spiMode, unsigned int bitsPerWord, unsigned int speed);
extern FskErr FskSPIDevRead(int SPIFD, unsigned char *data, unsigned int length);
extern FskErr FskSPIDevWrite(int SPIFD, unsigned char *data, unsigned int length);
extern FskErr FskSPIDevFDXTransfer(int SPIFD, unsigned char *data, unsigned int length);
extern FskErr FskSPIDevClose(int SPIFD);

typedef struct SPIK4Struct{
	int file;
} FskSPIK4Record, *FskSPIK4;

FskErr FskSPIPlatformInit(FskSPI spi){
	FskErr err;
	if (spi->busNum < 100){
		err = FskMemPtrNewClear(sizeof(FskSPIK4Record), (FskMemPtr *)&spi->platform);
		if (err != kFskErrNone) return err;
		PLATFORM->file = FskSPIDevOpen(spi->busNum, spi->cs, spi->spiMode, spi->bitsPerWord, spi->speed);
		if (PLATFORM->file < 0) return kFskErrOperationFailed;
	}
	return kFskErrNone;
}

FskErr FskSPIPlatformRead(FskSPI spi, unsigned char *data, int length){
	if (spi->busNum < 100){
		return FskSPIDevRead(PLATFORM->file, data, length);
	}
	return kFskErrNone;
}

FskErr FskSPIPlatformWrite(FskSPI spi, unsigned char *data, int length){
	if (spi->busNum < 100){
		return FskSPIDevWrite(PLATFORM->file, data, length);
	}
	return kFskErrNone;
}

FskErr FskSPIPlatformFDXTransfer(FskSPI spi, unsigned char *data, int length){
	if (spi->busNum < 100){
		return FskSPIDevFDXTransfer(PLATFORM->file, data, length);
	}
	return kFskErrNone;
}

FskErr FskSPIPlatformDispose(FskSPI spi){
	if (spi->busNum < 100){
		if (spi->platform){
			FskErr err = FskSPIDevClose(PLATFORM->file);
			FskMemPtrDispose(PLATFORM);
			return err;
		}
		return kFskErrNone;
	}
	return kFskErrNone;
}
