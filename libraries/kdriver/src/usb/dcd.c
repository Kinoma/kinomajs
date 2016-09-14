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
 * NOTE: No ISO support, Not thread-safe.
 * Thanks to http://atose.org/?page_id=258 for reference
 */

#include "88mw300.h"

#include "dcd.h"
#include "rsrc.h"

#include <stdint.h>
#include <string.h>

/* Callback */
static DcdEventCallback eventCallback;

/* Pre allocated QueueHeads */
static DcdEndpointQueueHead queueHeadList[DCD_MAX_ENDPOINT * 2] __attribute__ ((aligned (DCD_DQH_LIST_ALIGNMENT)));
#define GetQueueHead(epAddr) &queueHeadList[ToEPNumber(epAddr) * 2 + ToEPDirection(epAddr)]

/* dTD Resource Definition */
DcdDefineResourcePoolAligned(DTD, DCD_POOL_SIZE_DTD, sizeof(DcdEndpointTransferDescriptor), DCD_DTD_ALIGNMENT);

#define ToEPNumber(epAddr)				(epAddr & DCD_ENDPOINT_NUMBER)
#define ToEPDirection(epAddr)			((epAddr & DCD_ENDPOINT_DIRECTION) >> 7)
#define GetEndpointMask(epAddr)			(uint32_t) (1 << (ToEPNumber(epAddr) + ToEPDirection(epAddr) * 16))

DcdEndpointTransferDescriptor *DcdNewTransferDescriptor(int totalBytes, bool ioc)
{
	DcdEndpointTransferDescriptor *dtd;

	dtd = (DcdEndpointTransferDescriptor *) DcdAllocResource(DcdGetResourcePool(DTD));
	if (dtd != NULL) {
		memset(dtd, 0, sizeof(DcdEndpointTransferDescriptor));
		dtd->nextTD = DCD_DTD_TERMINATE;
		dtd->token.bit.totalBytes = totalBytes;
		dtd->token.bit.ioc = ioc ? 0x01 : 0x00;
		dtd->token.bit.status = DCD_DTD_STATUS_ACTIVE;
	}

	return dtd;
}

void DcdSetTransferDescriptorBuffer(DcdEndpointTransferDescriptor *dtd, void *buffer)
{
	int i;
	for (i = 0; i < DCD_DTD_MAX_PAGE_COUNT; i++) {
		dtd->bufferPages[i] = ((uint8_t *) buffer) + (i * DCD_BUFFER_PAGE_SIZE);
	}
}

void DcdDisposeTransferDescriptor(DcdEndpointTransferDescriptor *dtd)
{
	int i;

	dtd->nextTD = DCD_DTD_TERMINATE;
	dtd->token.bit.status = 0;
	/* Return back to address pool */
	DcdDisposeResource(DcdGetResourcePool(DTD), dtd);
}

void DcdInitQueueHead(int epAddr, int maxPacketSize, bool ios)
{
	DcdEndpointQueueHead *dqh;

	dqh = GetQueueHead(epAddr);
	memset(dqh, 0, sizeof(DcdEndpointQueueHead));
	dqh->capabilities.bit.maxPacketSize = maxPacketSize;
	dqh->capabilities.bit.ios = ios ? 0x01 : 0x00;
	dqh->overlay.nextTD = DCD_DTD_TERMINATE;
	dqh->private.headTD = NULL;
	dqh->private.tailTD = NULL;
}

