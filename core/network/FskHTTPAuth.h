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
/*
 * starting from RFC2617
 */
#ifndef __FSKHTTPAUTH__
#define __FSKHTTPAUTH__

#include "FskAssociativeArray.h"

enum {
	kFskHTTPAuthUnknown = 0,
	kFskHTTPAuthNone,
	kFskHTTPAuthDigest,
	kFskHTTPAuthBasic
};

#define HASHLEN 16
typedef char HASH[HASHLEN];
#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN+1];
#define IN
#define OUT

FskAPI(FskErr) FskHTTPAuthCalculateUsernamePasswordHash(
	IN char *pszUserName,
	IN char *pszRealm,
	IN char *pszPassword,
	OUT char ** userPassHash
);

/* calculate H(A1) as per HTTP Digest spec */
void FskHTTPAuthDigestCalcHA1(
    IN char * pszAlg,
    IN char * userPassHash,
    IN char * pszNonce,
    IN char * pszCNonce,
    OUT HASHHEX SessionKey
    );

/* calculate request-digest/response-digest as per HTTP Digest spec */
void FskHTTPAuthDigestCalcResponse(
    IN HASHHEX HA1,           /* H(A1) */
    IN char * pszNonce,       /* nonce from server */
    IN char * pszNonceCount,  /* 8 hex digits */
    IN char * pszCNonce,      /* client nonce */
    IN char * pszQop,         /* qop-value: "", "auth", "auth-int" */
    IN char * pszMethod,      /* method from the request */
    IN char * pszDigestUri,   /* requested URL */
    IN HASHHEX HEntity,       /* H(entity body) if qop="auth-int" */
    OUT HASHHEX Response      /* request-digest or response-digest */
    );


FskAPI(FskErr) FskHTTPAuthParseWWWAuthenticate(char *line, int *authType, FskAssociativeArray *out);
FskAPI(FskErr) FskHTTPAuthParseAuthenticationInfo(char *line, FskAssociativeArray *out);

#endif // __FSKHTTPAUTH__

