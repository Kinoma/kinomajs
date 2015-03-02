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
#include "rc.h"

#if !FSK_NO_RC4
void
rc4_init(rc4_state *rc4, const UInt8 *key, int keySize)
{
	int i;
	UInt8 j, t;
	UInt8 *s;

	for (i = 0, s = rc4->state; i < 256; i++)
		*s++ = i;
	for (i = j = 0, s = rc4->state; i < 256; i++) {
		j += s[i] + key[i % keySize];
		t = s[i];
		s[i] = s[j];
		s[j] = t;
	}
	rc4->i = rc4->j = 0;
}

void
rc4_process(UInt8 *bp, int bufsiz, rc4_state *rc4)
{
	UInt8 *bufend = bp + bufsiz;
	UInt8 i, j, t;
	UInt8 *s = rc4->state;

	while (bp < bufend) {
		i = ++rc4->i;
		j = rc4->j += s[i];
		t = s[i];
		s[i] = s[j];
		s[j] = t;
		t = s[i] + s[j];
		*bp++ ^= s[t];
	}
}
#endif
