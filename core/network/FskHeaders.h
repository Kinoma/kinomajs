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
#ifndef __HEADERS_H__
#define __HEADERS_H__

#include "FskUtilities.h"
#include "FskAssociativeArray.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kFskStrHost					"Host"
#define kFskStrDate					"Date"
#define kFskStrConnection			"Connection"
#define kFskStrProxyConnection		"Proxy-Connection"
#define kFskStrContentLength		"Content-Length"
#define kFskStrContentType			"Content-Type"
#define kFskStrCacheControl			"Cache-Control"
#define kFskStrLocation				"Location"
#define kFskStrExpect				"Expect"
#define kFskStrExpires				"Expires"
#define kFskStrServer				"Server"
#define kFskStrTransferEncoding		"Transfer-Encoding"
#define kFskStrKeepAlive			"Keep-Alive"
#define kFskStrAllow				"Allow"

#define kFskStr100Continue			"100-continue"

#define kFskStrChunked				"chunked"
#define kFskStrMaxAge				"max-age"
#define kFskStrClose				"close"
#define	kFskStrTextXMLCharset		"text/xml; charset=\"utf-8\""
#define kFskStrTextHTML				"text/html"
#define kFskServerAllow				"GET, HEAD, OPTIONS"
#define kFskStrXSeekRange			"X-Seek-Range"

#define kFskStrSubscribe			"SUBSCRIBE"
#define kFskStrUnsubscribe			"UNSUBSCRIBE"
#define kFskStrFskOriginalRequestor	"x-fsk-original-requestor"
#define kFskStrCallBack				"Call-Back"
#define kFskStrTimeout				"Timeout"
#define kFskStrSubscriptionID		"Subscription-ID"
#define kFskStrSID					"SID"

#define kFskStrGET					"GET"
#define kFskStrHEAD					"HEAD"
#define kFskStrPOST					"POST"
#define kFskStrOPTIONS				"OPTIONS"

#define kFskHTTPVersion1dot0		(0x00010000)
#define kFskHTTPVersion1dot1		(0x00010001)


enum {
	kFskHeadersDoNotMergeDuplicates = 1 << 0,
	kFskHeadersDoNotStripURILeadingSlash = 1 << 1,
	kFskHeadersNonStandardResponseReasonPhrase = 1 << 2
};

enum {
	kFskHeaderTypeUnknown = 0,
	kFskHeaderTypeRequest = 10,
	kFskHeaderTypeResponse= 20
};

typedef struct FskHeaders {
	Boolean		headersParsed;
	int			headerType;
	int			flags;

	char		*method;
	char		*protocol;
	char		*URI;
	char		*filename;			// Request pre-parsed
	FskAssociativeArray	parameters;
	int			responseCode;		// Response code
	char		*responseReasonPhrase;

	FskAssociativeArray	theHeaders;

	char		*leftover;
	char		*lastName;
	
} FskHeaders;

typedef FskAssociativeArrayIterator	FskHeaderIterator;
typedef FskAssociativeArrayNameList FskHeaderValues;

typedef struct FskResponseCode {
	int			HTTP_code;
	char		*HTTP_description;
} FskResponseCode;

#define lineEnd(a)		(FskStrIsCRLF((a)[0]) && FskStrIsCRLF((a)[1]))

FskAPI (FskErr) FskHeaderStructNew(FskHeaders **headers);
FskAPI (void) FskHeaderStructDispose(FskHeaders *headers);

FskAPI (int) FskHeadersParseChunk(char *blob, int blobSize, UInt16 headerType, FskHeaders *headers);
FskAPI (void) FskHeaderAddString(char *headerName, char *headerVal, FskHeaders *header);
FskAPI (void) FskHeaderAddInteger(char *headerName, SInt32 headerVal, FskHeaders *header);
FskAPI (void) FskHeaderRemove(char *headerName, FskHeaders *header);
FskAPI (char) *FskHeaderFind(char *headerName, FskHeaders *header);

FskAPI (FskHeaderIterator) FskHeaderIteratorNew(FskHeaders *headers);
FskAPI (FskHeaderIterator) FskHeaderIteratorNext(FskHeaderIterator iter);
FskAPI (void) FskHeaderIteratorDispose(FskHeaderIterator iter);

FskAPI (int) FskHeaderGenerateOutputLine(FskHeaderIterator iter, char *line, int lineLength);
FskAPI (int) FskHeaderGenerateOutputBlob(char *blob, int blobLength, Boolean withTerminator, FskHeaders *headers);

#define FskHeaderResponseCategory(r)	((r)/100)
enum {
	kHTTPResponseInformationalType	= 1,
	kHTTPResponseSuccessfulType		= 2,
	kHTTPResponseRedirectionType	= 3,
	kHTTPResponseClientErrorType	= 4,
	kHTTPResponseServerErrorType	= 5
};

FskAPI (char *) FskFindResponse(int code);

FskAPI (int) FskHeaderResponseCode(FskHeaders *headers);
FskAPI (char*) FskHeaderResponseReasonPhrase(FskHeaders *headers);
FskAPI (int) FskHeaderType(FskHeaders *headers);
FskAPI (char*) FskHeaderMethod(FskHeaders *headers);
FskAPI (UInt32) FskHeaderHTTPVersion(FskHeaders *headers);
FskAPI (char*) FskHeaderFilename(FskHeaders *headers);
FskAPI (FskAssociativeArray) FskHeaderParameters(FskHeaders *headers);
FskAPI (char*) FskHeaderURI(FskHeaders *headers);

FskAPI (char*) FskHeaderGetParameterValue(FskHeaders *headers, const char *name);

#ifdef __cplusplus
}
#endif

#endif // __HEADERS_H__

