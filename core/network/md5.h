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
#ifndef __FSK_MD5__
#define __FSK_MD5__

#include "Fsk.h"

#ifndef ulong64
#define ulong64 unsigned long long	// FskUInt64
#endif
#ifndef ulong32
#define ulong32 UInt32
#endif

typedef struct md5_hash_state {
	struct md5_state {
		ulong64 length;
		ulong32 state[4], curlen;
		unsigned char buf[64];
	} md5;
} md5_hash_state;

#ifdef __cplusplus
extern "C" {
#endif

FskAPI(void) md5_init(md5_hash_state * md);
FskAPI(void) md5_process (md5_hash_state *md, const unsigned char *in, unsigned long inlen);
FskAPI(void) md5_done(md5_hash_state *md, unsigned char *out);

#ifdef __cplusplus
}
#endif

#endif
