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
#include "common.h"


static char *
pem_strnnew(xsMachine *the, char *src, int n)
{
	char *dst;
	FskErr err;

	if ((err = FskMemPtrNew(n + 1, (FskMemPtr *)&dst)) != kFskErrNone)
		cryptThrowFSK(err);
	FskMemCopy(dst, src, n);
	dst[n] = '\0';
	return(dst);
}

static char *
pem_getline(char **pp, char *endp)
{
	char *p = *pp, *line = p;

	for (; p < endp && *p != '\n'; p++)
		;
	if (p < endp) {
		*pp = p + 1;
		return(line);
	}
	else
		return(NULL);
}

static char *
pem_getKeyword(xsMachine *the, char *p)
{
	char *kw;

	if (p[10] != ' ')
		return(NULL);
	/* -----BEGIN keyword */
	kw = p += 11;
	for (; *p != '\n'; p++) {
		if (*p == '-' && FskStrCompareWithLength(p, "-----", 5) == 0)
			return(pem_strnnew(the, kw, p - kw));
	}
	return(NULL);
}

static int
pem_processHeaders(xsMachine *the, char **pp, char *endp, xsSlot *res)
{
	char *name, *value;
	char *p, *tp, *endh;

	/* first, figure out whther there is headers or not */
	tp = *pp;
	endh = NULL;
	while ((p = pem_getline(&tp, endp)) != NULL && FskStrCompareWithLength(p, "-----", 5) != 0) {
		if (*p == '\n' || (*p == '\r' && *(p+1) == '\n')) {
			endh = p;
			break;
		}
	}
	if (endh == NULL)
		return(0);	/* no headers */
	/* then, set the header name and value to the result */
	xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
	while ((p = pem_getline(pp, endh)) != NULL) {
		name = p;
		value = NULL;
		for (; *p != '\n'; p++) {
			if (value == NULL && *p == ':')		/* find the first delimiter */
				value = p + 1;
		}
		if (value) {
			name = pem_strnnew(the, name, value - name - 1);	/* remove ':' */
			for (; *value == ' '; value++)
				;
			if (*(p - 1) == '\r')
				--p;
			value = pem_strnnew(the, value, p - value);
			xsSet(xsVar(1), xsID(name), xsString(value));
			FskMemPtrDispose(name);
			FskMemPtrDispose(value);
		}
	}
	(void)pem_getline(pp, endp);	/* remove the empty line */
	xsSet(*res, xsID("headers"), xsVar(1));
	return(1);
}

static char *
pemDecodeMessage(xsMachine *the, char **pp, char *endp, xsSlot *res)
{
	char *p, *body;

	while ((p = pem_getline(pp, endp)) != NULL) {
		if (FskStrCompareWithLength(p, "-----BEGIN", 10) == 0) {
			char *keyword = pem_getKeyword(the, p);
			if (!keyword)
				continue;
			*res = xsNew1(xsThis, xsID("Message"), xsString(keyword));
			FskMemPtrDispose(keyword);
			pem_processHeaders(the, pp, endp, res);
			/* assume encodingType is always base64 */
			body = *pp;
			while ((p = pem_getline(pp, endp)) != NULL && FskStrCompareWithLength(p, "-----", 5) != 0)
				;
			if (p != NULL) {
				char *bp = pem_strnnew(the, body, p - body);
				xsSet(*res, xsID("body"), xsNew1(xsGlobal, xsID("Chunk"), xsString(bp)));
				FskMemPtrDispose(bp);
			}
			*pp = p;
			break;
		}
	}
	return(p);
}

void
xs_pem_decode(xsMachine *the)
{
	void	*data	= NULL;
	UInt32	dataLen	= 0;
	char	*p, *endp;

	xsVars(2);	/* internal use */
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		data = xsToStringCopy(xsArg(0));
		dataLen = strlen(data);
	}
	else if (xsIsInstanceOf(xsArg(0), xsChunkPrototype)) {
		data = xsGetHostData(xsArg(0));
		dataLen = xsToInteger(xsGet(xsArg(0), xsID("length")));
	}
	else
		cryptThrow("kCryptTypeError");
	xsResult = xsNewInstanceOf(xsArrayPrototype);
	p = data;
	endp = p + dataLen;
	while (pemDecodeMessage(the, &p, endp, &xsVar(0))) {
		(void)xsCall1(xsResult, xsID("push"), xsVar(0));
	}
	if (xsTypeOf(xsArg(0)) == xsStringType)
		FskMemPtrDispose(data);
}
