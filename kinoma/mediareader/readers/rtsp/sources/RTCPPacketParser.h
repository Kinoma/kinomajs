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
//	RFC 1889

#ifndef _RTCP_PACKET_PARSER_H
#define _RTCP_PACKET_PARSER_H

#include "Fsk.h"

// RTCP packet types - PT
enum {
	kRTCPPacketTypeSenderReport = 200,
	kRTCPPacketTypeReceiverReport,
	kRTCPPacketTypeSDES,
	kRTCPPacketTypeBYE,
	kRTCPPacketTypeAPP
};

// Header for all RTCP packet types - 6.3.1
typedef struct {
	UInt8 version;
	UInt8 padding;
	UInt8 count;
	UInt8 PT;
	UInt16 length;
} RTCPHeaderRecord, *RTCPHeader;

// Report blocks - 6.3.1
typedef struct {
	UInt32 SSRC;
	UInt8 fractionLost;
	UInt32 cummulativePacketsLost;
	UInt16 highestSequenceNumReceived;
	UInt16 sequenceNumCycles;
	UInt32 interarrivalJitter;
	UInt32 LSR;
	UInt32 DLSR;
} RTCPReportBlockRecord, *RTCPReportBlock;

// Sender report
typedef struct {
	UInt32 SSRC;
	FskInt64 ntpTimestamp;
	UInt32 rtpTimestamp;
	UInt32 senderPacketCount;
	UInt32 senderOctetCount;

	RTCPReportBlock reportBlocks;		// Structure extended to contain 'n' report blocks
} RTCPSenderReportRecord, *RTCPSenderReport;

// Receiver report
typedef struct {
	UInt32 SSRC;

	RTCPReportBlock reportBlocks;		// Structure extended to contain 'n' report blocks
} RTCPReceiverReportRecord, *RTCPReceiverReport;

// Source description
typedef struct {
	UInt32 SSRC;

	char CNAME[256];	// canonical end-point identifier
	char NOTE[256];		// used by Opticodec to pass track title and artist
} RTCPSourceDescriptionRecord, *RTCPSourceDescription;

// Goodbye
typedef struct {
	UInt32 SSRC[1];		// Structure extended to contain 'n' source counts
} RTCPGoodbyeRecord, *RTCPGoodbye;

// App defined
typedef struct {
	UInt32 foo;
} RTCPAppDefinedRecord, *RTCPAppDefined;

typedef union RTCPPacketUnion {
	RTCPSenderReportRecord sr;
	RTCPReceiverReportRecord rr;
	RTCPSourceDescriptionRecord sd;
	RTCPGoodbyeRecord bye;
	RTCPAppDefinedRecord app;
} RTCPPacketUnion;

typedef struct {
	RTCPHeaderRecord header;

	// Packet specific
	RTCPPacketUnion flavor;
} RTCPPacketRecord, *RTCPPacket;

typedef FskErr (*RTCPPacketParserReceivePacketCallback)(RTCPPacket packet, void *refCon);

typedef struct {
	void *appRefCon;
	void *sessionRefCon;

	RTCPPacketParserReceivePacketCallback appReceivePacketCB;
	RTCPPacketParserReceivePacketCallback sessionReceivePacketCB;
} RTCPPacketParserRecord, *RTCPPacketParser;

#ifdef __cplusplus
extern "C" {
#endif

FskErr RTCPPacketParserNew(RTCPPacketParser *parser);
FskErr RTCPPacketParserDispose(RTCPPacketParser parser);

FskErr RTCPPacketParserSetSessionRefCon(RTCPPacketParser parser, void *refCon);
FskErr RTCPPacketParserSetSessionReceivePacketCallback(RTCPPacketParser parser, RTCPPacketParserReceivePacketCallback callback);
FskErr RTCPPacketParserSetAppRefCon(RTCPPacketParser parser, void *refCon);
FskErr RTCPPacketParserSetAppReceivePacketCallback(RTCPPacketParser parser, RTCPPacketParserReceivePacketCallback callback);

FskErr RTCPPacketParserProcessBuffer(RTCPPacketParser parser, unsigned char *buffer, UInt32 bufferSize);

FskErr RTCPPacketDispose(RTCPPacketParser parser, RTCPPacket packet);

#ifdef __cplusplus
}
#endif

#endif
