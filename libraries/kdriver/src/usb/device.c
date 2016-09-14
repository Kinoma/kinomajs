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

/**
 * USB Device Framework Implementation
 * NOTE: Thread-safe
 */

#include "88mw300.h"

#include "dcd.h"
#include "rsrc.h"

#include "mutex.h"
#include "usb/device.h"
#include "usb/common.h"
#include "usb/cdc.h"

/* Free RTOS */
#include <task.h>
#include <queue.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define EP0		0

#define min(x, y) (((x) < (y)) ? (x) : (y))

DcdDefineResourcePool(IR, USB_DEVICE_POOL_SIZE_IR, sizeof(UsbDeviceIORequest));

static UsbDeviceContext *ctx;
static uint8_t **stringDescriptors;

static UsbSetupData setupData;
static uint8_t currentConfiguration = 0;
static UsbDeviceState currentState = UsbDeviceStateNone;

static xSemaphoreHandle dcdMutex;		/* Mutex for DCD control */
static xQueueHandle irQueue;
static xSemaphoreHandle irMutex;		/* Mutex For IR resource */

static void controlLoop(void *arg)
{
	uint8_t epNum;
	uint8_t epDirection;
	UsbDeviceIORequest *ir;
	while (1) {
		if (!xQueueReceive(irQueue, &ir, 500)) {
			continue;
		}
		epNum = ir->epAddr & USB_ENDPOINT_NUMBER;
		epDirection = ir->epAddr & USB_ENDPOINT_DIRECTION;
		if (epNum == EP0) {
			/* Control Transfer - Status Phase */
			if (ir->status & DCD_DTD_STATUS_ERROR_MASK) {
				/* Error: Transaction Error  */
				DcdStallEndpoint(EP0);
			} else {
				if (ir->actualLength > 0) {
					/* Queue Status Phase */
					UsbDeviceIORequest *newIR = UsbDeviceNewIORequest(
						epNum | (epDirection ^ USB_ENDPOINT_DIRECTION), NULL, 0);
					UsbDeviceTransactionRequest(newIR);
				} /* else { } */ /* Status Phase Completed */
			}
			UsbDeviceDisposeIORequest(ir);
		} else {
			ctx->handleTranscationComplete(ir);
		}
	}
}

static void enqueueIR(UsbDeviceIORequest *ir)
{
	if (isISRContext()) {
		signed portBASE_TYPE xHigherPriorityTaskWoken = pdTRUE;
		xQueueSendFromISR(irQueue, &ir, &xHigherPriorityTaskWoken);
	} else {
		/* Wait Forever */
		while (!xQueueSend(irQueue, &ir, 1000));
	}
}

/******************************************************************************
 * Chapter 9 Commands
 ******************************************************************************/
static inline void setCurrentState(UsbDeviceState state)
{
	MutexLock(dcdMutex);
	currentState = state;
	MutexUnlock(dcdMutex);
}

static void setConfiguration()
{
	uint8_t index = 0;
	uint8_t *p;
	UsbStandardConfigurationDescriptor *configDescriptor;

	configDescriptor = (UsbStandardConfigurationDescriptor *)
		ctx->configurationDescriptors[currentConfiguration - 1];
	p = (uint8_t *) configDescriptor;
	/* Find Endpoint Descriptor then initialize it */
	while (index < configDescriptor->wTotalLength) {
		UsbStandardDescriptor *descriptor = (UsbStandardDescriptor *) p;
		if (descriptor->bDescriptorType == USB_DESCRIPTOR_TYPE_ENDPOINT) {
			UsbStandardEndpointDescriptor *epDescriptor =
				(UsbStandardEndpointDescriptor *) descriptor;
			DcdInitEndpoint(
				epDescriptor->bEndpointAddress,
				epDescriptor->bmAttributes & 0x03,
				epDescriptor->wMaxPacketSize);
		}
		index += descriptor->bLength;
		p += descriptor->bLength;
	}
}

