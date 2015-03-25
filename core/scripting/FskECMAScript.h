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
#ifndef __FSKECMASCRIPT__
#define __FSKECMASCRIPT__

#include "Fsk.h"

#include "xs.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__FSKECMASCRIPT_PRIV__) && !SUPPORT_INSTRUMENTATION
	FskDeclarePrivateType(FskECMAScript)
#else
	#include "FskExtensions.h"
	#include "FskThread.h"

	struct FskECMAScriptLibraryRecord {
		struct FskECMAScriptLibraryRecord	*next;
		FskLibrary							library;
		void								*byteCode;
		char								*name;
		char								path[1];
	};

	typedef struct FskECMAScriptLibraryRecord FskECMAScriptLibraryRecord;
	typedef FskECMAScriptLibraryRecord *FskECMAScriptLibrary;

	struct FskECMAScriptRecord {
		struct FskECMAScriptRecord	*next;

		xsMachine				*the;

		void					*context;
		void					*refcon;
		FskECMAScriptLibrary	libraries;
		FskThread				thread;
		UInt32					id;
		char					*name;
		Boolean					quitting;

		FskInstrumentedItemDeclaration

		xsIndex				id_x;
		xsIndex				id_y;
		xsIndex				id_width;
		xsIndex				id_height;
		xsIndex				id_left;
		xsIndex				id_right;
		xsIndex				id_top;
		xsIndex				id_bottom;
		xsIndex				id_r;
		xsIndex				id_g;
		xsIndex				id_b;
		xsIndex				id_a;
		xsIndex				id_Rectangle;
		xsIndex				id_Color;
		xsIndex				id_RGB;
		xsIndex				id_Point;
		xsIndex				id_Event;
		xsIndex				id_onCallback;

		xsIndex				id_length;
		xsIndex				id_inherit;
		xsIndex				id_prototype;
		xsIndex				id_buffer;
		xsIndex				id_data;
		xsIndex				id_close;
		xsIndex				id_Chunk;
		xsIndex				id_chunk;
		xsIndex				id_readData;
		xsIndex				id_writeData;
		xsIndex				id_stream;
		xsIndex				id_readChunk;
		xsIndex				id_free;

		xsIndex				id_target;
		xsIndex				id_FskUI;
		xsIndex				id_rgb;

		xsIndex				id_Bitmap;
		xsIndex				id_events;
		xsIndex				id_FileSystem;
		xsIndex				id_moreFileInfo;
		xsIndex				id_size;
		xsIndex				id_type;
		xsIndex				id_Date;
		xsIndex				id_creationDate;
		xsIndex				id_modificationDate;
		xsIndex				id_hidden;
		xsIndex				id_locked;
		xsIndex				id_path;
		xsIndex				id_extension;
		xsIndex				id_mime;
		xsIndex				id_filename;
		xsIndex				id_compositionTime;
		
		xsIndex				id_time;
		xsIndex				id_index;

		xsIndex				id_value;
	};

	typedef struct FskECMAScriptRecord FskECMAScriptRecord;
	typedef FskECMAScriptRecord *FskECMAScript;
#endif

#ifdef __FSKECMASCRIPT_PRIV__
	FskErr FskECMAScriptInitialize(const char *configPath);
	FskErr FskECMAScriptInitializationComplete(void);
	void FskECMAScriptTerminate(void);
	void FskECMAScriptHibernate(void);
#endif

FskAPI(FskErr) FskECMAScriptNew(FskECMAScript *vm, const char *name, xsAllocation *alloc);
FskAPI(FskErr) FskECMAScriptDispose(FskECMAScript vm);

enum {
	kFskECMAScriptKeepSource = 1
};

FskAPI(FskErr) FskECMAScriptFindByName(const char *name, FskECMAScript *vm);

FskAPI(FskErr) FskECMAScriptLoadFromFile(FskECMAScript vm, const char *fullPath, UInt32 flags, xsSlot *result);
FskErr FskECMAScriptLoadFromMemory(FskECMAScript vm, const char *script, UInt32 scriptLen, UInt32 flags, xsSlot *result);

FskErr FskECMAScriptLinkFromFile(FskECMAScript vm, const char *path);
FskErr FskECMAScriptLoadLibrary(FskECMAScript vm, const char *path);

FskAPI(xsMachine *) FskECMAScriptGetXSMachine(FskECMAScript vm);

FskAPI(FskErr) FskECMAScriptSetContext(FskECMAScript vm, void *context);
FskAPI(void) *FskECMAScriptGetContext(FskECMAScript vm);

FskAPI(FskErr) FskECMAScriptSetRefcon(FskECMAScript vm, void *refcon);
FskAPI(void) *FskECMAScriptGetRefcon(FskECMAScript vm);

FskAPI(void) FskECMAScriptThrowIf(xsMachine *the, FskErr err) FSK_FUNCTION_ANALYZER_NORETURN;
FskAPI(void) FskECMAScriptQuitIf(xsMachine *the, FskErr err) FSK_FUNCTION_ANALYZER_NORETURN;


