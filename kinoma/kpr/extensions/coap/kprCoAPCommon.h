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
#ifndef __KPRCOAPCOMMON__
#define __KPRCOAPCOMMON__

#include "kpr.h"

#define debugDisplayFunction 0

#if defined(debugDisplayFunction) && debugDisplayFunction
#define ENTER_FUNCTION() printf("ENTER: %s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__)
#define RETURN_FUNCTION() printf("RETURN: %d %s (%s:%d)\n", err, __FUNCTION__, __FILE__, __LINE__); return err
#define RETURN_FUNCTION_WITH(V) printf("RETURN: %lx %s (%s:%d)\n", (long int) (V), __FUNCTION__, __FILE__, __LINE__); return (V)
#define EXIT_FUNCTION() printf("EXIT: %s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__)
#else
#define ENTER_FUNCTION()
#define RETURN_FUNCTION() return err
#define RETURN_FUNCTION_WITH(V) return V
#define EXIT_FUNCTION()
#endif

//--------------------------------------------------
// Typedefs
//--------------------------------------------------

typedef struct KprCoAPClientRecord KprCoAPClientRecord, *KprCoAPClient;
typedef struct KprCoAPServerRecord KprCoAPServerRecord, *KprCoAPServer;
typedef struct KprCoAPEndpointRecord KprCoAPEndpointRecord, *KprCoAPEndpoint;
typedef struct KprCoAPMessageRecord KprCoAPMessageRecord, *KprCoAPMessage;
typedef struct KprCoAPReceiverRecord KprCoAPReceiverRecord, *KprCoAPReceiver;

/*
 +--------------------------+----------+----+------------------------+
 | Media type               | Encoding | ID | Reference              |
 +--------------------------+----------+----+------------------------+
 | text/plain;              | -        |  0 | [RFC2046] [RFC3676]    |
 | charset=utf-8            |          |    | [RFC5147]              |
 | application/link-format  | -        | 40 | [RFC6690]              |
 | application/xml          | -        | 41 | [RFC3023]              |
 | application/octet-stream | -        | 42 | [RFC2045] [RFC2046]    |
 | application/exi          | -        | 47 | [REC-exi-20140211]     |
 | application/json         | -        | 50 | [RFC7159]              |
 +--------------------------+----------+----+------------------------+
 */

typedef enum {
	kKprCoAPContentFormatNone = -1,
	kKprCoAPContentFormatPlainText = 0,
	kKprCoAPContentFormatLinkFormat = 40,
	kKprCoAPContentFormatXml = 41,
	kKprCoAPContentFormatOctetStream = 42,
	kKprCoAPContentFormatExi = 47,
	kKprCoAPContentFormatJson = 50,
} KprCoAPContentFormat;

typedef enum {
	kKprCoAPRequestMethodGET = 1,
	kKprCoAPRequestMethodPOST = 2,
	kKprCoAPRequestMethodPUT = 3,
	kKprCoAPRequestMethodDELETE = 4,
} KprCoAPRequestMethod;

typedef enum {
	kKprCoAPMessageTypeConfirmable = 0,
	kKprCoAPMessageTypeNonConfirmable = 1,
	kKprCoAPMessageTypeAcknowledgement = 2,
	kKprCoAPMessageTypeReset = 3,
} KprCoAPMessageType;

#define KprCoAPMessageCodeWith(cls, detail) ((((cls) & 0x7) << 5) | ((detail) & 0x1f))

