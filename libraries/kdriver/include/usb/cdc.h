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
#ifndef __K5_USB_CDC_H__
#define __K5_USB_CDC_H__

#include <stdint.h>

#define CDC_V1_10								0x0110
#define CDC_V1_20 								0x0120

#define CDC_COMMUNICATION_INTERFACE_CLASS		0x02
#define CDC_DATA_INTERFACE_CLASS				0x0A

#define CDC_CS_INTERFACE						0x24
#define CDC_CS_ENDPOINT							0x25

#define CDC_CALL_MANAGEMENT						0x01
#define CDC_ABSTRACT_CONTROL_MODEL				0x02
#define CDC_HEADER								0x00
#define CDC_ABSTRACT_CONTROL_MANAGEMENT			0x02
#define CDC_UNION								0x06

#define CDC_SEND_ENCAPSULATED_COMMAND			0x00
#define CDC_GET_ENCAPSULATED_RESPONSE			0x01
#define CDC_SET_LINE_CODING						0x20
#define CDC_GET_LINE_CODING						0x21
#define CDC_SET_CONTROL_LINE_STATE				0x22

typedef struct {
	uint32_t dwDTERate;		/* Data terminal rate in bits per second */
	uint8_t bCharFormat;	/* Number of stop bits */
	uint8_t bParityType;	/* Parity bit type */
	uint8_t bDataBits;		/* Number of data bits */
} CdcLineCoding;

void CdcInit();
int CdcWrite(uint8_t *buffer, int off, int len);
int CdcRead(uint8_t *buffer, int off, int len);
#define CdcPrint(str)	CdcWrite(str, 0, strlen(str))

#endif
