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
#include "kprWebSocketCommon.h"
#include "fips180.h"
#include "kpr.h"

void KprWebSocketMask(UInt8 *p0, UInt32 length, UInt8 maskData[4])
{
	UInt8 *p = (UInt8 *) p0;
	UInt8 *end = p + length;
	UInt32 pos = 0;

	while (p < end) {
		UInt8 maskByte = maskData[pos++ % 4];
		*p++ ^= maskByte;
	}
}

FskErr KprWebSocketCreateKey(char **key)
{
	FskErr err = kFskErrNone;
	SInt32 randomData[4];
	int i;

	for (i = 0; i < 4; i++) {
		randomData[i] = FskRandom();
	}

	FskStrB64Encode((char *)randomData, 16, key, NULL, false);
	bailIfNULL(*key);

bail:
	return err;
}

FskErr KprWebSocketCalculateHash(char *key, char **hash)
{
	struct sha1 ctx;
	UInt8 digest[SHA1_DGSTSIZE];

	sha1_create(&ctx);
	sha1_update(&ctx, key, FskStrLen(key));
	sha1_update(&ctx, WebSocketAcceptKeyUUID, FskStrLen(WebSocketAcceptKeyUUID));
	sha1_fin(&ctx, digest);

	FskStrB64Encode((char *)digest, SHA1_DGSTSIZE, hash, NULL, false);
	if (!*hash) return kFskErrMemFull;

	return kFskErrNone;
}