typedef enum {
	kKprCoAPMessageCodeEmpty    = 0,

	// Request
	kKprCoAPMessageCodeGET		= KprCoAPMessageCodeWith(0, 01),
	kKprCoAPMessageCodePOST		= KprCoAPMessageCodeWith(0, 02),
	kKprCoAPMessageCodePUT		= KprCoAPMessageCodeWith(0, 03),
	kKprCoAPMessageCodeDELETE	= KprCoAPMessageCodeWith(0, 04),

	// Response - Success

	kKprCoAPMessageCodeCreated	= KprCoAPMessageCodeWith(2, 01),
	kKprCoAPMessageCodeDeleted	= KprCoAPMessageCodeWith(2, 02),
	kKprCoAPMessageCodeValid	= KprCoAPMessageCodeWith(2, 03),
	kKprCoAPMessageCodeChanged	= KprCoAPMessageCodeWith(2, 04),
	kKprCoAPMessageCodeContent	= KprCoAPMessageCodeWith(2, 05),

	// Response - Client Error

	kKprCoAPMessageCodeBadRequest	= KprCoAPMessageCodeWith(4, 00),
	kKprCoAPMessageCodeUnauthorized	= KprCoAPMessageCodeWith(4, 01),
	kKprCoAPMessageCodeBadOption	= KprCoAPMessageCodeWith(4, 02),
	kKprCoAPMessageCodeForbidden	= KprCoAPMessageCodeWith(4, 03),
	kKprCoAPMessageCodeNotFound		= KprCoAPMessageCodeWith(4, 04),
	kKprCoAPMessageCodeMethodNotAllowed	= KprCoAPMessageCodeWith(4, 05),
	kKprCoAPMessageCodeNotAcceptable	= KprCoAPMessageCodeWith(4, 06),
	kKprCoAPMessageCodePreconditionFailed	= KprCoAPMessageCodeWith(4, 12),
	kKprCoAPMessageCodeRequestEntityTooLarge	= KprCoAPMessageCodeWith(4, 13),
	kKprCoAPMessageCodeUnsupportedContentFormat	= KprCoAPMessageCodeWith(4, 15),

	// Response - Server Error

	kKprCoAPMessageCodeInternalServerError	= KprCoAPMessageCodeWith(5, 00),
	kKprCoAPMessageCodeNotImplemented	= KprCoAPMessageCodeWith(5, 01),
	kKprCoAPMessageCodeBadGateway	= KprCoAPMessageCodeWith(5, 02),
	kKprCoAPMessageCodeServiceUnavailable	= KprCoAPMessageCodeWith(5, 03),
	kKprCoAPMessageCodeGatewayTimeout	= KprCoAPMessageCodeWith(5, 04),
	kKprCoAPMessageCodeProxyingNotSupported	= KprCoAPMessageCodeWith(5, 05),

} KprCoAPMessageCode;

/*
 5.10.  Option Definitions

 The individual CoAP options are summarized in Table 4 and explained
 in the subsections of this section.

 In this table, the C, U, and N columns indicate the properties
 Critical, UnSafe, and NoCacheKey, respectively.  Since NoCacheKey
 only has a meaning for options that are Safe-to-Forward (not marked
 Unsafe), the column is filled with a dash for UnSafe options.

 +-----+---+---+---+---+----------------+--------+--------+------------+
 | No. | C | U | N | R | Name           | Format | Length | Default    |
 +-----+---+---+---+---+----------------+--------+--------+------------+
 |   1 | x |   |   | x | If-Match       | opaque | 0-8    | (none)     |
 |   3 | x | x | - |   | Uri-Host       | string | 1-255  | (see below)|
 |   4 |   |   |   | x | ETag           | opaque | 1-8    | (none)     |
 |   5 | x |   |   |   | If-None-Match  | empty  | 0      | (none)     |
 |   7 | x | x | - |   | Uri-Port       | uint   | 0-2    | (see below)|
 |   8 |   |   |   | x | Location-Path  | string | 0-255  | (none)     |
 |  11 | x | x | - | x | Uri-Path       | string | 0-255  | (none)     |
 |  12 |   |   |   |   | Content-Format | uint   | 0-2    | (none)     |
 |  14 |   | x | - |   | Max-Age        | uint   | 0-4    | 60         |
 |  15 | x | x | - | x | Uri-Query      | string | 0-255  | (none)     |
 |  17 | x |   |   |   | Accept         | uint   | 0-2    | (none)     |
 |  20 |   |   |   | x | Location-Query | string | 0-255  | (none)     |
 |  35 | x | x | - |   | Proxy-Uri      | string | 1-1034 | (none)     |
 |  39 | x | x | - |   | Proxy-Scheme   | string | 1-255  | (none)     |
 |  60 |   |   | x |   | Size1          | uint   | 0-4    | (none)     |
 +-----+---+---+---+---+----------------+--------+--------+------------+

 draft-ietf-core-block-15
 +-----+---+---+---+---+--------+--------+--------+---------+
 | No. | C | U | N | R | Name   | Format | Length | Default |
 +-----+---+---+---+---+--------+--------+--------+---------+
 |  23 | C | U | - | - | Block2 | uint   |    0-3 | (none)  |
 |  27 | C | U | - | - | Block1 | uint   |    0-3 | (none)  |
 |  28 |   |   | x |   | Size2  | uint   |    0-4 | (none)  |
 +-----+---+---+---+---+--------+--------+--------+---------+

 +-----+---+---+---+---+---------+--------+--------+---------+
 | No. | C | U | N | R | Name    | Format | Length | Default |
 +-----+---+---+---+---+---------+--------+--------+---------+
 |   6 |   | x | - |   | Observe | uint   | 0-3 B  | (none)  |
 +-----+---+---+---+---+---------+--------+--------+---------+
 C=Critical, U=Unsafe, N=NoCacheKey, R=Repeatable
 */

