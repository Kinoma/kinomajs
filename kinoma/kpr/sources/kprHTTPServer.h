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
#ifndef __KPRHTTPSERVER__
#define __KPRHTTPSERVER__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

FskErr KprHTTPServerNew(KprHTTPServer* it, char* authority, char* path, UInt32 preferredPort);
FskAPI(void) KprHTTPServerDispose(KprHTTPServer self);
FskAPI(KprHTTPServer) KprHTTPServerGet(char* id);
FskAPI(UInt32) KprHTTPServerGetPort(KprHTTPServer self);
FskAPI(UInt32) KprHTTPServerGetTimeout(KprHTTPServer self);
FskAPI(void) KprHTTPServerSetTimeout(KprHTTPServer self, UInt32 timeout);
FskAPI(FskErr) KprHTTPServerStart(KprHTTPServer self);
FskAPI(FskErr) KprHTTPServerStop(KprHTTPServer self, Boolean flush);

FskAPI(void) KprHTTPTargetMessageSetResponseProtocol(KprMessage message, char* protocol);

FskAPI(void) KprNetworkInterfaceActivate(Boolean activateIt);
FskAPI(void) KprNetworkInterfaceCleanup();
FskAPI(void) KprNetworkInterfaceSetup();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
