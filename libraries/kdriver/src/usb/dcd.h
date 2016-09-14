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
 * Chipidea USB Device Controller Driver
 *
 * NOTE: No ISO support
 * Thanks to http://atose.org/?page_id=258 for reference
 */

#include <stdint.h>
#include <stdbool.h>

/* Resource Pool Configurations */
#define DCD_POOL_SIZE_DTD					32

#define DCD_MAX_ENDPOINT 					8

#define DCD_ENDPOINT_DIRECTION				0x80
#define DCD_ENDPOINT_NUMBER					0x0F
#define DCD_SETUP_PACKET_SIZE				8

#define DCD_BUFFER_PAGE_ALIGNMENT			4096
#define DCD_BUFFER_PAGE_SIZE				4096

/******************************************************************************
 * Endpoint Transfer Descriptor (dTD)
 ******************************************************************************/
#define DCD_DTD_ALIGNMENT					32
#define DCD_DTD_TERMINATE					((void *) 0x00000001)
#define DCD_DTD_STATUS_ACTIVE				0x80
#define DCD_DTD_STATUS_HALTED				0x40
#define DCD_DTD_STATUS_DATA_BUFFER_ERROR	0x20
#define DCD_DTD_STATUS_TRANSACTION_ERROR	0x08
#define DCD_DTD_STATUS_ERROR_MASK			(DCD_DTD_STATUS_HALTED | DCD_DTD_STATUS_DATA_BUFFER_ERROR | DCD_DTD_STATUS_TRANSACTION_ERROR)
#define DCD_DTD_MAX_PAGE_COUNT				5

typedef union {
	uint32_t all;
	struct {
		unsigned status : 8;
		unsigned reserved0 : 2;
		unsigned multO : 2;
		unsigned reserved1 : 3;
		unsigned ioc : 1;
		unsigned totalBytes : 15;
		unsigned reserved2 : 1;
	} bit __attribute__ ((packed));
} DcdEndpointTransferDescriptorToken;

typedef struct {
	void *nextTD;
	DcdEndpointTransferDescriptorToken token;
	void *bufferPages[DCD_DTD_MAX_PAGE_COUNT];
	void *private;
} __attribute__ ((packed)) DcdEndpointTransferDescriptor;

/******************************************************************************
 * Endpoint Queue Head (dQH)
 ******************************************************************************/
#define DCD_DQH_LIST_ALIGNMENT				2048
#define DCD_DQH_DIRECTION_IN				1
#define DCD_DQH_DIRECTION_OUT				0

/* Non standard usage (16Byte reserved area) */
typedef struct {
	void *headTD;
	void *tailTD;
	uint32_t reserved[2];
} __attribute__ ((packed)) DcdEndpointQueueHead_Private_t;

typedef union {
	uint32_t all;
	struct {
		unsigned reserved0 : 15;
		unsigned ios : 1;
		unsigned maxPacketSize : 11;
		unsigned reserved1 : 2;
		unsigned zlt : 1;
		unsigned mult : 2;
	} bit __attribute__ ((packed));
} DcdEndpointQueueHeadCapabilities;
 
typedef struct {
	DcdEndpointQueueHeadCapabilities capabilities;
	void *currentTD;
	DcdEndpointTransferDescriptor overlay;
	uint8_t setupBuffer[DCD_SETUP_PACKET_SIZE];
	DcdEndpointQueueHead_Private_t private;
} __attribute__ ((packed)) DcdEndpointQueueHead;

/******************************************************************************
 * Device Controller Driver API
 ******************************************************************************/
typedef enum {
	DcdEventTypeReset,
	DcdEventTypeSetup,
	DcdEventTypeTransactionComplete
} DcdEventType;

typedef enum {
	DcdEndpointTypeControl = 0,
	DcdEndpointTypeIsochronus = 1,
	DcdEndpointTypeBulk = 2,
	DcdEndpointTypeInterrupt = 3,
	DcdEndpointTypeNone = 4
} DcdEndpointType;

typedef void (*DcdEventCallback)	(DcdEventType eventType, int epAddr);

void DcdInit(DcdEventCallback callback);
void DcdSetDeviceAddress(uint16_t address);
DcdEndpointTransferDescriptor *DcdNewTransferDescriptor(int totalBytes, bool ioc);
void DcdDisposeTransferDescriptor(DcdEndpointTransferDescriptor *dtd);
/*void DcdInitQueueHead(int epAddr, int maxPacketSize, bool ios);*/
void DcdInsertTransferDescriptor(int epAddr, DcdEndpointTransferDescriptor *dtd);
DcdEndpointTransferDescriptor *DcdRemoveCompletedTransferDescriptor(int epAddr);
void DcdReadSetupBuffer(int epAddr, void *buffer, int off);
void DcdInitEndpoint(int epAddr, DcdEndpointType type, int maxPacketSize);
void DcdStallEndpoint(int epAddr);
void DcdISR();
void DcdSetTransferDescriptorBuffer(DcdEndpointTransferDescriptor *dtd, void *buffer);
#define DcdGetTransferStatus(dtd)		(dtd->token.bit.status)
