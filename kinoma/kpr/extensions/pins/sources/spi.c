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
#ifdef USESPI

#include "FskECMAScript.h"
#include "FskExtensions.h"
#include "KplUIEvents.h"
#include "kprPins.h"
#include "FskMemory.h"

FskSPI spiList = NULL;

static FskErr FskSPIDispose(FskSPI spi){
	if (spi == spiList) spiList = spiList->next;
	FskSPIPlatformDispose(spi);
	FskMemPtrDispose(spi);
	return kFskErrNone;
}

static FskErr FskSPIRemove(int bus, int cs){
	FskSPI toRemove = NULL;
	FskSPI parent = NULL;
	if (spiList->busNum == bus && spiList->cs == cs){
		toRemove = spiList;
		spiList = toRemove->next;
	}else{
		parent = spiList;
		while (parent->next != NULL){
			if (parent->next->busNum == bus && parent->next->cs == cs){
				toRemove = parent->next;
				parent->next = toRemove->next;
				break;
			}else{
				parent = parent->next;
			}
		}
	}
	if (toRemove == NULL) return kFskErrFileNotFound;
	return FskSPIDispose(toRemove);
}

static FskSPI FskSPIFind(FskSPI spi, int bus, int cs){
	if (spi == NULL) return NULL;
	if (spi->busNum == bus && spi->cs == cs) return spi;
	return FskSPIFind(spi->next, bus, cs);
}

static FskErr FskSPIAdd(int bus, int cs, int spiMode, unsigned int bitsPerWord, unsigned int speed){
	FskErr err = kFskErrNone;
	FskSPI spi = NULL;

	spi = FskSPIFind(spiList, bus, cs);
	if (spi != NULL) return kFskErrOutOfSequence;
	err = FskMemPtrNewClear(sizeof(FskSPIRecord), (FskMemPtr *)&spi);
	if (err != kFskErrNone) return err;
	spi->busNum = bus;
	spi->cs = cs;
	spi->spiMode = spiMode;
	spi->bitsPerWord = bitsPerWord;
	spi->speed = speed;
	err = FskSPIPlatformInit(spi);
	if (err != kFskErrNone){
		FskSPIDispose(spi);
	}else{
		spi->next = spiList;
		spiList = spi;
	}
	return err;
}

static FskErr FskSPIRead(int bus, int cs, unsigned char *data, int length){
	FskSPI spi = FskSPIFind(spiList, bus, cs);
	if (spi == NULL) return kFskErrFileNotFound;
	return FskSPIPlatformRead(spi, data, length);
}

static FskErr FskSPIWrite(int bus, int cs, unsigned char *data, int length){
	FskSPI spi = FskSPIFind(spiList, bus, cs);
	if (spi == NULL) return kFskErrFileNotFound;
	return FskSPIPlatformWrite(spi, data, length);
}

static FskErr FskSPIFDXTransfer(int bus, int cs, unsigned char *data, int length){
	FskSPI spi = FskSPIFind(spiList, bus, cs);
	if (spi == NULL) return kFskErrFileNotFound;
	return FskSPIPlatformFDXTransfer(spi, data, length);
}

static int getBusFromPins(xsMachine *the){
	return 3;  //TODO
}

void xs_spi_init(xsMachine *the){
	FskErr err = kFskErrNone;
	int bus = getBusFromPins(the);
	int cs = xsToInteger(xsArg(1));
	int argc = xsToInteger(xsArgc);
	int mode = 0;
	int speed = 26000000;
	int bitsPerWord = 8;

	if (argc > 2) mode = xsToInteger(xsArg(2));
	if (argc > 3) speed = xsToInteger(xsArg(3));
	if (argc > 4) bitsPerWord = xsToInteger(xsArg(4));

	err = FskSPIAdd(bus, cs, mode, (unsigned int)speed, (unsigned int)bitsPerWord);
	if (err != kFskErrNone){
		xsTrace("Error initializing SPI.");
		xsError(err);
	}
}

void xs_spi_read(xsMachine *the){
	FskErr err = kFskErrNone;
	int bus = getBusFromPins(the);
	int cs = xsToInteger(xsArg(1));
	int length = xsToInteger(xsArg(2));
	int x = 0;

	unsigned char *data = NULL;
	err = FskMemPtrNewClear(sizeof(unsigned char) * length, &data);
	if (err != kFskErrNone){
		xsTrace("Error allocating buffer in spi read.\n");
		xsError(err);
	}
	err = FskSPIRead(bus, cs, data, length);
	if (err != kFskErrNone){
		xsTrace("Error reading from SPI.\n");
		xsError(err);
	}
	xsResult = xsNewInstanceOf(xsArrayPrototype);
	for (x = 0; x < length; x++){
		xsSetAt(xsResult, xsInteger(x), xsInteger( (unsigned int)*(data + x)));
	}
}

void xs_spi_write(xsMachine *the){
	FskErr err = kFskErrNone;
	int bus = getBusFromPins(the);
	int cs = xsToInteger(xsArg(1));
	unsigned char *data = NULL;
	int length = xsToInteger(xsGet(xsArg(2), xsID("length")));
	int i = 0;

	err = FskMemPtrNewClear(sizeof(unsigned char) * length, &data);
	if (err != kFskErrNone){
		xsTrace("Error allocating buffer in spi write.\n");
		xsError(err);
	}

	for (i = 0; i < length; i++){
		int value = xsToInteger(xsGetAt(xsArg(2), xsInteger(i)));
		if (value > 255){
			xsTrace("Your specified value is larger than one byte.\n");
			xsError(kFskErrInvalidParameter);
		}
		*(data + i) = (unsigned char)value;
	}
	err = FskSPIWrite(bus, cs, data, length);
	if (err != kFskErrNone){
		xsTrace("Error in SPI write.\n");
		xsError(err);
	}
}

void xs_spi_transfer(xsMachine *the){
	FskErr err = kFskErrNone;
	int bus = getBusFromPins(the);
	int cs = xsToInteger(xsArg(1));
	unsigned char *data = NULL;
	int i = 0;

	int length = xsToInteger(xsGet(xsArg(2), xsID("length")));

	err = FskMemPtrNewClear(sizeof(unsigned char) * length, &data);
	if (err != kFskErrNone){
		xsTrace("Error allocating buffer in spi write.\n");
		xsError(err);
	}

	for (i = 0; i < length; i++){
		int value = xsToInteger(xsGetAt(xsArg(2), xsInteger(i)));
		if (value > 255){
			xsTrace("Your specified value is larger than one byte.\n");
			xsError(kFskErrInvalidParameter);
		}
		*(data + i) = (unsigned char)value;
	}
	err = FskSPIFDXTransfer(bus, cs, data, length);
	if (err != kFskErrNone){
		xsTrace("Error in SPI transfer.\n");
		xsError(err);
	}
	xsResult = xsNewInstanceOf(xsArrayPrototype);
	for (i = 0; i < length; i++){
		xsSetAt(xsResult, xsInteger(i), xsInteger( (unsigned int)*(data + i)));
	}
}

void xs_spi_close(xsMachine *the){
	int bus = getBusFromPins(the);
	int cs = xsToInteger(xsArg(1));

	xsThrowIfFskErr(FskSPIRemove(bus,cs));
}
#endif //USESPI
