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
#ifndef __FSKCOCOASUPPORTCOMMON__
#define __FSKCOCOASUPPORTCOMMON__

#include "FskAudio.h"
#include "FskBitmap.h"
#include "FskText.h"

    // bitmap
Boolean FskCocoaBitmapUseGL(void);

	// text
void FskCocoaTextInitialize(void);
void FskCocoaTextUninitialize(void);
FskErr FskCocoaTextNew(void **state);
FskErr FskCocoaTextDispose(void *state);
void FskCocoaTextGetFontList(void *state, char **fontList);
void FskCocoaTextGetBounds(void *state, const char *text, UInt32 textLength, UInt32 textSize, UInt32 textStyle, const char *fontName, FskRectangle bounds, FskTextFormatCache cache);
Boolean FskCocoaTextFitWidth(void *state, const char *text, UInt32 textLength, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt32 width, UInt32 flags, UInt32 *fitBytesOut, UInt32 *fitCharsOut, FskTextFormatCache cache);
void FskCocoaTextGetFontInfo(void *state, FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache cache);
Boolean FskCocoaTextFormatCacheNew(void *state, FskTextFormatCache *cache, FskBitmap bits, UInt32 textSize, UInt32 textStyle, const char *fontName);
void FskCocoaTextFormatCacheDispose(void *state, FskTextFormatCache cache);
Boolean FskCocoaTextDraw(void *state, FskBitmap fskBitmap, const char *text, UInt32 textLength, FskConstRectangle fskRect, FskConstRectangle clipFskRect, FskConstColorRGBA fskColorRGB, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, UInt16 horizontalAlignment, UInt16 verticalAlignment, const char *fontName, FskTextFormatCache cache);
void FskCocoaTextGetLayout(void *state, const char *text, UInt32 textLength, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt16 **unicodeTextPtr, UInt32 *unicodeLenPtr, FskFixed **layoutPtr, FskTextFormatCache cache);
void FskCocoaTextGetGlyphs(void *state, const char *text, UInt32 textLength, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt16 **glyphsPtr, UInt32 *glyphsLenPtr, FskFixed **layoutPtr, float *widthPtr, float *heightPtr, FskTextFormatCache cache);

// audio
#if TARGET_OS_IPHONE
Boolean FskCocoaAudioSessionSetupRecording(Boolean startRecord);
Boolean FskCocoaAudioSessionSetupPlaying(enum FskAudioOutCategory category);
Boolean FskCocoaAudioSessionSetupFakePlaying();
void FskCocoaAudioSessionTearDown();
#endif
Boolean FskCocoaAudioInitialize(FskAudioOut fskAudioOut);
#if USE_AUDIO_QUEUE
Boolean FskCocoaAudioEnqueueBuffers(FskAudioOut fskAudioOut);
Boolean FskCocoaAudioSetFormat(FskAudioOut audioOut, UInt32 format, UInt16 numChannels, double sampleRate, unsigned char *formatInfo, UInt32 formatInfoSize);
void FskCocoaAudioHasProperty(FskAudioOut audioOut, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
Boolean FskCocoaAudioSetProperty(FskAudioOut audioOut, UInt32 propertyID, FskMediaPropertyValue property);
Boolean FskCocoaAudioGetProperty(FskAudioOut audioOut, UInt32 propertyID, FskMediaPropertyValue property);
#endif
void FskCocoaAudioTerminate(FskAudioOut fskAudioOut);
void FskCocoaAudioGetVolume(FskAudioOut fskAudioOut, UInt16 *leftVolume, UInt16 *rightVolume);
void FskCocoaAudioSetVolume(FskAudioOut fskAudioOut, UInt16 leftVolume, UInt16 rightVolume);
Boolean FskCocoaAudioStart(FskAudioOut fskAudioOut);
void FskCocoaAudioStop(FskAudioOut fskAudioOut);
void FskCocoaAudioGetSamplePosition(FskAudioOut fskAudioOut, FskSampleTime *fskSampleTime);

// file system
char *FskCocoaGetSpecialPath(UInt32 type, const Boolean create);
const char *FskCocoaDisplayNameAtPath(const char *path);

#endif
