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
#ifndef __FSK_CURSOR_H__
#define __FSK_CURSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FskCursorRecord {
    struct FskCursorRecord *next;
    UInt32      cursorID; 
    UInt32      pixelFormat;
    SInt16      hotspotX;
    SInt16      hotspotY;
    UInt16      width;
    UInt16      height;
    UInt16      pad;
    FskBitmap   bits;
} FskCursorRecord, *FskCursor;

FskAPI(FskErr) FskCursorInitialize();
FskAPI(FskErr) FskCursorTerminate();
FskAPI(void) FskCursorShow(FskRectangle obscure);
FskAPI(void) FskCursorHide(FskRectangle obscure);
FskAPI(FskErr) FskCursorSetShape(UInt32 shape);
FskAPI(FskCursor) FskCursorFind(UInt32 id);


#define PIN(x,minx,maxx)	((x>maxx)?x=maxx : (x<minx)?x=minx : x)

#ifdef __cplusplus
}
#endif

#endif // __FSK_CURSOR_H__

