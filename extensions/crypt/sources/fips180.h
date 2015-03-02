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
#include "cryptTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !FSK_NO_SHA1
#define SHA1_NUMSTATE	5
#define SHA1_DGSTSIZE	(SHA1_NUMSTATE * 4)
#define SHA1_BLKSIZE	64

struct sha1 {
	UInt64 len;
	UInt32 state[SHA1_NUMSTATE];
	UInt8 buf[SHA1_BLKSIZE];
};

extern void sha1_create(struct sha1 *s);
extern void sha1_update(struct sha1 *s, const void *data, UInt32 size);
extern void sha1_fin(struct sha1 *s, UInt8 *dgst);
#endif

#if !FSK_NO_SHA256
#define SHA256_NUMSTATE	8
#define SHA256_DGSTSIZE	(SHA256_NUMSTATE * 4)
#define SHA256_BLKSIZE	64

struct sha256 {
	UInt64 len;
	UInt32 state[SHA256_NUMSTATE];
	UInt8 buf[SHA256_BLKSIZE];
};
extern void sha256_create(struct sha256 *s);
extern void sha256_update(struct sha256 *s, const void *data, UInt32 size);
extern void sha256_fin(struct sha256 *s, UInt8 *dgst);
#endif

#if !FSK_NO_SHA512
#define SHA512_NUMSTATE	8
#define SHA512_DGSTSIZE	(SHA512_NUMSTATE * 8)
#define SHA512_BLKSIZE	128
struct sha512 {
	UInt64 len[2];
	UInt64 state[SHA512_NUMSTATE];
	UInt8 buf[SHA512_BLKSIZE];
};
extern void sha512_create(struct sha512 *s);
extern void sha512_update(struct sha512 *s, const void *data, UInt32 size);
extern void sha512_fin(struct sha512 *s, UInt8 *dgst);
#endif

#ifdef __cplusplus
}
#endif
