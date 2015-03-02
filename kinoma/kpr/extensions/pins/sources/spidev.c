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
//includes from spidev docs
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "FskECMAScript.h"
#include "FskExtensions.h"

#if SUPPORT_INSTRUMENTATION
   FskInstrumentedSimpleType(HWP, "HWP");
   // gA2DTypeInstrumentation
   #define         HWP_DEBUG(...)   do { FskInstrumentedTypePrintfDebug (&gHWPTypeInstrumentation, __VA_ARGS__); } while(0);
#else
    #define         HWP_DEBUG(format, ...) fprintf(stderr, format, ##__VA_ARGS__)
#endif  // SUPPORT_INSTRUMENTATION

static char spiDev[80];

FskErr FskSPIDevSetMode(int SPIFD, int spiMode){
	if (!(SPIFD)) return kFskErrOutOfSequence;
	int error = ioctl(SPIFD, SPI_IOC_WR_MODE, &spiMode);
	if (error < 0) return kFskErrOperationFailed;
	error = ioctl(SPIFD, SPI_IOC_RD_MODE, &spiMode);
	if (error < 0) return kFskErrOperationFailed;
	return kFskErrNone;
}

FskErr FskSPIDevSetBitsPerWord(int SPIFD, unsigned int bitsPerWord){
	if (!(SPIFD)) return kFskErrOutOfSequence;
	int error = ioctl(SPIFD, SPI_IOC_WR_BITS_PER_WORD, &bitsPerWord);
	if (error < 0) return kFskErrOperationFailed;
	error = ioctl(SPIFD, SPI_IOC_RD_BITS_PER_WORD, &bitsPerWord);
	if (error < 0) return kFskErrOperationFailed;
	return kFskErrNone;
}

FskErr FskSPIDevSetSpeed(int SPIFD, unsigned int speed){
	if (!(SPIFD)) return kFskErrOutOfSequence;
	int error = ioctl(SPIFD, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (error < 0) return kFskErrOperationFailed;
	error = ioctl(SPIFD, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (error < 0) return kFskErrOperationFailed;
	return kFskErrNone;
}

FskErr FskSPIDevClose(int SPIFD){
	if (!(SPIFD)) return kFskErrOutOfSequence;
	int error = close(SPIFD);
	if (error < 0){
		return kFskErrOperationFailed;
	}else{
		SPIFD = 0;
		return kFskErrNone;
	}
}

int FskSPIDevOpen(int busNum, int chipSelect, int spiMode, unsigned int bitsPerWord, unsigned int speed){
	 FskErr err = kFskErrNone;
	 int spidev = -1;
	 sprintf(spiDev, "/dev/spidev%d.%d", busNum, chipSelect);
	 if ((spidev = open(spiDev, O_RDWR)) < 0){
	   HWP_DEBUG("FskSPIDevOpen: Failed to open spi bus at %s\n", spiDev);
	   return -1;
	 }

	 err = FskSPIDevSetMode(spidev, spiMode);
	 if (err != kFskErrNone){
		 HWP_DEBUG("FskSPIDevOpen: Failed to set mode on spi bus at %s\n", spiDev);
		 close(spidev);
		 return -1;
	 }
	 err = FskSPIDevSetBitsPerWord(spidev, bitsPerWord);
	 if (err != kFskErrNone){
		 HWP_DEBUG("FskSPIDevOpen: Failed to set bits per word on spi bus at %s\n", spiDev);
		 close(spidev);
		 return -1;
	 }
	 err = FskSPIDevSetSpeed(spidev, speed);
	 if (err != kFskErrNone){
		 HWP_DEBUG("FskSPIDevOpen: Failed to set speed on spi bus at %s\n", spiDev);
		 close(spidev);
		 return -1;
	 }
	 return spidev;
}

FskErr FskSPIDevRead(int SPIFD, unsigned char *data, unsigned int length){
	if (!(SPIFD)) return kFskErrOutOfSequence;

	if (read(SPIFD, data, length) != length){
		HWP_DEBUG("FskSPIDevRead: Read failed.  Length %d was requested.\n", length);
		return kFskErrOperationFailed;
	}else{
		return kFskErrNone;
	}
}

FskErr FskSPIDevWrite(int SPIFD, unsigned char *data, unsigned int length){
	if (!(SPIFD)) return kFskErrOutOfSequence;

	if (write(SPIFD, data, length) < 0){
		HWP_DEBUG("FskSPIDevWrite: Write failed.\n");
		return kFskErrOperationFailed;
	}else{
		return kFskErrNone;
	}
}

FskErr FskSPIDevFDXTransfer(int SPIFD, unsigned char *data, unsigned int length){
	FskErr err = kFskErrNone;
	struct spi_ioc_transfer xfer[length];
	int i = 0;

	if (!(SPIFD)) return kFskErrOutOfSequence;

	for (i = 0; i < length; i++){
		xfer[i].tx_buf = (unsigned long)(data + i);
		xfer[i].rx_buf = (unsigned long)(data + i);
		xfer[i].delay_usecs = 0;
		xfer[i].cs_change = 0;
	}

	err = ioctl(SPIFD, SPI_IOC_MESSAGE(length), &xfer);
	if (err != 0){
		err = kFskErrOperationFailed;
		HWP_DEBUG("FskSPIDevFDXTransfer: Failed to complete SPI transfer.\n");
	}
	return err;
}