typedef enum {
	// RFC 7252 core
	kKprCoAPMessageOptionIfMatch		 =  1,
	kKprCoAPMessageOptionUriHost		 =  3,
	kKprCoAPMessageOptionETag			 =  4,
	kKprCoAPMessageOptionIfNoneMatch	 =  5,
	kKprCoAPMessageOptionUriPort		 =  7,
	kKprCoAPMessageOptionLocationPath	 =  8,
	kKprCoAPMessageOptionUriPath		 = 11,
	kKprCoAPMessageOptionContentFormat	 = 12,
	kKprCoAPMessageOptionMaxAge			 = 14,
	kKprCoAPMessageOptionUriQuery		 = 15,
	kKprCoAPMessageOptionAccept			 = 17,
	kKprCoAPMessageOptionLocationQuery	 = 20,
	kKprCoAPMessageOptionProxyUri		 = 35,
	kKprCoAPMessageOptionProxyScheme	 = 39,
	kKprCoAPMessageOptionSize1			 = 60,

	// draft-ietf-core-block-15
	kKprCoAPMessageOptionBlock2			 = 23,
	kKprCoAPMessageOptionBlock1			 = 27,
	kKprCoAPMessageOptionSize2			 = 28,

	// draft-ietf-core-observe-14
	kKprCoAPMessageOptionObserve		 =  6,
} KprCoAPMessageOption;

typedef enum {
	kKprCoAPMessageOptionFormatEmpty,
	kKprCoAPMessageOptionFormatOpaque,
	kKprCoAPMessageOptionFormatUint,
	kKprCoAPMessageOptionFormatString,
} KprCoAPMessageOptionFormat;

typedef enum {
	kKprCoAPMessageObserveRegister = 0,
	kKprCoAPMessageObserveDeregister = 1,
} KprCoAPMessageObserveValue;


#define kKprCoAP_ACK_TIMEOUT 2  /* sec */
#define kKprCoAP_MAX_LATENCY 100 /* sec */
#define kKprCoAP_ACK_RANDOM_FACTOR 1.5
#define kKprCoAP_MAX_RETRANSMIT 4
#define kKprCoAP_NSTART 1
#define kKprCoAP_DEFAULT_LEISURE 5 /* sec */
#define kKprCoAP_PROBING_RATE 1 /* byte/second */

//--------------------------------------------------
// String Map Utility
//--------------------------------------------------

typedef struct KprCoAPStringMapEntry {
	int value;
	const char *str;
} KprCoAPStringMapEntry;

typedef struct KprCoAPStringMap {
	KprCoAPStringMapEntry *entries;
	int count;
} KprCoAPStringMap;

#define KprCoAPStringMapDefine(x, y) KprCoAPStringMap x = { y, sizeof(y) / sizeof(KprCoAPStringMapEntry) }

typedef Boolean (*KprCoAPStringMapTask)(int index, int value, const char *str, void *refcon1, void *refcon2);

Boolean KprCoAPStringMapForEach(KprCoAPStringMap *map, KprCoAPStringMapTask task, void *refcon1, void *refcon2);
FskErr KprCoAPStringMapFindMatchingStr(KprCoAPStringMap *map, const char *str, int *it);
FskErr KprCoAPStringMapFindMatchingValue(KprCoAPStringMap *map, int value, const char **it);

//--------------------------------------------------
// Methods
//--------------------------------------------------

FskErr KprCoAPMessageTypeToString(int type, const char **it);

FskErr KprCoAPMethodFromString(const char *str, int *method);
FskErr KprCoAPMethodToString(int method, const char **it);

FskErr KprCoAPContentFormatFromString(const char *str, int *format);
FskErr KprCoAPContentFormatToString(int format, const char **it);

FskErr KprCoAPMessageOptionFromString(const char *str, int *option);
FskErr KprCoAPMessageOptionToString(int option, const char **it);

FskErr KprCoAPStrNewWithLength(const char *src, int length, char **it);

double KprCoAPTimeToDouble(FskTime time);
double KprCoAPFromNow(FskTime time);
double KprCoAPGetNow();

//--------------------------------------------------
// Message Queue
//--------------------------------------------------

typedef struct KprCoAPMessageQueueRecord KprCoAPMessageQueueRecord, *KprCoAPMessageQueue;

struct KprCoAPMessageQueueRecord {
	KprCoAPMessageQueue next;
	KprCoAPMessage message;
};

FskErr KprCoAPMessageQueueAppend(KprCoAPMessageQueue *queue, KprCoAPMessage message);
FskErr KprCoAPMessageQueueAppendWithSize(KprCoAPMessageQueue *queue, KprCoAPMessage message, size_t recordSize, KprCoAPMessageQueue *it);
FskErr KprCoAPMessageQueueRemove(KprCoAPMessageQueue *queue, KprCoAPMessage message);
FskErr KprCoAPMessageQueueDispose(KprCoAPMessageQueue queue);
KprCoAPMessageQueue KprCoAPMessageQueueFind(KprCoAPMessageQueue queue, KprCoAPMessage message);

#endif
