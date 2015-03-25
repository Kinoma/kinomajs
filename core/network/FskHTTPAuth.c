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
#include "Fsk.h"
#include "FskMemory.h"
#include "FskString.h"
#include "FskHTTPAuth.h"
#include "FskAssociativeArray.h"

#include "md5.c"

static void CvtHex(
    HASH Bin,
    HASHHEX Hex
    )
{
    unsigned short i;
    unsigned char j;

    for (i = 0; i < HASHLEN; i++) {
        j = (Bin[i] >> 4) & 0xf;
        if (j <= 9)
            Hex[i*2] = (j + '0');
         else
            Hex[i*2] = (j + 'a' - 10);
        j = Bin[i] & 0xf;
        if (j <= 9)
            Hex[i*2+1] = (j + '0');
         else
            Hex[i*2+1] = (j + 'a' - 10);
    };
    Hex[HASHHEXLEN] = '\0';
}

FskErr FskHTTPAuthCalculateUsernamePasswordHash(
    char * pszUserName,
    char * pszRealm, 
    char * pszPassword,
    char ** userPassHash 
) {      
    struct md5_hash_state Md5Ctx; 
    FskErr err = kFskErrNone;
    char    *hashStr;

	if (!pszUserName || !pszRealm || !pszPassword)
		return kFskErrParameterError;
    
	err = FskMemPtrNewClear(HASHLEN + 1, &hashStr);
	BAIL_IF_ERR(err);
    
	md5_init(&Md5Ctx);
	md5_process(&Md5Ctx, (const unsigned char *)pszUserName, FskStrLen(pszUserName));
	md5_process(&Md5Ctx, (const unsigned char *)":", 1);
	md5_process(&Md5Ctx, (const unsigned char *)pszRealm, FskStrLen(pszRealm));
	md5_process(&Md5Ctx, (const unsigned char *)":", 1);
	md5_process(&Md5Ctx, (const unsigned char *)pszPassword, FskStrLen(pszPassword));
	md5_done(&Md5Ctx, (unsigned char *)hashStr);
	*userPassHash = hashStr;

bail:
	return err;
}

/* calculate H(A1) as per spec */
void FskHTTPAuthDigestCalcHA1(
    char * pszAlg,
    char * userPassHash,
    char * pszNonce,
    char * pszCNonce,
    HASHHEX SessionKey
    )
{
      struct md5_hash_state Md5Ctx;
      HASH HA1;

      if (FskStrCompareCaseInsensitive(pszAlg, "md5-sess") == 0) {
            md5_init(&Md5Ctx);
            md5_process(&Md5Ctx, (const unsigned char *)userPassHash, HASHLEN);
            md5_process(&Md5Ctx, (const unsigned char *)":", 1);
            md5_process(&Md5Ctx, (const unsigned char *)pszNonce, FskStrLen(pszNonce));
            md5_process(&Md5Ctx, (const unsigned char *)":", 1);
            md5_process(&Md5Ctx, (const unsigned char *)pszCNonce, FskStrLen(pszCNonce));
            md5_done(&Md5Ctx, (unsigned char *)HA1);
			CvtHex(HA1, SessionKey);
      }
		else
			CvtHex(userPassHash, SessionKey);
}

