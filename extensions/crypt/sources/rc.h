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
#include "cryptTypes.h"

typedef struct rc4_state {
	UInt8 i, j;
	UInt8 state[256];
} rc4_state;

extern void rc4_init(rc4_state *rc4, const UInt8 *key, int keySize);
extern void rc4_process(UInt8 *bp, int bufsiz, rc4_state *rc4);
