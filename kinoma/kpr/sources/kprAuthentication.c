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
//#include "fips180.h"
#include "kprAuthentication.h"
#include "kprUtilities.h"

FskErr KprAuthenticationDigestMD5(char* username, UInt32 usernameSize, char* password, UInt32 passwordSize, char* uri, UInt32 uriSize, char* challenge, char** response, char** verification)
{
	FskErr err = kFskErrNone;
//	char* algorithm = NULL;
	char* charset = NULL;
	char* cnonce = NULL;
	char* nc = NULL;
	char* nonce = NULL;
	char* qop = NULL;
	char* realm = NULL;
	
	char* name = NULL;
	char* value = NULL;
	char* ptr;
	char* end;
	char random[25];
	
	char* a1 = NULL;
	char* a2 = NULL;
	char* a = NULL;
	UInt32 size;
	
//	UInt32 algorithmSize = 0;
	UInt32 charsetSize = 0;
	UInt32 cnonceSize = 0;
	UInt32 ncSize = 0;
	UInt32 nonceSize = 0;
	UInt32 qopSize = 0;
	UInt32 realmSize = 0;
	
	char* escapedUsername = NULL;
	UInt32 escapedUsernameSize;
	UInt32 i;
	
	if (usernameSize == 0)
		usernameSize = FskStrLen(username);
	if (passwordSize == 0)
		passwordSize = FskStrLen(password);
	if (uriSize == 0)
		uriSize = FskStrLen(uri);
	
	for (i = 0, size = 0; i < usernameSize; i++) {
		switch (username[i]) {
			case '"': case '\\':
				size += 2;
				break;
			default:
				size += 1;
				break;
		}
	}
	bailIfError(FskMemPtrNew(size + 1, &escapedUsername));
	ptr = escapedUsername;
	for (i = 0; i < usernameSize; i++) {
		char c = username[i];
		switch (c) {
			case '"': case '\\':
				*ptr++ = '\\';
				*ptr++ = c;
				break;
			default:
				*ptr++ = c;
				break;
		}
	}
	escapedUsername[size] = '\0';
	escapedUsernameSize = size;
	
	ptr = challenge;
	while (ptr) {
		end = FskStrChr(ptr, '=');
		if (!end) BAIL(kFskErrBadData);
		*end = 0;
		name = FskStrStripHeadSpace(ptr);
		name = FskStrStripTailSpace(name);
		
		ptr = FskStrChr(end + 1, '"');
		if (ptr) {
			value = FskStrStripHeadSpace(ptr + 1);
			end = FskStrChr(value, '"');
			if (!end) BAIL(kFskErrBadData);
			*end = 0;
		}
		else {
			value = FskStrStripHeadSpace(end + 1);
			end = FskStrChr(value, ',');
			if (end) *end = 0;
		}
		value = FskStrStripTailSpace(value);
		
		if (end) {
			ptr = FskStrStripHeadSpace(end + 1);
			if (ptr && (*ptr == ',')) ptr++;
		}
		else
			ptr = NULL;
		
//		if (!FskStrCompare(name, "algorithm")) {
//			algorithm = value;
//			algorithmSize = FskStrLen(algorithm);
//		}
		if (!FskStrCompare(name, "charset")) {
			charset = value;
			charsetSize = FskStrLen(charset);
		}
		if (!FskStrCompare(name, "nc")) {
			nc = value;
			ncSize = FskStrLen(nc);
		}
		if (!FskStrCompare(name, "nonce")) {
			nonce = value;
			nonceSize = FskStrLen(nonce);
		}
		if (!FskStrCompare(name, "qop")) {
			qop = value;
			qopSize = FskStrLen(qop);
		}
		if (!FskStrCompare(name, "realm")) {
			realm = value;
			realmSize = FskStrLen(realm);
		}
	}
	if (!nc) {
		nc = "00000001";
		ncSize = 8;
	}
	if (!qop) {
		qop = "auth";
		qopSize = 4;
	}
	
	// random nonce
	sprintf(random, "%08x%08x%08x", (unsigned int)FskRandom(), (unsigned int)FskRandom(), (unsigned int)FskRandom());
	FskStrB64Encode(random, 24, &cnonce, &cnonceSize, false);
	if (cnonceSize == 0)
		BAIL(kFskErrMemFull);
	cnonceSize--; // do not count NULL termination!
	
	// calculate response	
	if (response || verification) {
		size = usernameSize + 1/*:*/ + realmSize + 1/*:*/ + passwordSize;
		bailIfError(FskMemPtrNew(size + 1, &a1));
		sprintf(a1, "%s:%s:%s", username, realm ? realm : "", password);
		bailIfError(KprCryptMD5(a1, size, (UInt8*)a1, NULL));
		size = 16 + 1/*:*/ + nonceSize + 1/*:*/ + cnonceSize;
		bailIfError(FskMemPtrRealloc(size + 1, &a1));
		sprintf(a1 + 16, ":%s:%s", nonce, cnonce);
		bailIfError(KprCryptMD5(a1, size, NULL, a1));
	}
	
	if (response) {
		size = 13/*AUTHENTICATE:*/ + uriSize;
		bailIfError(FskMemPtrNew(2 * size + 1, &a2));
		sprintf(a2, "AUTHENTICATE:%s", uri);
		bailIfError(KprCryptMD5(a2, size, NULL, a2));
		
		size = 32 + 1/*:*/ + nonceSize + 1/*:*/ + ncSize + 1/*:*/ + cnonceSize + 1/*:*/ + qopSize + 1/*:*/ + 32;
		bailIfError(FskMemPtrNew(size + 1, &a));
		sprintf(a, "%s:%s:%s:%s:%s:%s", a1, nonce, nc, cnonce, qop, a2);
		bailIfError(KprCryptMD5(a, size, NULL, a));
		
		if (realm) {
			// 81 = username="",realm="",nonce="",cnonce="",nc=,digest-uri="",qop=,response=,charset=
			size = 81 + escapedUsernameSize + realmSize + nonceSize + cnonceSize + ncSize + uriSize + qopSize + FskStrLen(a) + charsetSize;
			bailIfError(FskMemPtrNew(size + 1, response));
			sprintf(*response, "username=\"%s\",realm=\"%s\",nonce=\"%s\",cnonce=\"%s\",nc=%s,digest-uri=\"%s\",qop=%s,response=%s,charset=%s",
				escapedUsername, realm, nonce, cnonce, nc, uri, qop, a, charset);
		}
		else {
			// 72 = username="",nonce="",cnonce="",nc=,digest-uri="",qop=,response=,charset=
			size = 72 + escapedUsernameSize + nonceSize + cnonceSize + ncSize + uriSize + qopSize + FskStrLen(a) + charsetSize;
			bailIfError(FskMemPtrNew(size + 1, response));
			sprintf(*response, "username=\"%s\",nonce=\"%s\",cnonce=\"%s\",nc=%s,digest-uri=\"%s\",qop=%s,response=%s,charset=%s",
				escapedUsername, nonce, cnonce, nc, uri, qop, a, charset);
		}
	}
	
	if (verification) {
		FskMemPtrDispose(a2);
		FskMemPtrDispose(a);
		
		size = 1/*:*/ + uriSize;
		bailIfError(FskMemPtrNew(2 * size + 1, &a2));
		sprintf(a2, ":%s", uri);
		bailIfError(KprCryptMD5(a2, size, NULL, a2));
		
		size = 32 + 1/*:*/ + nonceSize + 1/*:*/ + ncSize + 1/*:*/ + cnonceSize + 1/*:*/ + qopSize + 1/*:*/ + 32;
		bailIfError(FskMemPtrNew(size + 1, &a));
		sprintf(a, "%s:%s:%s:%s:%s:%s", a1, nonce, nc, cnonce, qop, a2);
		bailIfError(KprCryptMD5(a, size, NULL, a));
		
		size = 8/*rspauth=*/ + FskStrLen(a);
		bailIfError(FskMemPtrNew(size + 1, verification));
		sprintf(*verification, "rspauth=%s", a);
	}
bail:
	FskMemPtrDispose(escapedUsername);
	FskMemPtrDispose(cnonce);
	FskMemPtrDispose(a2);
	FskMemPtrDispose(a1);
	FskMemPtrDispose(a);
	return err;
}

