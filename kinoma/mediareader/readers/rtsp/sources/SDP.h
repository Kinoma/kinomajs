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

#ifndef __SDP__
#define __SDP__

#include "Fsk.h"
#include "FskUtilities.h"
#include "FskList.h"

typedef struct SDPAttributeRecord{
	struct SDPAttributeRecord *next;

	char *attribute;
	char *value;		// only specified in value attributes, i.e. <attribute>:<value>
} SDPAttributeRecord, *SDPAttribute;

typedef struct SDPAttributeListRecord {
	FskList head;
} SDPAttributeListRecord, *SDPAttributeList;

typedef struct SDPMediaFormatRecord {
	struct SDPMediaFormatRecord *next;

	UInt32 payloadType;
} SDPMediaFormatRecord, *SDPMediaFormat;

typedef struct SDPMediaFormatListRecord {
	FskList head;
} SDPMediaFormatListRecord, *SDPMediaFormatList;

typedef struct {
	UInt32 address;
	char *addressStr;
	UInt32 ttl;
	UInt32 nAddresses;
} SDPConnectionRecord, *SDPConnection;

typedef struct SDPMediaDescriptionRecord {
	struct SDPMediaDescriptionRecord *next;

	char *mediaName;		// "audio", "video", "control", etc...
	UInt32 port;
	UInt32 nPorts;
	SDPConnection connection;
	char *transport;		// "RTP/AVP", "udp"
	SDPMediaFormatList formatList;
	SDPAttributeList mediaAttributes;
} SDPMediaDescriptionRecord, *SDPMediaDescription;

typedef struct SDPMediaDescriptionListRecord {
	FskList head;
} SDPMediaDescriptionListRecord, *SDPMediaDescriptionList;

typedef struct {
	char *username;
	char *sessionID;
	char *version;
	UInt32 address;
	char *addressStr;
} SDPOriginRecord, *SDPOrigin;

// NTP (network time protocol) time values in seconds
// Subtract 2208988800 to convert to Unix time
typedef struct {
	UInt32 start;
	UInt32 stop;
} SDPTimeDescriptionRecord, *SDPTimeDescription;

typedef struct {
	UInt32 version;
	char *name;
	char *information;
	SDPOriginRecord origin;
	SDPConnectionRecord connection;
	SDPTimeDescriptionRecord time;
	SDPAttributeList sessionAttributes;
	SDPMediaDescriptionList mediaDescriptions;
} SDPSessionDescriptionRecord, *SDPSessionDescription;

typedef struct {
	unsigned char *data;
	int position;
} bitStreamRecord, *bitStream;

#ifdef __cplusplus
extern "C" {
#endif

FskErr SDPSessionDescriptionNewFromMemory(UInt8 *buffer, UInt32 bufferLen, SDPSessionDescription *desc);
FskErr SDPSessionDescriptionDispose(SDPSessionDescription desc);

SDPAttribute SDPFindSessionAttribute(SDPSessionDescription desc, char *attributeName);
SDPAttribute SDPFindMediaAttribute(SDPMediaDescription desc, char *attributeName);
SDPAttribute SDPFindMediaAttributeWithException(SDPMediaDescription desc, char *attributeName, char *exceptVal);

SDPMediaDescription SDPFindMediaDescription(SDPSessionDescription desc, char *mediaName);

void splitToken(char *token, UInt16 *nParts, char splitChar, char **parts);
char *copyAttributeValue(char *hdr, char *name, char **value);
unsigned long getBits(bitStream bits, int count);

#ifdef __cplusplus
}
#endif

#endif
