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
#if !FSK_NO_MD5
#include "cryptTypes.h"

#define MD5_NUMSTATE	4
#define MD5_DGSTSIZE	(MD5_NUMSTATE * 4)
#define MD5_BLKSIZE	64

struct md5 {
	UInt64 len;
	UInt32 state[MD5_NUMSTATE];
	UInt8 buf[MD5_BLKSIZE];
};

extern void md5_create(struct md5 *s);
extern void md5_update(struct md5 *s, const void *data, UInt32 size);
extern void md5_fin(struct md5 *s, UInt8 *dgst);
#endif /* !FSK_NO_MD5 */