static int getDescriptor(uint8_t type, uint8_t index, uint8_t *buffer, int length)
{
	UsbStandardDescriptor *descriptor = NULL;
	int actualLength = 0;

	switch (type) {
	case USB_DESCRIPTOR_TYPE_DEVICE:
		descriptor = (UsbStandardDescriptor *) ctx->deviceDescriptor;
		actualLength = descriptor->bLength;
		break;
	case USB_DESCRIPTOR_TYPE_CONFIGURATION:
		if (index < ctx->numConfigurations) {
			descriptor = (UsbStandardDescriptor *) ctx->configurationDescriptors[index];
			actualLength = ((UsbStandardConfigurationDescriptor *) descriptor)->wTotalLength;
		}
		break;
	case USB_DESCRIPTOR_TYPE_STRING:
		if (stringDescriptors != NULL) {
			descriptor = (UsbStandardDescriptor *) stringDescriptors[index];
			actualLength = descriptor->bLength;
		}
		break;
	}

	if (actualLength > 0) {
		actualLength = min(actualLength, length);
		memcpy(buffer, descriptor, actualLength);
		return actualLength;
	} else {
		return 0;
	}
}

static void handleEP0Setup()
{
	bool success = true;
	static uint8_t buffer[256];
	int length = 0;
	int direction;

	DcdReadSetupBuffer(EP0, &setupData, 0);

	length = setupData.wLength;
	if (!setupData.bmRequestType.bit.direction && length != 0) {
		direction = USB_ENDPOINT_DIRECTION_OUT;
	} else {
		direction = USB_ENDPOINT_DIRECTION_IN;
	}

	switch (setupData.bmRequestType.bit.type) {
	case USB_REQUEST_TYPE_STANDARD:
		switch (setupData.bRequest) {
		case USB_REQUEST_GET_CONFIGURATION:
			buffer[0] = currentConfiguration;
			length = 1;
			break;
		case USB_REQUEST_GET_DESCRIPTOR:
			length = getDescriptor(setupData.wValue >> 8, setupData.wValue & 0xFF, buffer, length);
			if (length == 0) {
				success = false;
			}
			break;
		case USB_REQUEST_GET_STATUS:
			switch (setupData.bmRequestType.bit.recipient) {
			case USB_REQUEST_RECIPIENT_DEVICE:
			case USB_REQUEST_RECIPIENT_INTERFACE:
			case USB_REQUEST_RECIPIENT_ENDPOINT:
				buffer[0] = 0;
				buffer[1] = 0;
				length = 2;
				break;
			default:
				success = false;
			}
			break;
		case USB_REQUEST_SET_ADDRESS:
			DcdSetDeviceAddress(setupData.wValue);
			setCurrentState(UsbDeviceStateAddress);
			break;
		case USB_REQUEST_SET_CONFIGURATION:
			currentConfiguration = setupData.wValue & 0xFF;
			if (currentConfiguration == 0) {
				/* TODO: Deconfigure */
				setCurrentState(UsbDeviceStateAddress);

			} else if (currentConfiguration <= ctx->numConfigurations) {
				if (currentConfiguration != 0) {
					/* TODO: Uncofigure */
				}
				setConfiguration();
				setCurrentState(UsbDeviceStateConfigured);
				/* Notify */
				ctx->setConfiguration(currentConfiguration);
			} else {
				success = false;
			}
			break;
		}
		break;
	case USB_REQUEST_TYPE_CLASS:
		if (ctx->handleClassRequest != NULL) {
			ctx->handleClassRequest(&setupData);
			/* Terminate here */
			return;
		} else {
			success = false;
		}
		break;
	case USB_REQUEST_TYPE_VENDOR:
		if (ctx->handleVendorRequest != NULL) {
			ctx->handleVendorRequest(&setupData);
			/* Terminate here */
			return;
		} else {
			success = false;
		}
		break;
	default:
		success = false;
	}

	if (!success) {
		DcdStallEndpoint(EP0);
		return;
	}

	/* Queue Data/Status Phase */
	UsbDeviceTransactionRequest(UsbDeviceNewIORequest(direction, buffer, length));
}

/******************************************************************************
 * DCD Callback
 ******************************************************************************/
static void eventCallback(DcdEventType eventType, int epAddr)
{
	uint8_t epNum = epAddr & USB_ENDPOINT_NUMBER;
	uint8_t epDirection = epAddr & USB_ENDPOINT_DIRECTION;
	DcdEndpointTransferDescriptor *dtd;
	UsbDeviceIORequest *ir;

	switch (eventType) {
	case DcdEventTypeReset:
		setCurrentState(UsbDeviceStateDefault);
		break;
	case DcdEventTypeSetup:
		if (epNum != EP0) {
			/* Error: Unsupported */
			DcdStallEndpoint(epNum);
			return;
		}
		handleEP0Setup();
		break;
	case DcdEventTypeTransactionComplete:
		do {
			MutexLock(dcdMutex);
			dtd = DcdRemoveCompletedTransferDescriptor(epAddr);
			MutexUnlock(dcdMutex);
			if (dtd == NULL) {
				break;	/* Terminate */
			}
			ir = (UsbDeviceIORequest *) dtd->private;
			if (ir != NULL) {
				ir->status = DcdGetTransferStatus(dtd);
				/* TODO: Data larger than 16KB */
				ir->actualLength = ir->totalLength - dtd->token.bit.totalBytes;
				enqueueIR(ir);
			}
			MutexLock(dcdMutex);
			DcdDisposeTransferDescriptor(dtd);
			MutexUnlock(dcdMutex);
		} while (true);
		break;
	}
}