FskErr KprAuthenticationSCRAMSHA1Message(const char* username, UInt32 usernameSize, char** message)
{
	FskErr err = kFskErrNone;
#if 0
	char *result = NULL;
	UInt32 size;
	
	if (usernameSize == 0)
		usernameSize = FskStrLen(username);
	size = 2/*n=*/ + usernameSize + 6/*,r=KPR*/ + 24/*%08x%08x%08x*/;
	bailIfError(FskMemPtrNew(size + 1, &result));
	FskMemCopy(result + 0, "n=", 2);
	FskMemCopy(result + 2, username, usernameSize);
	FskMemCopy(result + 2 + usernameSize, ",r=KPR", 6);
	sprintf(result + 2 + usernameSize + 6, "%08x%08x%08x", (unsigned int)random(), (unsigned int)random(), (unsigned int)random());
	result[size] = '\0';
	*message = result;
bail:
#endif
	return err;
}

FskErr KprAuthenticationSCRAMSHA1MessageToken(const char* message, UInt32 messageSize, char** messageToken)
{
	FskErr err = kFskErrNone;
#if 0
	char *result = NULL;
	UInt32 resultSize, size;
	char *token = NULL;
	
	if (messageSize == 0)
		messageSize = FskStrLen(message);
	size = 3/*n,,*/ + messageSize;
	bailIfError(FskMemPtrNew(size, &token));
	FskMemCopy(token + 0, "n,,", 3);
	FskMemCopy(token + 3, message, messageSize);
	FskStrB64Encode(token, size, &result, &resultSize, false);
	if (resultSize == 0)
		BAIL(kFskErrMemFull);
	resultSize--; // do not count NULL termination!
	*messageToken = result;
bail:
	FskMemPtrDispose(token);
#endif
	return err;
}

