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
#ifndef __KPRSSDPSERVER__
#define __KPRSSDPSERVER__

typedef struct KprSSDPServerStruct KprSSDPServerRecord, *KprSSDPServer;

typedef void (*KprSSDPServerRegisteredCallback)(KprSSDPServer self);
typedef void (*KprSSDPServerUnregisteredCallback)(KprSSDPServer self);

struct KprSSDPServerStruct {
	KprSSDPCommonPart;
	UInt32 expire;
	char* path;
	UInt32 port;
	char* uuid;
	KprSSDPServerRegisteredCallback registeredCallback;
	KprSSDPServerUnregisteredCallback unregisteredCallback;

	KprSSDPDevice device;
	FskInstrumentedItemDeclaration
};

FskErr KprSSDPServerNew(KprSSDPServer *it, UInt32 port, const char* path, UInt32 expire, const char* uuid, const char* type);
void KprSSDPServerDispose(KprSSDPServer self);
#define KprSSDPServerAddService(self, service) KprSSDPCommonAddService((KprSSDP)(self), service)
FskErr KprSSDPServerStart(KprSSDPServer self);
FskErr KprSSDPServerStop(KprSSDPServer self);

#endif
