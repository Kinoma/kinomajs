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
#ifndef __KPRWEBSOCKETCOMMON__
#define __KPRWEBSOCKETCOMMON__

#include "xs.h"

//--------------------------------------------------
// WebSocket Common
//--------------------------------------------------

enum {
	kKprWebSocketStateConnecting = 0,
	kKprWebSocketStateOpen = 1,
	kKprWebSocketStateClosing = 2,
	kKprWebSocketStateClosed = 3,
};

enum {
	kKprWebSocketOpcodeContinuationFrame = 0x00,
	kKprWebSocketOpcodeTextFrame = 0x01,
	kKprWebSocketOpcodeBinaryFrame = 0x02,
	kKprWebSocketOpcodeClose = 0x08,
	kKprWebSocketOpcodePing = 0x09,
	kKprWebSocketOpcodePong = 0x0a,
};

#define WebSocketAcceptKeyUUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

void KprWebSocketMask(UInt8 *p0, UInt32 length, UInt8 maskData[4]);
FskErr KprWebSocketCreateKey(char **key);
FskErr KprWebSocketCalculateHash(char *key, char **hash);

#endif
