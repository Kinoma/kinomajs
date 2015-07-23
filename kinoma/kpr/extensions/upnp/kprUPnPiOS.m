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
#include "FskCocoaSupportPhone.h"

#include "kpr.h"
#include "kpr_iOS.h"

static BOOL gUPnPNeedPositionUpdate;

void KprUPnPControllerServiceActionCallbackPlay_iOS(KprUPnPController self, const char* serviceType)
{
	// NSLog(@"========= START PLAYING ============");
	
	FskCocoaAudioSessionSetupFakePlaying();

	KprToneGeneratorPlay();

	KprSystemNowPlayingInfoSetIdling(true);
}

void KprUPnPControllerServiceActionCallbackStop_iOS(KprUPnPController self, const char* serviceType)
{
	// NSLog(@"========= STOP PLAYING ============");

	KprToneGeneratorStop();

	KprSystemNowPlayingInfoSetIdling(false);

	FskCocoaAudioSessionTearDown();
}

void KprUPnPControllerServiceActionCallbackGetPositionInfo_iOS(KprUPnPController self, const char* serviceType, double duration, double position)
{
	if (gUPnPNeedPositionUpdate) {
		KprSystemNowPlayingInfoSetTime(duration, position);
		gUPnPNeedPositionUpdate = NO;
	}
}

void KprUPnPControllerServiceActionCallbackSeek_iOS(KprUPnPController self, const char* serviceType)
{
	gUPnPNeedPositionUpdate = YES;
}

void KprUPnPControllerServiceActionCallbackOther_iOS(KprUPnPController self, const char* serviceType, const char *actionName)
{
	// NSLog(@"========= OTHER: %s ============", actionName);
}

void KprUPnPControllerServiceActionCallbackError_iOS(KprUPnPController self, const char* serviceType, const char *actionName, SInt32 errorCode, const char* errorDescription)
{
	// NSLog(@"========= ERROR: %s %ld %s ============", actionName, errorCode, errorDescription);
}

void KprUPnPControllerGotMetadata_iOS(KprUPnPMetadata metadata)
{
	gUPnPNeedPositionUpdate = YES;

	KprSystemNowPlayingInfoSetUPnPMetadata(metadata);
}

void KprUPnPControllerUtility_iOS(KprUPnPController self, char* actionName)
{
	if (FskStrCompare(actionName, "finalize") == 0) {
		KprToneGeneratorStop();

		KprSystemNowPlayingInfoSetIdling(false);
		KprSystemNowPlayingInfoSetUPnPMetadata(NULL);

		FskCocoaAudioSessionTearDown();
	}
}


void UPnP_Controller_isBackgroundPlaying(xsMachine *the)
{	
	if (KprSystemNowPlayingInfoGetIdling())
		xsResult = xsTrue;
	else
		xsResult = xsFalse;
}