#ifdef __FSKECMASCRIPT_PRIV__
	// shared defines

	#include "FskBitmap.h"
	#include "FskEvent.h"
	#include "FskMediaPlayer.h"

	typedef struct {
		void			*next;
		FskBitmap		bitmap;
		Boolean			doCache;
		Boolean			loadable;
		Boolean			loadedBounds;
		xsMachine		*the;
		SInt32			width;
		SInt32			height;
		UInt32			seed;
	} xsNativeBitmapRecord, *xsNativeBitmap;

	typedef struct {
		UInt32			kind;
		FskMediaPlayer	player;
		xsSlot			obj;
		FskECMAScript	vm;
		char			*path;
	} xsNativeMediaPlayerRecord, *xsNativeMediaPlayer;

	typedef struct {
		UInt32				kind;
		xsNativeMediaPlayer	np;
		void				*track;
	} xsNativeMediaPlayerTrackRecord, *xsNativeMediaPlayerTrack;

	typedef struct {
		FskEvent		event;
		Boolean			needToDispose;
	} xsNativeEventRecord, *xsNativeEvent;

	FskAPI(FskErr) readStreamIntoMemory(xsMachine *the, xsSlot *stream, xsSlot *scratch, unsigned char **data, UInt32 *dataSize);
	
	FskAPI(FskBitmap) xsBitmapToFskBitmap(xsMachine *the, xsSlot *xsBits);
	FskAPI(void) fskBitmapToXSBitmap(xsMachine *the, FskBitmap fskBits, Boolean disposable, xsSlot *xsBits);
	FskAPI(void) xsBitmapEnsure(xsMachine *the, xsNativeBitmap nb, xsSlot *obj);

	FskAPI(void) xsRectangleToFskRectangle(xsMachine *the, xsSlot *ref, FskRectangle r);
	FskAPI(void) xsPointToFskPoint(xsMachine *the, xsSlot *ref, FskPoint p);
	FskAPI(void) fskRectangleToXSRectangle(xsMachine *the, FskConstRectangle r, xsSlot *ref);
	FskAPI(void) xsWindowGetRectangle(xsMachine *the, UInt32 index, Boolean fourIntegers, FskRectangle r);
	FskAPI(void) xsRGBColorToFskRGBColor(xsMachine *the, xsSlot *ref, FskColorRGB r);
	FskAPI(void) xsRGBColorToFskRGBAColor(xsMachine *the, xsSlot *ref, FskColorRGBA r);
	FskAPI(void) xsMemPtrToChunk(xsMachine *the, xsSlot *ref, FskMemPtr data, UInt32 dataSize, Boolean alreadyAllocated);

	#include "FskWindow.h"
	typedef struct {
		FskWindow		win;
		FskPort			port;
		xsSlot			obj;
		xsSlot			event;
		FskECMAScript	vm;

		Boolean			wrapped;
		Boolean			bilinearInterpolation;
		SInt32			blendLevel;
		UInt32			graphicsMode;

		xsIndex			id_hAlignment;
		xsIndex			id_vAlignment;
		xsIndex			id_size;
		xsIndex			id_style;
		xsIndex			id_color;
		xsIndex			id_font;
		xsIndex			id_truncate;
		xsIndex			id_cache;
		xsIndex			id_textFormat;
		xsIndex			id_onUpdate;
		xsIndex			id_onMouseDown;
		xsIndex			id_onMouseMoved;
		xsIndex			id_onMouseUp;
		xsIndex			id_onKeyDown;
		xsIndex			id_onKeyUp;

		xsSlot			eventObj;
	} xsNativeWindowRecord, *xsNativeWindow;

	#include "FskHTTPClient.h"
	typedef struct {
		FskHTTPClient	client;
		xsSlot			obj;
		FskECMAScript	vm;
		UInt32			useCount;
	} xsNativeHTTPClientRecord, *xsNativeHTTPClient;

	typedef struct {
		xsNativeHTTPClient		nht;
		FskHTTPClientRequest	req;
		xsSlot			obj;
		char			*postBuffer;
		Boolean			hasContentLength;
		Boolean			hasUserAgent;
		UInt8			stage;			// 0 = not yet added, 1 == added, 2 == complete
		Boolean			needsForget;
		FskECMAScript	vm;
	} xsNativeHTTPRequestRecord, *xsNativeHTTPRequest;

	typedef FskErr (*do_getMediaProperty_proc)(void *obj, UInt32 propertyID, FskMediaPropertyValue value);
	typedef FskErr (*do_hasMediaProperty_proc)(void *obj, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);

	void do_getMediaProperty(xsMachine *the, void *obj, void *getProc);
	void do_setMediaProperty(xsMachine *the, void *obj, void *hasProc, void *setProc);

	FskAPI(void) FskECMAScriptGetDebug(char **host, char **breakpoints, Boolean *keepSource);
	FskAPI(FskECMAScript) FskECMAScriptGetServerVM(void);		//@@ temporary
#endif

#ifdef __cplusplus
}
#endif

#endif
