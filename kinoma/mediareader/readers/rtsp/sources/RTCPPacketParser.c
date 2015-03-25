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

/*
	RFC 1889
*/

#include "RTCPPacketParser.h"
#include "FskUtilities.h"
#include "FskEndian.h"

static FskErr doSenderReport(RTCPPacketParser parser, RTCPPacket *packet, unsigned char *buffer, UInt32 bufferSize);
static FskErr doReceiverReport(RTCPPacketParser parser, RTCPPacket *packet, unsigned char *buffer, UInt32 bufferSize);
static FskErr doSDES(RTCPPacketParser parser, RTCPPacket *packet, unsigned char *buffer, UInt32 bufferSize);
static FskErr doBYE(RTCPPacketParser parser, RTCPPacket *packet, unsigned char *buffer, UInt32 bufferSize);

FskErr RTCPPacketParserNew(RTCPPacketParser *parser)
{
	FskErr err = 0;

	err = FskMemPtrNewClear(sizeof(RTCPPacketParserRecord), (FskMemPtr*)parser);

	return err;
}

FskErr RTCPPacketParserDispose(RTCPPacketParser parser)
{
	FskErr err = 0;

	FskMemPtrDispose(parser);

	return err;
}

FskErr RTCPPacketParserSetSessionRefCon(RTCPPacketParser parser, void *refCon)
{
	FskErr err = 0;

	parser->sessionRefCon = refCon;

	return err;
}

FskErr RTCPPacketParserSetSessionReceivePacketCallback(RTCPPacketParser parser, RTCPPacketParserReceivePacketCallback callback)
{
	FskErr err = 0;

	parser->sessionReceivePacketCB = callback;

	return err;
}

FskErr RTCPPacketParserSetAppRefCon(RTCPPacketParser parser, void *refCon)
{
	FskErr err = 0;

	parser->appRefCon = refCon;

	return err;
}

FskErr RTCPPacketParserSetAppReceivePacketCallback(RTCPPacketParser parser, RTCPPacketParserReceivePacketCallback callback)
{
	FskErr err = 0;

	if (NULL == parser) goto bail;

	parser->appReceivePacketCB = callback;

bail:
	return err;
}

FskErr RTCPPacketParserProcessBuffer(RTCPPacketParser parser, unsigned char *buffer, UInt32 bufferSize)
{
	FskErr err = 0;
	RTCPPacket packet = 0;
	UInt8 *p = buffer;
	UInt16 packetLength;
	RTCPHeaderRecord header;

	do {
		// Populate the header
		header.version = (p[0] >> 6) & 0x3;
		header.padding = (p[0] >> 5) & 0x1;
		header.count = p[0] & 0x1F;
		header.PT = p[1];
		header.length = FskEndianU16_BtoN(*(UInt16*)&p[2]);

		if (header.version != 2) {
			err = kFskErrRTSPBadPacket;
			goto bail;
		}

		switch(header.PT) {
			case kRTCPPacketTypeSenderReport:
				err = doSenderReport(parser, &packet, p, bufferSize);
				break;
			case kRTCPPacketTypeReceiverReport:
				err = doReceiverReport(parser, &packet, p, bufferSize);
				break;
			case kRTCPPacketTypeSDES:
				err = doSDES(parser, &packet, p, bufferSize);
				break;
			case kRTCPPacketTypeBYE:
				err = doBYE(parser, &packet, p, bufferSize);
				break;

			case kRTCPPacketTypeAPP:
				err = 0;
				break;

			default:
				err = kFskErrRTSPBadPacket;
				goto bail;
		}
		if (0 != err) goto bail;

		if (0 != packet) {
			packet->header = header;
			
			// Call the session callback function
			if (parser->sessionReceivePacketCB) {
				err = parser->sessionReceivePacketCB(packet, parser->sessionRefCon);
				BAIL_IF_ERR(err);
			}

			// Call the application callback function
			if (parser->appReceivePacketCB)
				parser->appReceivePacketCB(packet, parser->appRefCon);
			else
				RTCPPacketDispose(parser, packet);
		}

		// Advance to next packet, assuming this is a compound packet
		packet = 0;
		packetLength = (header.length + 1) << 2;
		bufferSize -= packetLength;
		p += packetLength;

	} while (bufferSize > 4);	// size of the header

bail:
	RTCPPacketDispose(parser, packet);
	FskMemPtrDispose(buffer);

	return err;
}

FskErr RTCPPacketDispose(RTCPPacketParser parser, RTCPPacket packet)
{
	FskErr err = 0;

	FskMemPtrDispose(packet);

	return err;
}