/* calculate request-digest/response-digest as per HTTP Digest spec */
void FskHTTPAuthDigestCalcResponse(
    HASHHEX HA1,           /* H(A1) */
    char * pszNonce,       /* nonce from server */
    char * pszNonceCount,  /* 8 hex digits */
    char * pszCNonce,      /* client nonce */
    char * pszQop,         /* qop-value: "", "auth", "auth-int" */
    char * pszMethod,      /* method from the request */
    char * pszDigestUri,   /* requested URL */
    HASHHEX HEntity,       /* H(entity body) if qop="auth-int" */
    HASHHEX Response      /* request-digest or response-digest */
    )
{
      struct md5_hash_state  Md5Ctx;
      HASH HA2;
      HASH RespHash;
       HASHHEX HA2Hex;

      // calculate H(A2)
      md5_init(&Md5Ctx);
      md5_process(&Md5Ctx, (const unsigned char *)pszMethod, FskStrLen(pszMethod));
      md5_process(&Md5Ctx, (const unsigned char *)":", 1);
      md5_process(&Md5Ctx, (const unsigned char *)pszDigestUri, FskStrLen(pszDigestUri));
      if (FskStrCompareCaseInsensitive(pszQop, "auth-int") == 0) {
            md5_process(&Md5Ctx, (const unsigned char *)":", 1);
            md5_process(&Md5Ctx, (const unsigned char *)HEntity, HASHHEXLEN);
      };
      md5_done(&Md5Ctx, (unsigned char *)HA2);
       CvtHex(HA2, HA2Hex);

      // calculate response
      md5_init(&Md5Ctx);
      md5_process(&Md5Ctx, (const unsigned char *)HA1, HASHHEXLEN);
      md5_process(&Md5Ctx, (const unsigned char *)":", 1);
      md5_process(&Md5Ctx, (const unsigned char *)pszNonce, FskStrLen(pszNonce));
      md5_process(&Md5Ctx, (const unsigned char *)":", 1);
      if (pszQop && *pszQop) {
          md5_process(&Md5Ctx, (const unsigned char *)pszNonceCount, FskStrLen(pszNonceCount));
          md5_process(&Md5Ctx, (const unsigned char *)":", 1);
          md5_process(&Md5Ctx, (const unsigned char *)pszCNonce, FskStrLen(pszCNonce));
          md5_process(&Md5Ctx, (const unsigned char *)":", 1);
          md5_process(&Md5Ctx, (const unsigned char *)pszQop, FskStrLen(pszQop));
          md5_process(&Md5Ctx, (const unsigned char *)":", 1);
      };
      md5_process(&Md5Ctx, (const unsigned char *)HA2Hex, HASHHEXLEN);
      md5_done(&Md5Ctx, (unsigned char *)RespHash);
      CvtHex(RespHash, Response);
}

static FskErr sNameValueSplit(char *line, FskAssociativeArray *out) {
	FskErr err = kFskErrNone;
	char *c, n[256],v[1024];

	*out = FskAssociativeArrayNew();
	if (*out == NULL) {
		BAIL(kFskErrMemFull);
	}
	c = line;
	while (1) {
		char cc[2];
		c = FskStrStripHeadSpace(c);
		if ('\0' == *c) break;
		c = FskStrNCopyUntil(n, c, 255, '=');
		if ('\0' == *c) break;
		v[0] = cc[0] = *c++;
		cc[1] = 0;
		c = FskStrNCopyUntil(v + 1, c, 1024, cc[0]);
		FskStrCat(v, cc);
		FskAssociativeArrayElementSetString(*out, n, v);
		while (',' == *c)
			c++;
	}
	
bail:
	if (err && *out) {
		FskAssociativeArrayDispose(*out);
		*out = NULL;
	}
	return err;
}

FskErr FskHTTPAuthParseAuthenticationInfo(char *line, FskAssociativeArray *out)
{
	return sNameValueSplit(line, out);
}

FskErr FskHTTPAuthParseWWWAuthenticate(char *line, int *authType, FskAssociativeArray *out)
{
	char *c, *p;

	*authType = kFskHTTPAuthNone;
	*out = NULL;
	p = FskStrStr(line, "Digest");
	if (p) {
		*authType = kFskHTTPAuthDigest;
		c = FskStrChr(p, ' ');
		if (c) c++;
	}
	else {
		p = FskStrStr(line, "Basic");
		if (p) {
			*authType = kFskHTTPAuthBasic;
			c = FskStrChr(p, ' ');
			if (c) c++;
		}
		else {
			*authType = kFskHTTPAuthNone;
			c = line;
		}
	}

	if (c)
		return sNameValueSplit(c, out);
	else
		return kFskErrNone;
}

