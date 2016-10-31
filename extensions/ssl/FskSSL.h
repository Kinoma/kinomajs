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
#ifndef __FSKSSL_H__
#define __FSKSSL_H__

#ifdef __cplusplus
extern "C" {
#endif

FskAPI(FskErr) FskSSLInitialize(char *CA_List);
FskAPI(void) FskSSLTerminate(void);

#if CLOSED_SSL
#include "FskNetUtils.h"

FskAPI (FskErr) FskSSLNew(void **fsslp, const char *host, int port, Boolean blocking, long flags, int priority);
FskAPI (FskErr) FskSSLNewWithOption(void **fsslp, FskSSLOption *option);
FskAPI (FskErr) FskSSLAttach(void **fsslp, FskSocket skt);
FskAPI (void) FskSSLDispose(void *fssl);
FskAPI (FskErr) FskSSLLoadCerts(void *fssl, FskSocketCertificateRecord *cert);
FskAPI (FskErr) FskSSLHandshake(void *fssl, FskNetSocketCreatedCallback callback, void *refCon, Boolean initiate, int timeout);
FskAPI (FskErr) FskSSLRead(void *fssl, void *buf, int *bufLen);
FskAPI (FskErr) FskSSLWrite(void *fssl, const void *buf, int *bufLen);
FskAPI (int) FskSSLGetBytesAvailable(void *fssl);
FskAPI (FskErr) FskSSLClose(void *fssl);
FskAPI (FskErr) FskSSLFlush(void *a);
FskAPI (int) FskSSLGetBytesAvailable(void *a);
#endif

#ifdef __cplusplus
}
#endif

#endif // __FSKSSL_H__