FskErr doSenderReport(RTCPPacketParser parser, RTCPPacket *packet, unsigned char *buffer, UInt32 bufferSize)
{
	FskErr err = kFskErrNone;
	UInt8 rrCount, *p = buffer;
	UInt16 i;

	// Calculate the number of sender reports
	rrCount = p[0] & 0x1F;

	// Allocate the packet
	err = FskMemPtrNew(sizeof(RTCPPacketRecord) + (rrCount * sizeof(RTCPReportBlockRecord)), (FskMemPtr*)packet);
	if (0 != err) goto bail;

	if (rrCount == 0)
		(*packet)->flavor.sr.reportBlocks = NULL;
	else
		(*packet)->flavor.sr.reportBlocks = (RTCPReportBlock)((*packet) + 1);  // @@ untested, this is rare

	// Skip past the header
	p += 4;

	// read the sender SSRC
	(*packet)->flavor.sr.SSRC = FskEndianU32_BtoN(*(UInt32*)p); p += 4;

	// read the sender info
	(*packet)->flavor.sr.ntpTimestamp = ((FskInt64)FskEndianU32_BtoN(*(UInt32*)p) << 32) | (FskInt64)FskEndianU32_BtoN(*(UInt32*)(p + 4)); p += 8;
	(*packet)->flavor.sr.rtpTimestamp = FskEndianU32_BtoN(*(UInt32*)p); p += 4;
	(*packet)->flavor.sr.senderPacketCount = FskEndianU32_BtoN(*(UInt32*)p); p += 4;
	(*packet)->flavor.sr.senderOctetCount = FskEndianU32_BtoN(*(UInt32*)p); p += 4;

	// read the report blocks
	for (i = 0; i < rrCount; ++i) {
		RTCPReportBlock reportBlock = &(*packet)->flavor.sr.reportBlocks[i];
		reportBlock->SSRC = FskEndianU32_BtoN(*(UInt32*)p); p += 4;
		reportBlock->fractionLost = *p;
		reportBlock->cummulativePacketsLost = FskEndianU32_BtoN(*(UInt32*)p) & 0xFFF; p += 4;
		reportBlock->sequenceNumCycles = FskEndianU16_BtoN(*(UInt16*)p); p += 2;
		reportBlock->highestSequenceNumReceived = FskEndianU16_BtoN(*(UInt16*)p); p += 2;
		reportBlock->interarrivalJitter = FskEndianU32_BtoN(*(UInt32*)p); p += 4;
		reportBlock->LSR = FskEndianU32_BtoN(*(UInt32*)p); p += 4;
		reportBlock->DLSR = FskEndianU32_BtoN(*(UInt32*)p); p += 4;
	}

bail:
	return err;
}

FskErr doReceiverReport(RTCPPacketParser parser, RTCPPacket *packet, unsigned char *buffer, UInt32 bufferSize)
{
	FskErr err = 0;

	return err;
}

FskErr doSDES(RTCPPacketParser parser, RTCPPacket *packet, unsigned char *buffer, UInt32 bufferSize)
{
	FskErr err = 0;
	UInt8 SC, *chunkEnd, *p = buffer;
	UInt16 i;

	// Calculate the number of source chunks
	SC = p[0] & 0x1F;

	// Allocate the packet
	err = FskMemPtrNew(sizeof(RTCPPacketRecord), (FskMemPtr*)packet);
	if (0 != err) goto bail;

	// Skip past the header
	p += 4;

	// read the SSRC
	(*packet)->flavor.sd.SSRC = FskEndianU32_BtoN(*(UInt32*)p);
	p += 4;

	// read the CNAME
	(*packet)->flavor.sd.CNAME[0] = 0;
	(*packet)->flavor.sd.NOTE[0] = 0;
	chunkEnd = buffer + bufferSize;
	for (i = 0; i < SC; ++i) {
		UInt8 length, id;
		while (*p && (p < chunkEnd)) {
			id = *p++;
			length = *p++;
			if (1 == id) {
				FskMemMove((*packet)->flavor.sd.CNAME, p, length);
				(*packet)->flavor.sd.CNAME[length] = 0;
			}
			else
			if (7 == id) {
				FskMemMove((*packet)->flavor.sd.NOTE, p, length);
				(*packet)->flavor.sd.NOTE[length] = 0;
			}
			p += length;
		}
	}

bail:
	return err;
}

FskErr doBYE(RTCPPacketParser parser, RTCPPacket *packet, unsigned char *buffer, UInt32 bufferSize)
{
	FskErr err = 0;
	UInt8 SC, *p = buffer;
	UInt16 i;

	// Calculate the source count
	SC = p[0] & 0x1F;

	// Allocate the packet
	err = FskMemPtrNew(sizeof(RTCPPacketRecord) + (SC * sizeof(RTCPGoodbyeRecord)), (FskMemPtr*)packet);
	if (0 != err) goto bail;

	// Skip past the header
	p += 4;

	for (i = 0; i < SC; ++i) {
		(*packet)->flavor.bye.SSRC[i] = FskEndianU32_BtoN(*(UInt32*)p); p += 4;
	}

bail:
	return err;
}
