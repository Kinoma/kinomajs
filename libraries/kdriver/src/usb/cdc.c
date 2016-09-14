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
 * CDC/ACM USB Serial Driver
 * NOTE: Thread-safe
 */

#include "ringbuf.h"
#include "mutex.h"
#include "usb/device.h"
#include "usb/common.h"
#include "usb/cdc.h"

/* Free RTOS */
#include <portable.h>

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define USB_VID_MRVL							0x1286
#define USB_PID_MRVL							0x8080
#define USB_MFG_STR								"MARVELL"
#define USB_PROD_STR							"Kinoma Element"

#define CONFIGURATION_CDC						0x01
#define INTERFACE_CIF							0x00
#define INTERFACE_DIF							0x01

#define CONTROL_MAX_PACKET_SIZE					64
#define BULK_MAX_PACKET_SIZE					512

#define DATA_OUT_ADDR							0x02
#define DATA_IN_ADDR							0x82

#define BUFFER_SIZE								4096

static uint8_t deviceDescriptor[] = {
	/* Device Descriptor */
	USB_DESCRIPTOR_SIZE_DEVICE,					/* bLength */
	USB_DESCRIPTOR_TYPE_DEVICE,					/* bDescriptorType: DEVICE */
	UINT16LE(0x200),							/* bcdUSB: 2.00 */
	USB_DEVICE_CLASS_COMMUNICATIONS,			/* bDeviceClass: CDC */
	0,
	0,
	CONTROL_MAX_PACKET_SIZE,					/* bMaxPacketSize0 */
	UINT16LE(USB_VID_MRVL),						/* idVendor */
	UINT16LE(USB_PID_MRVL),						/* idProduct */
	UINT16LE(0x100),							/* bcdDevice: 1.00 */
	1,											/* iManufacturer: #1 */
	2,											/* iProduct: #2 */
	0,											/* iSerialNumber: N/A */
	1,											/* bNumConfigurations: 1 */
};

