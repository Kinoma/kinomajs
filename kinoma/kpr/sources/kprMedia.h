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
#ifndef __KPRMEDIA__
#define __KPRMEDIA__

#include "FskMediaPlayer.h"
#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum {
	kprMediaStatePaused = 0,
	kprMediaStatePlaying = 1,
	kprMediaStateFailed = 2,
	kprMediaStateWaiting = 3
};

typedef FskErr (*MediaPlayerSetWindowHook)(FskMediaPlayer player, FskWindow window, UInt32 drawingMethod);

typedef struct {
	KprSlotPart;
	KprContentPart;
	FskMediaPlayer mp;
	char* url;
	double progress;
	double seekableFrom;
	double seekableTo;
	char* title;
	char* artist;
	char* album;
	FskBitmap cover;
	KprMessage message;
	UInt32 bitRate;
    UInt32 accumulatedInterval;
    UInt32 scale;
    MediaPlayerSetWindowHook setWindowHook;
} KprMediaRecord, *KprMedia;

FskAPI(FskErr) KprMediaNew(KprMedia *self, KprCoordinates coordinates);
FskAPI(Boolean) KprMediaGetSeeking(KprMedia self);
FskAPI(void) KprMediaGetVolume(KprMedia self, float* volume);
FskAPI(void) KprMediaSetSeeking(KprMedia self, Boolean seekIt);
FskAPI(void) KprMediaSeekBy(KprMedia self, double fraction);
FskAPI(void) KprMediaSetTime(KprMedia self, double fraction);
FskAPI(void) KprMediaSetURL(KprMedia self, char* url, char* mime);
FskAPI(void) KprMediaSetVolume(KprMedia self, float volume);
FskAPI(void) KprMediaStart(KprMedia self);
FskAPI(void) KprMediaStop(KprMedia self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
