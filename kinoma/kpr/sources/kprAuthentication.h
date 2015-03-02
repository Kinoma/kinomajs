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
#ifndef __KPRAuthentication__
#define __KPRAuthentication__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

FskAPI(FskErr) KprAuthenticationDigestMD5(char* username, UInt32 usernameSize, char* password, UInt32 passwordSize, char* uri, UInt32 uriSize, char* challenge, char** response, char** verification);
FskAPI(FskErr) KprAuthenticationSCRAMSHA1Message(const char* username, UInt32 usernameSize, char** message);
FskAPI(FskErr) KprAuthenticationSCRAMSHA1MessageToken(const char* message, UInt32 messageSize, char** messageToken);
FskAPI(FskErr) KprAuthenticationSCRAMSHA1Response(const char *value, UInt32 valueSize, const char* message, UInt32 messageSize, const char* password, UInt32 passwordSize, char** response, char** verifier);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
