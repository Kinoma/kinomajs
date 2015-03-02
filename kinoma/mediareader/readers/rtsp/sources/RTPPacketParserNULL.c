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

#include "FskUtilities.h"
#include "RTPPacketParser.h"

// Private state
typedef struct {
	UInt32 state;
} NULLPacketParserRecord, *NULLPacketParser;

void nullPacketParserInitialize(RTPPacketHandler handler);

static FskErr nullPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName);
static FskErr nullPacketParserNew(RTPPacketParser parser);
static FskErr nullPacketParserDispose(RTPPacketParser parser);
static FskErr nullPacketParserProcessPacket(RTPPacketParser parser, RTPPacket packet);
static FskErr nullPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet);

/* -----------------------------------------------------------------------*/

void nullPacketParserInitialize(RTPPacketHandler handler)
{
	handler->version = 1;

	handler->doCanHandle = nullPacketParserCanHandle;
	handler->doNew = nullPacketParserNew;
	handler->doDispose = nullPacketParserDispose;
	handler->doProcessPacket = nullPacketParserProcessPacket;
	handler->doDisposePacket = nullPacketParserDisposePacket;
}

/* -----------------------------------------------------------------------*/

FskErr nullPacketParserNew(RTPPacketParser parser)
{
	parser->mediaFormat = kRTPMediaFormatUnknown;
	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr nullPacketParserDispose(RTPPacketParser parser)
{
	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr nullPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName)
{
	// I handle everything!
	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr nullPacketParserProcessPacket(RTPPacketParser parser, RTPPacket rtpHeader)
{
	rtpHeader->frames = 0;
	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr nullPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet)
{
	return 0;
}