/******************************************************************************
 * Public API
 ******************************************************************************/
void UsbDeviceInit(UsbDeviceContext *_ctx, uint8_t **_stringDescriptors)
{
	__disable_irq();
	NVIC_DisableIRQ(USBC_IRQn);

	ctx = _ctx;
	stringDescriptors = _stringDescriptors;

	/* Init FreeRTOS contexts */
	dcdMutex = xSemaphoreCreateMutex();
	irQueue = xQueueCreate(USB_DEVICE_IR_QUEUE_SIZE, sizeof(UsbDeviceIORequest *));
	irMutex = xSemaphoreCreateMutex();
	xTaskCreate(&controlLoop, "usb-device-control", 4096, NULL, 1, NULL);

	DcdInitResourcePool(DcdGetResourcePool(IR));
#ifdef USB_DEVICE_USE_4K_BUFFER
	/* Init Page Buffer */
	DcdInitResourcePool(DcdGetResourcePool(PAGE));
#endif
	/* Initialize USB Device Controller Driver */
	DcdInit(&eventCallback);

	/*
	 * Set priority below configMAX_SYSCALL_INTERRUPT_PRIORITY (0xa)
	 * because this interrupt handler will be using freertos APIs.
	 */
	NVIC_SetPriority(USBC_IRQn, configKERNEL_INTERRUPT_PRIORITY - 1);

	NVIC_EnableIRQ(USBC_IRQn);
	__enable_irq();
}

UsbDeviceIORequest *UsbDeviceNewIORequest(int epAddr, void *buffer, int totalLength)
{
	UsbDeviceIORequest *ir;

	MutexLock(irMutex);
	ir = (UsbDeviceIORequest *) DcdAllocResource(DcdGetResourcePool(IR));
	MutexUnlock(irMutex);
	if (ir != NULL) {
		memset(ir, 0, sizeof(UsbDeviceIORequest));
		ir->epAddr = epAddr;
		ir->buffer = buffer;
		ir->totalLength = totalLength;
	}
	return ir;
}

int UsbDeviceTransactionRequest(UsbDeviceIORequest *ir)
{
	int status = 0;
	DcdEndpointTransferDescriptor *dtd;
	/* FIXME: Transaction over 16KB is not supported yet */
	if (ir->totalLength > (DCD_BUFFER_PAGE_SIZE * (DCD_DTD_MAX_PAGE_COUNT - 1))) {
		return -USB_DEVICE_ERROR_INVALID_SIZE;
	}
	/* New DTD */
	MutexLock(dcdMutex);
	dtd = DcdNewTransferDescriptor(ir->totalLength, true);
	if (dtd == NULL) {
		status = -USB_DEVICE_ERROR_DTD_EXHAUSTED;
		goto done;
	}
	if (ir->totalLength > 0 && ir->buffer != NULL) {
		DcdSetTransferDescriptorBuffer(dtd, ir->buffer);
	}
	ir->status = DCD_DTD_STATUS_ACTIVE;
	ir->actualLength = 0;
	dtd->private = ir;
	DcdInsertTransferDescriptor(ir->epAddr, dtd);

done:
	MutexUnlock(dcdMutex);

	return status;
}

void UsbDeviceDisposeIORequest(UsbDeviceIORequest *ir)
{
	MutexLock(irMutex);
	DcdDisposeResource(DcdGetResourcePool(IR), ir);
	MutexUnlock(irMutex);
}

UsbDeviceState UsbDeviceGetCurrentState()
{
	UsbDeviceState state;
	MutexLock(dcdMutex);
	state = currentState;
	MutexUnlock(dcdMutex);
	return state;
}

/******************************************************************************
 * ISR Bridge
 ******************************************************************************/
void USB_IRQHandler(void)
{
	DcdISR();
}
