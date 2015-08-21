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
#include "FskExtensions.h"

#define __FSKBITMAP_PRIV__
#define __FSKPORT_PRIV__

#include "kprBehavior.h"
#include "kprContent.h"
#include "kprEffect.h"
#include "kprWifiManager.h"
#include "kprMessage.h"
#include "kprSkin.h"
#include "kprURL.h"
#include "kprShell.h"
#include "kprUtilities.h"
#include "FskHardware.h"

#include "FskPlatformImplementation.h"
FskInstrumentedSimpleType(kprWifiManager, kprWifiManager);
#define mlog  FskkprWifiManagerPrintfMinimal
#define nlog  FskkprWifiManagerPrintfNormal
#define vlog  FskkprWifiManagerPrintfVerbose
#define dlog  FskkprWifiManagerPrintfDebug

typedef struct
{
 	xsMachine*  the;
 	xsSlot      slot;
    int         data;
	xsIndex*    code;
    int         netId;
}KprWifiManager;

FskExport(FskErr) kprWifiManager_fskLoad(FskLibrary library)
{
	return kFskErrNone;
}


FskExport(FskErr) kprWifiManager_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

void KprWifiManagerDispose( KprWifiManager *session )
{
    dlog(">>> KprWifiManagerDispose");
    
    FskMemPtrDispose(session);
    
    dlog("<<< KprWifiManagerDispose");
}

void KPR_WifiManager_URL(xsMachine *the)
{
	KprWifiManager *self = NULL;
    FskErr err = kFskErrNone;
    
	err = FskMemPtrNewClear(sizeof(KprWifiManager), &self);
	BAIL_IF_ERR( err );

    dlog(">>> KPR_WifiManager_URL: self = 0x%x", (unsigned)self);
	xsSetHostData(xsResult, self);
    
	self->the = the;
	self->slot = xsResult;
    self->code = the->code;
    self->netId = gAndroidCallbacks->androidWifiGetNetId();
	xsRemember(self->slot);
    
bail:    
	xsThrowIfFskErr(err);
    dlog("<<< KPR_WifiManager_URL");
}

void KPR_wifimanager_destructor(void *data)
{
    dlog(">>> KPR_wifimanager_destructor, data = %x", (unsigned)data);
    
    if (data) {
        KprWifiManager *self = (KprWifiManager *)data;
        int netId = gAndroidCallbacks->androidWifiGetNetId();
        if (self->netId != netId && self->netId >= 0) {
            gAndroidCallbacks->androidWifiSelectNetworkById(self->netId);
        }      
        
        FskMemPtrDispose(data);
    }
    
    dlog("<<< KPR_wifimanager_destructor");
}
    
void KPR_wifimanager_getNetId(xsMachine *the) 
{
    FskErr err = kFskErrNone;
    int netId;
    
    dlog(">>> KPR_wifimanager_getNetId");
    
	KprWifiManager *self = xsGetHostData(xsThis);
    netId = gAndroidCallbacks->androidWifiGetNetId();
    dlog("netId = %d", netId); 
    
    xsResult = xsInteger(netId);  
    
bail:
	xsThrowIfFskErr(err); 
    dlog("<<< KPR_wifimanager_getNetId");      
}

void KPR_wifimanager_getIpAddress(xsMachine *the) 
{
    FskErr err = kFskErrNone;
    char buf[64];
    unsigned ip;
    dlog(">>> KPR_wifimanager_getIpAddress");
    
	KprWifiManager *self = xsGetHostData(xsThis);
    ip = gAndroidCallbacks->androidWifiGetIpAddress();
    sprintf(buf, "%d.%d.%d.%d", (ip>>24) & 0xFF, (ip>>16) & 0xFF, (ip>>8) & 0xFF, (ip>>0) & 0xFF);
    dlog("ip = %s", buf); 
    
    xsResult = xsString(buf);
    
bail:
	xsThrowIfFskErr(err);  
    dlog("<<< KPR_wifimanager_getIpAddress");     
}

void KPR_wifimanager_getSsid(xsMachine *the) 
{
    FskErr err = kFskErrNone;
    char * ssid = "";
    dlog(">>> KPR_wifimanager_getSsid");
    
    KprWifiManager *self = xsGetHostData(xsThis);
    ssid = gAndroidCallbacks->androidWifiGetSsid();
    if (ssid) {
        xsResult = xsString(ssid);
        FskMemPtrDispose(ssid);
    }
    else {
		xsResult = xsString("");
    }
    
bail:
	xsThrowIfFskErr(err);  
    dlog("<<< KPR_wifimanager_getSsid");     
}

void KPR_wifimanager_updateNetwork(xsMachine *the) 
{
    FskErr err = kFskErrNone;
    char buf[128];
    int preNetId = -1;
    char * preSsid = "";
    
    dlog(">>> KPR_wifimanager_updateNetwork");
      
	KprWifiManager *self = xsGetHostData(xsThis);
    char *ssid = xsToString(xsArg(0));    
	void *key = xsToString(xsArg(1));    
    int security = xsToInteger(xsArg(2));    
    
    dlog("KPR_wifimanager_updateNetwork: Update to \"ssid: %s, key: %s, security: %d\"",
                    ssid, key, security);
    preSsid = gAndroidCallbacks->androidWifiGetSsid();
    preNetId = gAndroidCallbacks->androidWifiUpdateNetwork(ssid, key);
    
    //ssid is surrounded by double quatation marks(not ture for all android phones)
    if (preSsid) {
        if (preSsid[0] == '\"')
            sprintf(buf, "{\"ssid\": %s, \"netId\": %d}", preSsid, preNetId);
        else
            sprintf(buf, "{\"ssid\": \"%s\", \"netId\": %d}", preSsid, preNetId);
        FskMemPtrDispose(preSsid);
    } else {
        sprintf(buf, "{\"ssid\": \"%s\", \"netId\": %d}", "", preNetId);
    }
    dlog("KPR_wifimanager_updateNetwork-Pre info: %s", buf);
    xsResult = xsString(buf);
    
bail:
	xsThrowIfFskErr(err); 
    dlog("<<< KPR_wifimanager_updateNetwork");      
}

void KPR_wifimanager_selectNetworkById(xsMachine *the) 
{
    FskErr err = kFskErrNone;
    
	KprWifiManager *self = xsGetHostData(xsThis);
    int netId = xsToInteger(xsArg(0));
    dlog(">>> KPR_wifimanager_selectNetworkById: %d", netId);
    
    gAndroidCallbacks->androidWifiSelectNetworkById(netId);
    
bail:
	xsThrowIfFskErr(err); 
    dlog("<<< KPR_wifimanager_selectNetworkById");   
}

void KPR_wifimanager_getScanResults(xsMachine *the) 
{
    FskErr err = kFskErrNone;
    char * scanResults = "";
    
    KprWifiManager *self = xsGetHostData(xsThis);
    scanResults = gAndroidCallbacks->androidWifiGetScanResults();
    dlog(">>> KPR_wifimanager_getScanResults: %s", scanResults);
    if (scanResults) {
        xsResult = xsString(scanResults);
        FskMemPtrDispose(scanResults);
    }
	else {
		xsResult = xsString("");
    }
    
bail:
    xsThrowIfFskErr(err);  
    dlog("<<< KPR_wifimanager_getScanResults");     
}

