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
#ifndef __FSKRESOLVER_H__
#define __FSKRESOLVER_H__

#include "Fsk.h"
#include "FskNetUtils.h"
#include "FskThread.h"

#ifdef __cplusplus
extern "C" {
#endif

// hostname resolution
#ifndef __FSKNETUTILS_PRIV__
	FskDeclarePrivateType(FskResolver)

	typedef void (*FskResolverCallbackFn)(FskResolver rr);
#else
	struct FskResolverRecord;
	typedef void (*FskResolverCallbackFn)(struct FskResolverRecord *rr);

	typedef struct FskResolverRecord {
		struct FskResolverRecord     *next;
		UInt32                  resolvedIP;
		UInt16			targetPort;
		Boolean                 cancelled;
		void                    *ref;
		FskErr					err;
		FskResolverCallbackFn   cb;
		FskThread				callee;
		long			queryType;		/* see FskNetUtils.h */

		FskInstrumentedItemDeclaration
		char                    name[1];
	} FskResolverRecord, *FskResolver;
#endif

FskAPI(void) FskAsyncResolverInitialize(void);
FskAPI(void) FskAsyncResolverTerminate(void);

FskAPI(FskErr) FskNetHostnameResolveQTAsync(char *hostname, long queryType, FskResolverCallbackFn onResolved, void *ref, FskResolver *rr);
#define FskNetHostnameResolveAsync(hostname, onResolved, ref, rr)	FskNetHostnameResolveQTAsync(hostname, 0L, onResolved, ref, rr)
FskAPI(FskErr) FskResolverCancel(FskResolver rr);
FskAPI(FskErr) FskResolverCancelByRef(void *ref);

FskAPI(FskErr) FskNetHostnameResolveQT(char *hostname, long queryType, UInt32 *addr, UInt16 *port);
#define FskNetHostnameResolve(hostname, addr)	FskNetHostnameResolveQT(hostname, 0L, (UInt32*)(void*)(addr), NULL)

FskAPI(FskErr) FskNetServerSelection(const char *dname, char **srvName, UInt16 *targetPort);

FskAPI(FskErr) FskResolverDispose(FskResolver rr);

#if SUPPORT_INSTRUMENTATION
enum {
	kFskResolverInstrMsgResolutionRequest = kFskInstrumentedItemFirstCustomMessage,
	kFskResolverInstrMsgCancelRequest,
	kFskResolverInstrMsgCanceled,
	kFskResolverInstrMsgResolved,
};
#endif

#ifdef __cplusplus
}
#endif

#endif // __NET_UTILS_H__