void DcdInsertTransferDescriptor(int epAddr, DcdEndpointTransferDescriptor *dtd)
{
	uint32_t temp, mask;
	bool emptyList = false;
	DcdEndpointQueueHead *dqh;
	DcdEndpointTransferDescriptor *tailTD;

	mask = GetEndpointMask(epAddr);
	dqh = GetQueueHead(epAddr);

	/* Update Head */
	if (dqh->private.headTD == NULL) {
		dqh->private.headTD = dtd;
		emptyList = true;
	}
	/* Update Tail */
	tailTD = dqh->private.tailTD;
	dqh->private.tailTD = dtd;	/* FIXME: dTD may have multiple link */
	if (tailTD != NULL) {
		tailTD->nextTD = dtd;
	}

	if (!emptyList) {
		/* Case 2 - List is NOT empty */
		if (MW300_USBC->ENDPTPRIME & mask) {
			/* EP has already been primed */
			return;
		}
		/* dTD TripWire */
		do {
			MW300_USBC->USBCMD |= USBC_USBCMD_ATDTW;
			temp = MW300_USBC->ENDPTSTAT;
		} while (!(MW300_USBC->USBCMD & USBC_USBCMD_ATDTW));
		MW300_USBC->USBCMD &= ~USBC_USBCMD_ATDTW;	/* Clear TripWire */

		if (temp & mask) {
			return;
		}
	}

	dqh->overlay.nextTD = dtd;
	dqh->overlay.token.all = 0;

	temp = MW300_USBC->ENDPTPRIME;
	MW300_USBC->ENDPTPRIME = temp | mask;
#if 0
	while (MW300_USBC->ENDPTPRIME & mask)
		;
#endif
}

DcdEndpointTransferDescriptor *DcdRemoveCompletedTransferDescriptor(int epAddr)
{
	DcdEndpointQueueHead *dqh;
	DcdEndpointTransferDescriptor *dtd;
	DcdEndpointTransferDescriptor *completed;

	dqh = GetQueueHead(epAddr);
	dtd = dqh->private.headTD;
	if (dtd == NULL) {
		return NULL;
	}
	if (dtd->token.bit.status & DCD_DTD_STATUS_ACTIVE) {
		/* Still active */
		return NULL;
	}

	completed = dtd;

	/* Update head */
	dqh->private.headTD = dtd->nextTD;
	if (dqh->private.headTD == DCD_DTD_TERMINATE) {
		dqh->private.headTD = NULL;
	}
	dtd = dqh->private.headTD;
	/* Update tail */
	if (dtd == NULL) {
		dqh->private.tailTD = NULL;
	}

	return completed;
}

void DcdReadSetupBuffer(int epAddr, void *buffer, int off)
{
	DcdEndpointQueueHead *dqh;

	dqh = GetQueueHead(epAddr & DCD_ENDPOINT_NUMBER);
	/* Setup TripWire */
	do {
		MW300_USBC->USBCMD |= USBC_USBCMD_SUTW;
		memcpy(buffer + off, dqh->setupBuffer, DCD_SETUP_PACKET_SIZE);
	} while (!(MW300_USBC->USBCMD & USBC_USBCMD_SUTW));
	MW300_USBC->USBCMD &= ~USBC_USBCMD_SUTW;	/* Clear TripWire */
}

void DcdInit(DcdEventCallback callback)
{
	eventCallback = callback;

	MW300_USBC->USBMODE |=
		USBC_USBMODE_CM(2) |	/* DEVICE MODE */
		USBC_USBMODE_SLOM;		/* Turn on the trip-wire mechanism for Setup packets */

	/* Init dTD & dQH */
	DcdInitResourcePool(DcdGetResourcePool(DTD));
	MW300_USBC->ENDPTLISTADDR = (uint32_t) queueHeadList;

	/* Init EP0 */
	DcdInitQueueHead(0x00, 64, true);
	DcdInitQueueHead(0x80, 64, true);

	/* Clear all the status bits */
	MW300_USBC->USBSTS =
		USBC_USBSTS_URI |		/* USB USB Reset Received */
		USBC_USBSTS_PCI |		/* USB Port Change Detect */
		USBC_USBSTS_UI;			/* USB Interrupt (USBINT) */

	/* Enable interrupts */
	MW300_USBC->USBINTR =
		USBC_USBINTR_URE |		/* USB Reset Enable */
		USBC_USBINTR_PCE |		/* USB Port Change Detect */
		USBC_USBINTR_UE;		/* USB Interrupt Enable. */

	/* Speed setting */
//	USBC->PORTSC1.BF.PSPD = 2;

	/* Run */
	MW300_USBC->USBCMD |= USBC_USBCMD_RS;
}

void DcdSetDeviceAddress(uint16_t address)
{
	MW300_USBC->DEVICEADDR = ((address & 0x7F) << 25) | 0x01000000;
}
		
