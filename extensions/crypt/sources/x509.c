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
#include "common.h"
#include "fips180.h"

static int
_getBerLen(unsigned char **pp, unsigned char *endp)
{
	unsigned char *p = *pp;
	int l;

	if (p >= endp)
		return -1;
	if ((l = *p++) > 0x80) {
		int n = l - 0x80;
		for (l = 0; --n >= 0;) {
			if (p >= endp)
				return -1;
			l = (l << 8) | *p++;
		}
	}
	*pp = p;
	return l;
}

void
xs_x509_decodeSubjectKeyId(xsMachine *the)
{
	UInt32 sz = xsToInteger(xsGet(xsArg(0), xsID("length")));
	unsigned char *p = xsGetHostData(xsArg(0)), *endp = p + sz, *endTBS, *endEXT, *extnID, *spki, *spk;
	int l, extnIDLen, spkLen;
	static UInt8 id_ce_ski[] = {2 * 40 + 5, 29, 14};	/* [2, 5, 29, 14] */

#define getTag()	(p < endp ? (int)*p++ : -1)
#define getBerLen()	_getBerLen(&p, endp)
#define nextTag()	(getTag(), l = getBerLen(), p += l)

	if (getTag() != 0x30)
		return;
	if ((l = getBerLen()) < 0)
		return;
	if (p + l > endp)
		return;
	/* TBSCertficate */
	if (getTag() != 0x30)
		return;
	if ((l = getBerLen()) < 0)
		return;
	if ((endTBS = p + l) > endp)
		return;
	if (*p & 0x80) {
		/* EXPLICT Version */
		p++;
		nextTag();
	}
	nextTag();	/* serialNumber */
	nextTag();	/* signature */
	nextTag();	/* issuer */
	nextTag();	/* validity */
	nextTag();	/* subject */
	spki = p;	/* subjectPublicKeyInfo */
	nextTag();
	/* OPTIONAL */
	while (p < endTBS) {
		int tag = getTag();
		if ((l = getBerLen()) < 0)
			return;
		switch (tag & 0x1f) {
		case 1:	/* issuerUniqueID */
		case 2:	/* subjectUniqueID */
			p += l;
			continue;	/* goto the next tag */
		case 3:	/* extensions */
			break;	/* fall thru */
		default:
			return;
		}
		/* must be a SEQUENCE of [1..MAX] */
		if (getTag() != 0x30)
			return;
		if ((l = getBerLen()) < 0)
			return;
		endEXT = p + l;
		while (p < endEXT) {
			/* must be a SEQUENCE of {extnID, critical, extnValue} */
			if (getTag() != 0x30)
				return;
			if ((l = getBerLen()) < 0)
				return;
			/* extnID: OBJECT ID */
			if (getTag() != 0x06)
				return;
			if ((extnIDLen = getBerLen()) < 0)
				return;
			extnID = p;
			p += extnIDLen;
			/* critical: BOOLEAN */
			if (*p == 0x01)
				nextTag();
			/* extnValue: OCTET STRING */
			if (getTag() != 0x04)
				return;
			if ((l = getBerLen()) < 0)
				return;
			if (extnIDLen == sizeof(id_ce_ski) && FskMemCompare(extnID, id_ce_ski, extnIDLen) == 0) {
				/* SKI: OCTET STRING */
				if (getTag() != 0x04)
					return;
				l = getBerLen();
				xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(l));
				FskMemCopy(xsGetHostData(xsResult), p, l);
				return;
			}
			p += l;
		}
	}
	{
		/*
		 * Couldn't find Subject Key Identifier. Make up the ID from the Subject Public Key
		 */
		struct sha1 sha1;

		p = spki;
		if (getTag() != 0x30)
			return;	/* should be a SEQUENCE */
		l = getBerLen();
		/* skip AlgorithmIdentifier */
		nextTag();
		if (getTag() != 0x03)
			return;	/* should be a BIT STRING */
		spkLen = getBerLen();
		spk = p;
		xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(SHA1_DGSTSIZE));
		sha1_create(&sha1);
		sha1_update(&sha1, spk, spkLen);
		sha1_fin(&sha1, xsGetHostData(xsResult));
	}
}