FskErr KprAuthenticationSCRAMSHA1Response(const char *value, UInt32 valueSize, const char* message, UInt32 messageSize, const char* password, UInt32 passwordSize, char** response, char** verifier)
{
	FskErr err = kFskErrNone;
#if 0
	char *result = NULL;
	UInt32 iterationCount, resultSize, size;
	char *clientFinalMessage = NULL, *clientFinalMessageWithoutProof = NULL, *clientProof = NULL;
	char *serverFirstMessage = NULL, *serverNonce = NULL, *serverSalt = NULL, *serverSignature = NULL;
	UInt32 clientFinalMessageWithoutProofSize, clientProofSize;
	UInt32 serverFirstMessageSize, serverNonceSize, serverSaltSize, serverSignatureSize;
	char *start, *stop;
	
	// http://tools.ietf.org/html/rfc5802#section-3
	unsigned char SaltedPassword[KPR_CRYPT_SHA1_HASH_SIZE];		// SaltedPassword  := Hi(Normalize(password), salt, i)
	unsigned char ClientKey[KPR_CRYPT_SHA1_HASH_SIZE];			// ClientKey       := HMAC(SaltedPassword, "Client Key")
	unsigned char StoredKey[KPR_CRYPT_SHA1_HASH_SIZE];			// StoredKey       := H(ClientKey)
	char *AuthMessage = NULL;									// AuthMessage     := client-first-message-bare + "," +
																//                    server-first-message + "," +
																//                    client-final-message-without-proof
	unsigned char ClientSignature[KPR_CRYPT_SHA1_HASH_SIZE];	// ClientSignature := HMAC(StoredKey, AuthMessage)
	char ClientProof[KPR_CRYPT_SHA1_HASH_SIZE];					// ClientProof     := ClientKey XOR ClientSignature
	unsigned char ServerKey[KPR_CRYPT_SHA1_HASH_SIZE];			// ServerKey       := HMAC(SaltedPassword, "Server Key")
	unsigned char ServerSignature[KPR_CRYPT_SHA1_HASH_SIZE];	// ServerSignature := HMAC(ServerKey, AuthMessage)
	
	if (valueSize == 0)
		valueSize = FskStrLen(value);
	bailIfError(FskStrB64Decode(value, valueSize, &serverFirstMessage, &serverFirstMessageSize));
	if (serverFirstMessageSize == 0)
		BAIL(kFskErrMemFull);
	if (messageSize == 0)
		messageSize = FskStrLen(message);
	if (passwordSize == 0)
		passwordSize = FskStrLen(password);
	
	// server nonce
	start = serverFirstMessage + 2/*r=*/;
	stop = FskStrChr(start, ',');
	bailIfNULL(stop);
	size = stop - start;
	bailIfError(FskMemPtrNew(size + 1, &serverNonce));
	FskMemCopy(serverNonce, start, size);
	serverNonce[size] = '\0';
	serverNonceSize = size;
	
	// server salt
	start = stop + 1/*,*/ + 2/*s=*/;
	stop = FskStrChr(start, ',');
	bailIfNULL(stop);
	size = stop - start;
	bailIfError(FskStrB64Decode(start, size, &serverSalt, &serverSaltSize));
	
	// iteration count
	start = stop + 1/*,*/ + 2/*i=*/;
	iterationCount = FskStrToNum(start);
	
	// check client nonce
	result = FskStrChr(message, ',');
	bailIfNULL(result);
	size = messageSize - (result + 1 - message);
	if (FskStrCompareWithLength(serverFirstMessage, result + 1, size))
		BAIL(kFskErrAuthFailed);
	
	// client-final-message-without-proof
	size = 9/*c=biws,r=*/ + serverNonceSize;
	bailIfError(FskMemPtrNew(size, &clientFinalMessageWithoutProof));
	FskMemCopy(clientFinalMessageWithoutProof + 0, "c=biws,r=", 9);
	FskMemCopy(clientFinalMessageWithoutProof + 9, serverNonce, serverNonceSize);
	clientFinalMessageWithoutProofSize = size;
	
	bailIfError(KprCryptPKCS5_PBKDF2_HMAC_SHA1(password, passwordSize, serverSalt, serverSaltSize, iterationCount, KPR_CRYPT_SHA1_HASH_SIZE, SaltedPassword));
	bailIfError(KprCryptHMAC_SHA1(SaltedPassword, KPR_CRYPT_SHA1_HASH_SIZE, "Client Key", 10, ClientKey));
	bailIfError(KprCryptHMAC_SHA1(SaltedPassword, KPR_CRYPT_SHA1_HASH_SIZE, "Server Key", 10, ServerKey));
	bailIfError(KprCryptSHA1(ClientKey, KPR_CRYPT_SHA1_HASH_SIZE, StoredKey, NULL));
	
	size = messageSize + 1/*,*/ + serverFirstMessageSize + 1/*,*/ + clientFinalMessageWithoutProofSize;
	bailIfError(FskMemPtrNew(size, &AuthMessage));
	FskMemCopy(AuthMessage + 0, message, messageSize);
	FskMemCopy(AuthMessage + messageSize, ",", 1);
	FskMemCopy(AuthMessage + messageSize + 1, serverFirstMessage, serverFirstMessageSize);
	FskMemCopy(AuthMessage + messageSize + 1 + serverFirstMessageSize, ",", 1);
	FskMemCopy(AuthMessage + messageSize + 1 + serverFirstMessageSize + 1, clientFinalMessageWithoutProof, clientFinalMessageWithoutProofSize);
	bailIfError(KprCryptHMAC_SHA1(StoredKey, KPR_CRYPT_SHA1_HASH_SIZE, (const unsigned char *)AuthMessage, size, ClientSignature));
	bailIfError(KprCryptHMAC_SHA1(ServerKey, KPR_CRYPT_SHA1_HASH_SIZE, (const unsigned char *)AuthMessage, size, ServerSignature));
	FskStrB64Encode(ServerSignature, KPR_CRYPT_SHA1_HASH_SIZE, &serverSignature, &serverSignatureSize, false);
	if (serverSignatureSize == 0)
		BAIL(kFskErrMemFull);
	serverSignatureSize--; // do not count NULL termination!
	size = 2/*v=*/ + serverSignatureSize;
	bailIfError(FskMemPtrNew(size, &result));
	FskMemCopy(result + 0, "v=", 2);
	FskMemCopy(result + 2, serverSignature, serverSignatureSize);
	FskStrB64Encode(result, size, &result, &resultSize, false);
	if (resultSize == 0)
		BAIL(kFskErrMemFull);
	resultSize--; // do not count NULL termination!
	*verifier = result;
	
	KprCryptXOR(ClientKey, ClientSignature, KPR_CRYPT_SHA1_HASH_SIZE, ClientProof);
	FskStrB64Encode(ClientProof, KPR_CRYPT_SHA1_HASH_SIZE, &clientProof, &clientProofSize, false);
	if (clientProofSize == 0)
		BAIL(kFskErrMemFull);
	clientProofSize--; // do not count NULL termination!
	
	// client-final-message
	size = clientFinalMessageWithoutProofSize + 3/*,p=*/ + clientProofSize;
	bailIfError(FskMemPtrNew(size, &clientFinalMessage));
	FskMemCopy(clientFinalMessage + 0, clientFinalMessageWithoutProof, clientFinalMessageWithoutProofSize);
	FskMemCopy(clientFinalMessage + clientFinalMessageWithoutProofSize, ",p=", 3);
	FskMemCopy(clientFinalMessage + clientFinalMessageWithoutProofSize + 3, clientProof, clientProofSize);
	FskStrB64Encode(clientFinalMessage, size, &result, &resultSize, false);
	if (resultSize == 0)
		BAIL(kFskErrMemFull);
	resultSize--; // do not count NULL termination!
	*response = result;
bail:
	FskMemPtrDispose(AuthMessage);
	FskMemPtrDispose(clientFinalMessage);
	FskMemPtrDispose(clientFinalMessageWithoutProof);
	FskMemPtrDispose(clientProof);
	FskMemPtrDispose(serverFirstMessage);
	FskMemPtrDispose(serverNonce);
	FskMemPtrDispose(serverSalt);
	FskMemPtrDispose(serverSignature);
#endif
	return err;
}
