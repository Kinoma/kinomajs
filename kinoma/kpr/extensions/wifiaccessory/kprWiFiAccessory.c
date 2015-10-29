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
#include "xs.h"
#include "FskExtensions.h"
#include "kprWiFiAccessory.h"

void KPR_WiFiAccessory_Browser(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprWiFiAccessoryBrowser self = NULL;

	bailIfError(KprWiFiAccessoryBrowserNew(&self, the, the->code, xsThis));

	if (!xsTest(xsArg(0))) {
		xsNewHostProperty(xsThis, xsID_behavior, xsThis, xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	} else {
		xsNewHostProperty(xsThis, xsID_behavior, xsArg(0), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	}

	xsSetHostData(xsThis, self);

bail:
	if (err) KprWiFiAccessoryBrowserDispose(self);
	xsThrowIfFskErr(err);
}

void KPR_WiFiAccessory_browser_destructor(void *it)
{
	KprWiFiAccessoryBrowserDispose(it);
}

void KPR_WiFiAccessory_browser_get_accessories(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprWiFiAccessoryBrowser browser = xsGetHostData(xsThis);

	bailIfError(KprWiFiAccessoryBrowserGetAccessories(browser, the));

bail:
	xsThrowIfFskErr(err);
}
void KPR_WiFiAccessory_browser_start(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprWiFiAccessoryBrowser browser = xsGetHostData(xsThis);

	bailIfError(KprWiFiAccessoryBrowserStart(browser));

bail:
	xsThrowIfFskErr(err);
}

void KPR_WiFiAccessory_browser_stop(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprWiFiAccessoryBrowser browser = xsGetHostData(xsThis);

	bailIfError(KprWiFiAccessoryBrowserStop(browser));

bail:
	xsThrowIfFskErr(err);
}


void KPR_WiFiAccessory_accessory_destructor(void *it)
{
	KprWiFiAccessoryDispose(it);
}

void KPR_WiFiAccessory_accessory_get_name(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprWiFiAccessoryBrowser browser = xsGetHostData(xsThis);

	bailIfError(KprWiFiAccessoryGetName(browser, the));

bail:
	xsThrowIfFskErr(err);
}
void KPR_WiFiAccessory_accessory_configure(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KprWiFiAccessoryBrowser browser = xsGetHostData(xsThis);

	bailIfError(KprWiFiAccessoryConfigure(browser, xsArg(0)));

bail:
	xsThrowIfFskErr(err);

}

FskErr kprWiFiAccessory_fskLoad(FskLibrary library)
{
	return kFskErrNone;
}
FskErr kprWiFiAccessory_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}
