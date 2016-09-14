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
#ifndef __K5_USB_COMMON_H__
#define __K5_USB_COMMON_H__

#include <stdint.h>

/* Misc */
#define UINT16LE(x)										(x & 0xFF),((x >> 8) & 0xFF)

/******************************************************************************
 * USB Definitions
 ******************************************************************************/
#define USB_REQUEST_GET_STATUS							0
#define USB_REQUEST_CLEAR_FEATURE						1
#define USB_REQUEST_SET_FEATURE							3
#define USB_REQUEST_SET_ADDRESS							5
#define USB_REQUEST_GET_DESCRIPTOR						6
#define USB_REQUEST_SET_DESCRIPTOR						7
#define USB_REQUEST_GET_CONFIGURATION					8
#define USB_REQUEST_SET_CONFIGURATION					9
#define USB_REQUEST_GET_INTERFACE						10
#define USB_REQUEST_SET_INTERFACE						11
#define USB_REQUEST_SYNCH_FRAME							12

#define USB_REQUEST_TYPE_STANDARD						0
#define USB_REQUEST_TYPE_CLASS							1
#define USB_REQUEST_TYPE_VENDOR							2
#define USB_REQUEST_TYPE_RESERVED						3 

#define USB_REQUEST_RECIPIENT_DEVICE					0
#define USB_REQUEST_RECIPIENT_INTERFACE					1
#define USB_REQUEST_RECIPIENT_ENDPOINT					2
#define USB_REQUEST_RECIPIENT_OTHER						3 

#define USB_DESCRIPTOR_SIZE_DEVICE						18
#define USB_DESCRIPTOR_SIZE_CONFIGUARTION				9
#define USB_DESCRIPTOR_SIZE_INTERFACE					9
#define USB_DESCRIPTOR_SIZE_ENDPOINT					7
#define USB_DESCRIPTOR_SIZE_INTERFACE_ASSOC				8

#define USB_DESCRIPTOR_TYPE_DEVICE						1
#define USB_DESCRIPTOR_TYPE_CONFIGURATION				2
#define USB_DESCRIPTOR_TYPE_STRING						3
#define USB_DESCRIPTOR_TYPE_INTERFACE					4
#define USB_DESCRIPTOR_TYPE_ENDPOINT					5
#define USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER			6
#define USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION	7
#define USB_DESCRIPTOR_TYPE_INTERFACE_POWER				8
#define USB_DESCRIPTOR_TYPE_OTG							9
#define USB_DESCRIPTOR_TYPE_DEBUG						10
#define USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION		11

#define USB_TRANSFER_TYPE_CONTROL						0
#define USB_TRANSFER_TYPE_ISOCHRONOUS					1
#define USB_TRANSFER_TYPE_BULK							2
#define USB_TRANSFER_TYPE_INTERRUPT						3

#define USB_DEVICE_CLASS_COMMUNICATIONS					0x02

#define USB_CONFIG_POWER_MA(mA)							((mA)/2)

#define USB_MAX_PACKET_SIZE_DEFAULT						8
#define USB_MAX_PACKET_SIZE_CONTROL						64

#define USB_ENDPOINT_DIRECTION				0x80
#define USB_ENDPOINT_DIRECTION_IN			0x80
#define USB_ENDPOINT_DIRECTION_OUT			0x00
#define USB_ENDPOINT_NUMBER					0x0F

typedef union {
	uint8_t all;
	struct {
		unsigned recipient: 5;
		unsigned type : 2;
		unsigned direction : 1;
	} __attribute__ ((packed)) bit;
} UsbSetupData_RequestType_t;

typedef struct {
	UsbSetupData_RequestType_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} __attribute__ ((packed)) UsbSetupData;

/******************************************************************************
 * Chapter 9 Definitions
 ******************************************************************************/

typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
} __attribute__ ((packed)) UsbStandardDescriptor;

typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;
} __attribute__ ((packed)) UsbStandardDeviceDescriptor;

typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint8_t bNumConfigurations;
	uint8_t reserved;
} __attribute__ ((packed)) UsbDeviceQualifierDescriptor;

typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	uint8_t bmAttributes;
	uint8_t bMaxPower;
} __attribute__ ((packed)) UsbStandardConfigurationDescriptor;

typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t iInterface;
} __attribute__ ((packed)) UsbStandardInterfaceDescriptor;

typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bEndpointAddress;        // the endpont number
	uint8_t bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t bInterval;
} __attribute__ ((packed)) UsbStandardEndpointDescriptor;

typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wLANGID;
}  __attribute__ ((packed)) UsbStringDescriptorLanguage;

typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t wString[];
}  __attribute__ ((packed)) UsbStringDescriptor;

#endif