static uint8_t configDescriptor[] = {
/**************** Configuration 1 ****************/
	/* Configuration #1 Descriptor */
	USB_DESCRIPTOR_SIZE_CONFIGUARTION,			// bLength
	USB_DESCRIPTOR_TYPE_CONFIGURATION,			// bDescriptorType: CONFIGURATION
	UINT16LE(75),								// wTotalLength
	2,											// bNumInterfaces: 2
	CONFIGURATION_CDC,							// bConfigurationValue
	0,											// iConfiguration: No string descriptor
	0xC0,										// bmAttributes: Self-powered
	USB_CONFIG_POWER_MA(100),					// bMaxPower
	/* Interface Association Descriptor */
	USB_DESCRIPTOR_SIZE_INTERFACE_ASSOC,		// bLength: Interface association Descriptor size
	USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION,	// bDescriptorType: INTERFACE ASSOCIATION
	INTERFACE_CIF,								// bFirstInterface
	2,											// bInterfaceCount
	USB_DEVICE_CLASS_COMMUNICATIONS,			// bFunctionClass
	CDC_ABSTRACT_CONTROL_MODEL,					// bFunctionSubClass
	0x00,										// bFunctionProtocol
	0x00,										// biFunction
/**************** Interface 0 ****************/
	/* Interface #0 Descriptor */
	USB_DESCRIPTOR_SIZE_INTERFACE,				// bLength
	USB_DESCRIPTOR_TYPE_INTERFACE,				// bDescriptorType: INTERFACE
	INTERFACE_CIF,								// bInterfaceNumber
	0x00,										// bAlternateSetting: N/A
	1,											// bNumEndpoints: 1x Int
	CDC_COMMUNICATION_INTERFACE_CLASS,			// bInterfaceClass: Communication Interface Class
	CDC_ABSTRACT_CONTROL_MODEL,					// bInterfaceSubClass: Abstract Control Model
	0,											// bInterfaceProtocol: no protocol used
	0x00,										// iInterface: N/A
	/* Header Functional Descriptor */
	0x05,										// bLength: Endpoint Descriptor size
	CDC_CS_INTERFACE,							// bDescriptorType: CS_INTERFACE
	CDC_HEADER,									// bDescriptorSubtype: Header Func Desc
	UINT16LE(CDC_V1_20),						// bcdCDC: 1.20
	/* Union Functional Descriptor */
	0x05,										// bFunctionLength
	CDC_CS_INTERFACE,							// bDescriptorType: CS_INTERFACE
	CDC_UNION,									// bDescriptorSubtype: Union func desc
	INTERFACE_CIF,								// bControlInterface: Communication class interface is master
	INTERFACE_DIF,								// bSubordinateInterface0: Data class interface is slave 0
	/* Call Management Functional Descriptor */
	0x05,										// bFunctionLength
	CDC_CS_INTERFACE,							// bDescriptorType: CS_INTERFACE
	CDC_CALL_MANAGEMENT,						// bDescriptorSubtype: Call Management Func Desc
	0x01,										// bmCapabilities: device handles call management
	INTERFACE_DIF,								// bDataInterface: CDC data IF ID
	/* Abstract Control Management Functional Descriptor */
	0x04,										// bFunctionLength
	CDC_CS_INTERFACE,							// bDescriptorType: CS_INTERFACE
	CDC_ABSTRACT_CONTROL_MANAGEMENT,			// bDescriptorSubtype: Abstract Control Management desc
	0x02,										// bmCapabilities: SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE supported
	/* Endpoint #1 Descriptor */
	USB_DESCRIPTOR_SIZE_ENDPOINT,				// bLength
	USB_DESCRIPTOR_TYPE_ENDPOINT,				// bDescriptorType: ENDPOINT
	0x81,										// bEndpointAddress
	USB_TRANSFER_TYPE_INTERRUPT,				// bmAttributes
	UINT16LE(CONTROL_MAX_PACKET_SIZE),			// wMaxPacketSize
	10,											// bInterval:
/**************** Interface 1 ****************/
	/* Interface #1 Descriptor */
	USB_DESCRIPTOR_SIZE_INTERFACE,				// bLength
	USB_DESCRIPTOR_TYPE_INTERFACE,				// bDescriptorType: INTERFACE
	INTERFACE_DIF,								// bInterfaceNumber
	0x00,										// bAlternateSetting: N/A
	2,											// bNumEndpoints: 1x In, 1x Out
	CDC_DATA_INTERFACE_CLASS,					// bInterfaceClass: Data Interface Class
	0,											// bInterfaceSubClass: no subclass available
	0,											// bInterfaceProtocol: no protocol used
	0x00,										// iInterface: N/A
	/* Endpoint #2 Descriptor */
	USB_DESCRIPTOR_SIZE_ENDPOINT,				// bLength
	USB_DESCRIPTOR_TYPE_ENDPOINT,				// bDescriptorType: ENDPOINT
	DATA_OUT_ADDR,								// bEndpointAddress
	USB_TRANSFER_TYPE_BULK,						// bmAttributes
	UINT16LE(BULK_MAX_PACKET_SIZE),				// wMaxPacketSize
	0x00,										// bInterval: ignore for Bulk transfer
	/* Endpoint #3 Descriptor */
	USB_DESCRIPTOR_SIZE_ENDPOINT,				// bLength
	USB_DESCRIPTOR_TYPE_ENDPOINT,				// bDescriptorType: ENDPOINT
	DATA_IN_ADDR,								// bEndpointAddress
	USB_TRANSFER_TYPE_BULK,						// bmAttributes
	UINT16LE(BULK_MAX_PACKET_SIZE),				// wMaxPacketSize
	0x00,										// bInterval: ignore for Bulk transfer
};

static uint8_t stringDescriptor0[] = {
	4,
	USB_DESCRIPTOR_TYPE_STRING,
	UINT16LE(0x0409),						/* wLANGID: English(US) */
};

static uint8_t stringDescriptor1[] = {
	16,
	USB_DESCRIPTOR_TYPE_STRING,
	'M', 0x00, 'A', 0x00, 'R', 0x00, 'V', 0x00, 'E', 0x00, 'L', 0x00, 'L', 0x00,
};

static uint8_t stringDescriptor2[] = {
	30,
	USB_DESCRIPTOR_TYPE_STRING,
	'K', 0x00, 'i', 0x00, 'n', 0x00, 'o', 0x00, 'm', 0x00, 'a', 0x00, ' ', 0x00,
	'E', 0x00, 'l', 0x00, 'e', 0x00, 'm', 0x00, 'e', 0x00, 'n', 0x00, 't', 0x00,
};

