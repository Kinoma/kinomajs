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
#ifndef __K5_USB_DEVICE_H__
#define __K5_USB_DEVICE_H__

#include <stdint.h>

//#define USB_DEVICE_USE_4K_BUFFER			1

/* Resource Pool Configurations */
#define DCD_POOL_SIZE_BUFFER_PAGE			8
#define USB_DEVICE_POOL_SIZE_IR				32

#define USB_DEVICE_IR_QUEUE_SIZE			48

#define USB_DEVICE_ERROR_DTD_EXHAUSTED		0x01
#define USB_DEVICE_ERROR_BUFFER_OOM			0x02
#define USB_DEVICE_ERROR_INVALID_SIZE		0x03
#define USB_DEVICE_ERROR_INVALID_STATE		0x04

typedef struct {
	int epAddr;
	void* buffer;
	int actualLength;
	int totalLength;
	uint8_t status;
} UsbDeviceIORequest;

typedef struct {
	uint8_t numConfigurations;
	uint8_t *deviceDescriptor;
	void (*setConfiguration)			(int configuration);
	void (*handleClassRequest)			(void *setupData);
	void (*handleVendorRequest)			(void *setupData);
	void (*handleTranscationComplete)	(UsbDeviceIORequest *ir);
	uint8_t *configurationDescriptors[];
} UsbDeviceContext;

typedef enum {
	UsbDeviceStateNone,
	UsbDeviceStateDefault,
	UsbDeviceStateAddress,
	UsbDeviceStateConfigured
} UsbDeviceState;

void UsbDeviceInit(UsbDeviceContext *ctx, uint8_t **stringDescriptors);
UsbDeviceIORequest *UsbDeviceNewIORequest(int epAddr, void *buffer, int totalLength);
int UsbDeviceTransactionRequest(UsbDeviceIORequest *ir);
void UsbDeviceDisposeIORequest(UsbDeviceIORequest *ir);
UsbDeviceState UsbDeviceGetCurrentState();

#endif
