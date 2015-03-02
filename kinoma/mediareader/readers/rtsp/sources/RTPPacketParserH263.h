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

#ifndef _RTPH263_H_
#define _RTPH263_H_

#include "RTPPacketParser.h"

static const short h263_format[8][2] = {
    { 0, 0 },
    { 128, 96 },
    { 176, 144 },
    { 352, 288 },
    { 704, 576 },
    { 1408, 1152 },
    { 320, 240 }
};

typedef struct H263BitBuffer
{
	unsigned char* buffer;
	UInt32 bufptr;
	UInt32 bitcnt;
	unsigned char buf;
}H263BitBuffer;

typedef enum {
	MDP_H263_MODE_1 = 1,
	MDP_H263_MODE_2,
	MDP_H263_MODE_3
}MDP_H263_MODES;


typedef struct 
{
	UInt32 mode;
	char P;
	char V;
	char PEBIT;
	unsigned char PLEN;
} RTPPayloadH263;

#ifdef __cplusplus
	extern "C" {
#endif

FskErr RTPPacketProcessH263(RTPPacket rtpHeader);

#ifdef __cplusplus
		}
#endif


#endif