static uint8_t *stringDescriptors[] = {
	stringDescriptor0, stringDescriptor1, stringDescriptor2
};

static CdcLineCoding lineCoding = { 115200, 0, 0, 8 };
static uint8_t rxBuffer[BUFFER_SIZE];
static UsbDeviceIORequest *rxRequest;

static xSemaphoreHandle rbMutex;

RingbufDefine(CDC, BUFFER_SIZE);

static void setConfiguration(int configuration)
{
	if (configuration != CONFIGURATION_CDC) {
		return;
	}
	rxRequest = UsbDeviceNewIORequest(DATA_OUT_ADDR, rxBuffer, BUFFER_SIZE);
	/* Queue RX */
	UsbDeviceTransactionRequest(rxRequest);
}

static void handleClassRequest(void *setupPacket)
{
	UsbSetupData *setupData = (UsbSetupData *) setupPacket;
	int direction;
	int length = setupData->wLength;
	if (!setupData->bmRequestType.bit.direction && length != 0) {
		direction = USB_ENDPOINT_DIRECTION_OUT;
	} else {
		direction = USB_ENDPOINT_DIRECTION_IN;
	}
	switch (setupData->bRequest) {
	case CDC_SEND_ENCAPSULATED_COMMAND:
	case CDC_GET_ENCAPSULATED_RESPONSE:
		UsbDeviceTransactionRequest(UsbDeviceNewIORequest(direction, NULL, 0));
		break;
	case CDC_SET_LINE_CODING:
		/* Will Receive Data */
	case CDC_GET_LINE_CODING:
		UsbDeviceTransactionRequest(UsbDeviceNewIORequest(direction, &lineCoding, 7));
		break;
	case CDC_SET_CONTROL_LINE_STATE:
		UsbDeviceTransactionRequest(UsbDeviceNewIORequest(direction, NULL, 0));
		break;
	default:
		DcdStallEndpoint(0);
	}
}

static void handleTranscationComplete(UsbDeviceIORequest *ir)
{
	if (ir->epAddr == DATA_OUT_ADDR) {
		if (ir->actualLength > 0) {
			MutexLock(rbMutex);
			RingbufWrite(RingbufGet(CDC), rxBuffer, 0, ir->actualLength);
			MutexUnlock(rbMutex);
		}
		/* Queue RX */
		UsbDeviceTransactionRequest(rxRequest);
	} else {
		vPortFree(ir->buffer);
		UsbDeviceDisposeIORequest(ir);
	}
}

static UsbDeviceContext cdcContext = {
	.numConfigurations = 1,
	.deviceDescriptor = deviceDescriptor,
	.setConfiguration = &setConfiguration,
	.handleClassRequest = &handleClassRequest,
	.handleVendorRequest = NULL,
	.handleTranscationComplete = &handleTranscationComplete,
	.configurationDescriptors = {configDescriptor},
};

void CdcInit()
{
	rbMutex = xSemaphoreCreateMutex();
	UsbDeviceInit(&cdcContext, stringDescriptors);
}

int CdcWrite(uint8_t *buffer, int off, int len)
{
	UsbDeviceIORequest *txRequest;
	void *pv;
	if (UsbDeviceGetCurrentState() != UsbDeviceStateConfigured) {
		return -USB_DEVICE_ERROR_INVALID_STATE;
	}
	txRequest = UsbDeviceNewIORequest(DATA_IN_ADDR, NULL, len);
	if (txRequest == NULL) {
		return -USB_DEVICE_ERROR_BUFFER_OOM;
	}
	pv = pvPortMalloc(sizeof(uint8_t) * len);
	if (pv == NULL) {
		UsbDeviceDisposeIORequest(txRequest);
		return -USB_DEVICE_ERROR_BUFFER_OOM;
	}
	memcpy(pv, buffer + off, len);
	txRequest->buffer = pv;
	/* Queue TX */
	return UsbDeviceTransactionRequest(txRequest);
}

int CdcRead(uint8_t *buffer, int off, int len)
{
	uint32_t r;
	MutexLock(rbMutex);
	r = RingbufRead(RingbufGet(CDC), buffer, 0, len);
	MutexUnlock(rbMutex);
	return (int) r;
}
