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
#include "xs6Platform.h"
#include "xs.h"

#ifndef howmany
#define howmany(x, y)	((x + (y) - 1) / (y))
#endif

static const int8_t b64tab[] = {
	/* start with '+' = 0x2b */
	62, -1, -1, -1, 63,				/* + , - . / */
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61,		/* 0 1 2 ... */
	-1, -1, -1,					/* : ; < */
	0,						/* = */
	-1, -1, -1,					/* > ? @ */
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,			/* A B C ... */
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25,
	-1, -1, -1, -1, -1, -1,				/* [ \ ] ^ _ ` */
	26, 27, 28, 29, 30, 31, 32, 33, 34, 35,		/* a b c ... */
	36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
	46, 47, 48, 49, 50, 51,				/* ... z */
};

static size_t
c_decode64(unsigned char *out, const char *in, size_t sz)
{
	unsigned int c1, c2, c3, c4;
	unsigned char *op = out;
#define C2I(c)	(c >= '+' && c <= 'z' ? b64tab[c-'+'] : 0)
#define OUTC(c)	if (sz-- != 0) *op++ = (c); else break

	while (*in != '\0') {
		c1 = *in != '\0' ? *in++ : 0;
		c2 = *in != '\0' ? *in++ : 0;
		c3 = *in != '\0' ? *in++ : 0;
		c4 = *in != '\0' ? *in++ : 0;
		c1 = C2I(c1);
		c2 = C2I(c2);
		OUTC((c1 << 2) | (c2 >> 4));
		if (c3 != '=') {
			c3 = C2I(c3);
			OUTC((c2 << 4) | (c3 >> 2));
			if (c4 != '=') {
				c4 = C2I(c4);
				OUTC((c3 << 6) | c4);
			}
		}
	}
	return op - out;
}

static const char b64str[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static size_t
c_encode64(char *out, const unsigned char *in, size_t sz)
{
	unsigned long l;
	long n = (long)sz;
	char *op = out;
#define ENC(x)	b64str[(x) & 0x3f]

	while (n > 0) {
		l = --n >= 0 ? *in++ : 0;
		l = (l << 8) | (--n >= 0 ? *in++ : 0);
		l = (l << 8) | (--n >= 0 ? *in++ : 0);
		*op++ = ENC(l >> 18);
		*op++ = ENC(l >> 12);
		*op++ = n < -1 ? '=' : ENC(l >> 6);
		*op++ = n < 0 ? '=' : ENC(l);
	}
	return op - out;
}

void
xs_bin_encode(xsMachine *the)
{
	void *src = xsToArrayBuffer(xsArg(0));
	size_t len = xsGetArrayBufferLength(xsArg(0));
	char *dst;
	size_t n;

	if ((dst = c_malloc(howmany(len, 3) * 4 + 1)) == NULL)
		xsErrorPrintf("Bin: no mem");
	n = c_encode64(dst, src, len);
	dst[n] = '\0';
	xsResult = xsString(dst);
	c_free(dst);
}

void
xs_bin_decode(xsMachine *the)
{
	void *src = xsToString(xsArg(0));
	size_t len = c_strlen(src);
	size_t n = howmany(len, 4) * 3;

	xsResult = xsArrayBuffer(NULL, n);
	n = c_decode64(xsToArrayBuffer(xsResult), src, n);
	xsSetArrayBufferLength(xsResult, n);
}


/*
 * PEM
 */

static char *
pem_strnnew(xsMachine *the, char *src, int n)
{
	char *dst;

	if ((dst = c_malloc(n + 1)) == NULL)
		xsUnknownError("out of memory");
	c_memcpy(dst, src, n);
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
		if (*p == '-' && c_strncmp(p, "-----", 5) == 0)
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
	while ((p = pem_getline(&tp, endp)) != NULL && c_strncmp(p, "-----", 5) != 0) {
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
			c_free(name);
			c_free(value);
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
		if (c_strncmp(p, "-----BEGIN", 10) == 0) {
			char *keyword = pem_getKeyword(the, p);
			if (!keyword)
				continue;
			*res = xsNew1(xsThis, xsID("Message"), xsString(keyword));
			c_free(keyword);
			pem_processHeaders(the, pp, endp, res);
			/* assume encodingType is always base64 */
			body = *pp;
			while ((p = pem_getline(pp, endp)) != NULL && c_strncmp(p, "-----", 5) != 0)
				;
			if (p != NULL) {
				char *bp = pem_strnnew(the, body, p - body);
				size_t n = howmany(c_strlen(bp), 4) * 3;

				xsVar(1) = xsArrayBuffer(NULL, n);
				n = c_decode64(xsToArrayBuffer(xsVar(1)), bp, n);
				xsSetArrayBufferLength(xsVar(1), n);
				xsSet(*res, xsID("body"), xsVar(1));
				c_free(bp);
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
	void *data = NULL;
	int dataLen = 0;
	char *p, *endp;

	xsVars(2);	/* internal use */
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		data = xsToString(xsArg(0));
		dataLen = c_strlen(data);
	}
	else if (xsIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		data = xsToArrayBuffer(xsArg(0));
		dataLen = xsGetArrayBufferLength(xsArg(0));
	}
	else
		xsTypeError("not a string");
	if ((p = c_malloc(dataLen)) == NULL)
		xsUnknownError("out of memory");
	c_memcpy(p, data, dataLen);
	endp = p + dataLen;
	xsResult = xsNewInstanceOf(xsArrayPrototype);
	while (pemDecodeMessage(the, &p, endp, &xsVar(0)))
		(void)xsCall1(xsResult, xsID("push"), xsVar(0));
	c_free(p);
}
