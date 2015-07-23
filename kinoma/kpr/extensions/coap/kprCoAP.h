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
#ifndef __KPRCOAP__
#define __KPRCOAP__

#include "kpr.h"
#include "kprCoAPClient.h"
#include "kprCoAPServer.h"

//--------------------------------------------------
// Typedefs
//--------------------------------------------------

typedef struct KPR_CoAP_ClientHostRecord KPR_CoAP_ClientHostRecord, *KPR_CoAP_ClientHostData;
typedef struct KPR_CoAP_ServerHostRecord KPR_CoAP_ServerHostRecord, *KPR_CoAP_ServerHostData;
typedef struct KPR_CoAP_RequestHostRecord KPR_CoAP_RequestHostRecord, *KPR_CoAP_RequestHostData;

struct KPR_CoAP_ClientHostRecord {
	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	KprCoAPClient client;
	KPR_CoAP_RequestHostData requests;
};

struct KPR_CoAP_ServerHostRecord {
	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	KprCoAPServer server;

	xsSlot resources;
};

struct KPR_CoAP_RequestHostRecord {
	KPR_CoAP_RequestHostData next;

	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	KprCoAPMessage request;
	KPR_CoAP_ClientHostData client; // reference only
};

#endif