void DcdInitEndpoint(int epAddr, DcdEndpointType type, int maxPacketSize)
{
	uint8_t ep = ToEPNumber(epAddr);
	uint32_t temp, mask;

	if (ep == 0) {
		/* Endpoint 0 is invalid */
		return;
	}

	mask = 0xC0 | ((uint8_t) type << 2);
	if (type == DcdEndpointTypeControl) {
		DcdInitQueueHead(epAddr & ~DCD_ENDPOINT_DIRECTION, maxPacketSize, true);
		DcdInitQueueHead(epAddr | DCD_ENDPOINT_DIRECTION, maxPacketSize, true);
		mask = mask | (mask << 16);
	} else {
		DcdInitQueueHead(epAddr, maxPacketSize, false);
		mask <<= ToEPDirection(epAddr) * 16;
	}

	temp = MW300_USBC->ENDPTCTRL[ep];
	temp |= mask;
	MW300_USBC->ENDPTCTRL[ep] = temp;
}

void DcdStallEndpoint(int epAddr)
{
	uint8_t ep = ToEPNumber(epAddr);
	uint32_t temp, mask;

	if (ep == 0 /*|| type == DcdEndpointTypeControl*/) {
		mask = 0x00010001;
	} else {
		mask = 1 << (ToEPDirection(epAddr) * 16);
	}

	temp = MW300_USBC->ENDPTCTRL[ep];
	temp |= mask;
	MW300_USBC->ENDPTCTRL[ep] = temp;
}

/******************************************************************************
 * ISR
 ******************************************************************************/

void DcdISR()
{
	uint32_t usbStatus;

	usbStatus = MW300_USBC->USBSTS;
	MW300_USBC->USBSTS = usbStatus;	/* Clear interrupts */

	if (usbStatus & USBC_USBSTS_UI) {
		int endpoint, direction;
		uint32_t endpointSetupStatus = MW300_USBC->ENDPTSETUPSTAT & 0x0000FFFF;
		uint32_t endpointComplete = MW300_USBC->ENDPTCOMPLETE & 0x00FF00FF;
		uint32_t endpointStatus = MW300_USBC->ENDPTSTAT;

		/* Clear */
		MW300_USBC->ENDPTSETUPSTAT = endpointSetupStatus;
		MW300_USBC->ENDPTCOMPLETE = endpointComplete;

		/* Scan QueueHeads */
		for (endpoint = 0; endpoint < DCD_MAX_ENDPOINT; endpoint++) {
			for (direction = 0; direction < 2; direction++) {
				uint8_t epAddr = (endpoint | (DCD_ENDPOINT_DIRECTION * direction));
				if (endpointComplete & GetEndpointMask(epAddr)) {
					eventCallback(DcdEventTypeTransactionComplete, epAddr);
				}
			}
			if (endpointSetupStatus & (1 << endpoint)) {
				eventCallback(DcdEventTypeSetup, endpoint);
			}
		}
	}

	if (usbStatus & USBC_USBSTS_URI) {
		uint32_t endpointSetupStatus;
		uint32_t endpointComplete;

		MW300_USBC->DEVICEADDR = ~0xFE000000;
		/* Clear all setup token semaphores */
		endpointSetupStatus = MW300_USBC->ENDPTSETUPSTAT;
		MW300_USBC->ENDPTSETUPSTAT = endpointSetupStatus;
		/* Clear all the endpoint complete status bits */
		endpointComplete = MW300_USBC->ENDPTCOMPLETE;
		MW300_USBC->ENDPTCOMPLETE = endpointComplete;
		/* Clear all primed status */
		while (MW300_USBC->ENDPTPRIME != 0);
		/* Flush */
		MW300_USBC->ENDPTFLUSH = 0xFFFFFFFF;
		/* Make sure port is in reset */
		if (MW300_USBC->PORTSC1 & USBC_PORTSC1_PR) {
			/* TODO: Need hard reset */
		}
		/* TODO: Free All dTD */
		eventCallback(DcdEventTypeReset, 0);
	}

	if (usbStatus & USBC_USBSTS_PCI) {
		/* TODO: Notify as an event */
	}
}
