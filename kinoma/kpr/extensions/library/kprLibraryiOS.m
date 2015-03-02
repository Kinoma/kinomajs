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
#define _XOPEN_SOURCE
#define __FSKHTTPSERVER_PRIV__
#include "FskAAScaleBitmap.h"
#include "FskEnvironment.h"
#include "FskExtensions.h"
#include "FskECMAScript.h"
#include "FskHTTPServer.h"
#include "FskImage.h"
#include "FskMediaPlayer.h"
#include "FskString.h"
#include "FskThread.h"
#include "FskMuxer.h"

#include "kpr.h"
#include "kprHTTPClient.h"
#include "kprImage.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprURL.h"
#include "kprUtilities.h"
#include "kprLibrary.h"
#include "kprLibraryServer.h"

#import <MediaPlayer/MediaPlayer.h>
#import <AssetsLibrary/AssetsLibrary.h>
#import <AVFoundation/AVFoundation.h>
#import <AVFoundation/AVAsset.h>
#import <AVFoundation/AVAssetExportSession.h>

struct KprLibraryStruct {
	xsMachine* the;
	ALAssetsLibrary* library;
	KprLibraryServer server;
};

static FskErr KprLibraryNew();
static void KprLibraryDispose();
static void KprLibraryCancel(KprService service, KprMessage message);
extern void KprLibraryInvoke(KprService service, KprMessage message);
extern void KprLibraryStart(KprService service, FskThread thread, xsMachine* the);
extern void KprLibraryStop(KprService service);

static char KprLibraryGetInitial(xsMachine* the, NSString *string);
static void KprLibrarySort(xsMachine* the, xsIntegerValue i);

static FskErr KprLibraryGetAlbums(xsMachine* the);
static FskErr KprLibraryGetArtists(xsMachine* the);
static FskErr KprLibraryGetArtistAlbums(xsMachine* the, char* artist);
static FskErr KprLibraryGetGenres(xsMachine* the);
static FskErr KprLibraryGetAlbumSongs(xsMachine* the, char* album);
static FskErr KprLibraryGetAlbumSongsOfArtist(xsMachine *the, char *album, char *artist);
static NSArray *KprLibraryFetchAlbumSongs(char* album);
static void KprLibraryBuildAlbumSongsArray(xsMachine* the, NSArray *items);
static FskErr KprLibraryGetArtistSongs(xsMachine* the, char* artist);
static FskErr KprLibraryGetGenreSongs(xsMachine* the, char* genre);
static FskErr KprLibraryGetSongs(xsMachine* the);
static FskErr KprLibraryGetSongsAux(xsMachine* the, NSArray *items);
static FskErr KprLibraryGetSongThumbnail(KprMessage message, char* song);
static FskErr KprLibraryGetMovies(KprLibrary library, KprMessage message);
static FskErr KprLibraryGetPictures(KprLibrary library, KprMessage message);
static FskErr KprLibrarySavePicture(KprLibrary library, KprMessage message);
static FskErr KprLibraryGetPictureThumbnail(KprLibrary library, KprMessage message, char* url);
static FskErr KprLibraryGetPictureInfo(KprLibrary library, KprMessage message, char* url);

static KprLibraryRecord gLibrary = { NULL, NULL, NULL };
KprServiceRecord gLibraryService = {
	NULL,
	0,
	"xkpr://library/",
	NULL,
	NULL,
	KprServiceAccept,
	KprLibraryCancel,
	KprLibraryInvoke,
	KprLibraryStart,
	KprLibraryStop,
	NULL,
	NULL,
	NULL
};
static FskThread gLibraryThread = NULL;
static Boolean gLibraryThreadQuitting = false;
static SInt32 gGeneration;

static void KprLibraryLoop(void* theParameter UNUSED)
{
	FskErr err = kFskErrNone;
	FskThreadInitializationComplete(FskThreadGetCurrent());
	bailIfError(KprLibraryNew());
	while (!gLibraryThreadQuitting)
		FskThreadRunloopCycle(-1);
	KprLibraryDispose();
bail:
	return;
}

FskExport(FskErr) kprLibrary_fskLoad(FskLibrary it)
{
	KprServiceRegister(&gLibraryService);
	return kFskErrNone;
}

FskExport(FskErr) kprLibrary_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

FskErr KprLibraryNew()
{
	xsAllocation allocation = {
		1100000,		// 700 * 1024,
		32 * 1024,
		70000,			// 62000,
		4096,
		2048,
		16000,
		1993
	};
	FskErr err = kFskErrNone;
	char* url = NULL;
	char* path = NULL;
	KprLibrary self = &gLibrary;
    char *generationStr = FskEnvironmentGet("Generation");
	
	self->the = xsAliasMachine(&allocation, gShell->root, "library", self);
	bailIfNULL(self->the);
	self->library = [[ALAssetsLibrary alloc] init];
	
	gLibraryService.machine = self->the;
	gLibraryService.thread = gLibraryThread;
	
    if (generationStr)
        gGeneration = FskStrToNum(generationStr);

	bailIfError(KprLibraryServerNew(&self->server));
bail:
	FskMemPtrDispose(path);
	FskMemPtrDispose(url);
	return err;
}

void KprLibraryDispose()
{
	KprLibrary self = &gLibrary;

	KprLibraryServerDispose(self->server);
	
	if (self->the)
		xsDeleteMachine(self->the);
		
	//KprLibraryServerStop(self);
}

void KprLibraryCancel(KprService service UNUSED, KprMessage message)
{
	FskInstrumentedItemPrintfNormal(message, "Library cancel %s", message->url);
}

void KprLibraryInvoke(KprService service UNUSED, KprMessage message)
{
	FskErr err = kFskErrNone;
	if (KprMessageContinue(message)) {
		KprLibrary self = &gLibrary;
		FskAssociativeArray query = NULL;
		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageLibraryBegin, message)
		if (message->parts.query) {
			bailIfError(KprQueryParse(message->parts.query, &query));
        }
		if (FskStrCompareWithLength("/server/", message->parts.path, 8) == 0) {
			if (FskStrCompareWithLength("start", message->parts.name, message->parts.nameLength) == 0) {
				char* value = FskAssociativeArrayElementGetString(query, "port");
				KprLibraryServerStart(self->server, value);
			}
			else if (FskStrCompareWithLength("stop", message->parts.name, message->parts.nameLength) == 0) {
				KprLibraryServerStop(self->server);
			}
		}
		else if (FskStrCompareWithLength("movies", message->parts.name, message->parts.nameLength) == 0) {
            bailIfError(KprLibraryGetMovies(self, message));
		}
		else if (FskStrCompareWithLength("/music/", message->parts.path, 7) == 0) {
			xsBeginHostSandboxCode(self->the, NULL);
			{
				xsVars(30);
				{
					xsTry {
						KprMessageScriptTarget target = (KprMessageScriptTarget)message->stream;
						xsVar(0) = xsNewInstanceOf(xsArrayPrototype);
						if (FskStrCompareWithLength("albums", message->parts.name, message->parts.nameLength) == 0) {
							char* value;
							if ((value = FskAssociativeArrayElementGetString(query, "artist")))
								xsThrowIfFskErr(KprLibraryGetArtistAlbums(the, value));
							else
								xsThrowIfFskErr(KprLibraryGetAlbums(the));
						}
						else if (FskStrCompareWithLength("artists", message->parts.name, message->parts.nameLength) == 0)
							xsThrowIfFskErr(KprLibraryGetArtists(the));
						else if (FskStrCompareWithLength("genres", message->parts.name, message->parts.nameLength) == 0)
							xsThrowIfFskErr(KprLibraryGetGenres(the));
						else if (FskStrCompareWithLength("songs", message->parts.name, message->parts.nameLength) == 0) {
							char* value;
							if ((value = FskAssociativeArrayElementGetString(query, "album"))) {
								char *artist = FskAssociativeArrayElementGetString(query, "artist");
								if (artist && *artist) {
									xsThrowIfFskErr(KprLibraryGetAlbumSongsOfArtist(the, value, artist));
								} else {
									xsThrowIfFskErr(KprLibraryGetAlbumSongs(the, value));
								}
							}
							else if ((value = FskAssociativeArrayElementGetString(query, "artist")))
								xsThrowIfFskErr(KprLibraryGetArtistSongs(the, value));
							else if ((value = FskAssociativeArrayElementGetString(query, "genre")))
								xsThrowIfFskErr(KprLibraryGetGenreSongs(the, value));
							else
								xsThrowIfFskErr(KprLibraryGetSongs(the));
						}
						target->result = xsMarshall(xsVar(0));
					}
					xsCatch {
					}
				}
			}
			xsEndHostSandboxCode();
		}
		else if (FskStrCompareWithLength("pictures", message->parts.name, message->parts.nameLength) == 0) {
            bailIfError(KprLibraryGetPictures(self, message));
		}
		else if (FskStrCompareWithLength("/pictures/", message->parts.path, 10) == 0) {
			if (FskStrCompareWithLength("save", message->parts.name, message->parts.nameLength) == 0) {
				bailIfError(KprLibrarySavePicture(self, message));
			}
		}
		else if (FskStrCompareWithLength("thumbnail", message->parts.name, message->parts.nameLength) == 0) {
			char* value;
			if ((value = FskAssociativeArrayElementGetString(query, "id")))	{
				bailIfError(KprLibraryGetSongThumbnail(message, value));
			}
			else if ((value = FskAssociativeArrayElementGetString(query, "url")))	{
				bailIfError(KprLibraryGetPictureThumbnail(self, message, value));
			}
		}
		else if (FskStrCompareWithLength("info", message->parts.name, message->parts.nameLength) == 0) {
			char* value;
			if ((value = FskAssociativeArrayElementGetString(query, "url")))	{
				bailIfError(KprLibraryGetPictureInfo(self, message, value));
			}
		}
	bail:
		FskAssociativeArrayDispose(query);
	}
	if (kFskErrNeedMoreTime != err) {
		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageLibraryEnd, message)
		message->error = err;
		KprMessageComplete(message);
	}
}

void KprLibraryStart(KprService service, FskThread thread, xsMachine* the)
{
	FskErr err = kFskErrNone;
	UInt32 flags = kFskThreadFlagsWaitForInit | kFskThreadFlagsJoinable;
	gLibraryThreadQuitting = false;
	bailIfError(FskThreadCreate(&gLibraryThread, KprLibraryLoop, flags, NULL, "Library"));
bail:
	return;
}

void KprLibraryStop(KprService service)
{
	gLibraryThreadQuitting = true;
	FskThreadJoin(gLibraryThread);
}

char KprLibraryGetInitial(xsMachine* the, NSString *string)
{
    if (string) {
        char* initial;
		string = [string lowercaseString];
		if ([string hasPrefix: @"("])
			string = [string substringFromIndex: 1];
		else if ([string hasPrefix: @"a "])
			string = [string substringFromIndex: 2];
		else if ([string hasPrefix: @"an "])
			string = [string substringFromIndex: 3];
		else if ([string hasPrefix: @"the "])
			string = [string substringFromIndex: 4];
		string = [string substringToIndex: 1];
		string = [string stringByFoldingWithOptions: NSCaseInsensitiveSearch + NSDiacriticInsensitiveSearch + NSWidthInsensitiveSearch locale: [NSLocale currentLocale]];
		string = [string uppercaseString];
		initial = (char*)[string UTF8String];
		if ((65 <= *initial) && (*initial <= 90)) {
			xsNewHostProperty(xsVar(1), xsID("initial"), xsString(initial), xsDefault, xsDontScript);
			return *initial - 65;
		}
	}
	xsNewHostProperty(xsVar(1), xsID("initial"), xsString("#"), xsDefault, xsDontScript);
	return 26;
}

void KprLibrarySort(xsMachine* the, xsIntegerValue i)
{
	xsIndex previous_id = xsID("previous");
	xsSet(xsVar(0), xsID("length"), xsInteger(i));
	char code;
	for (code = 29; code > 2; code--) {
		xsResult = xsVar(code);
		while (xsTest(xsResult)) {
			i--;
			xsSetAt(xsVar(0), xsInteger(i), xsResult);
			xsVar(1) = xsResult;
			xsResult = xsGet(xsResult, previous_id);
			xsDelete(xsVar(1), previous_id);
		}
	}
}

FskErr KprLibraryGetAlbums(xsMachine* the)
{
	FskErr err = kFskErrNone;
	MPMediaQuery *query = [MPMediaQuery albumsQuery];
	MPMediaPropertyPredicate *filter = [MPMediaPropertyPredicate 
		predicateWithValue:[NSNumber numberWithInt:0]
		forProperty: MPMediaItemPropertyIsCloudItem];
	[query addFilterPredicate: filter];
	NSArray *collections = [query collections];
	NSNumber *number;
	NSString *string;
	xsIntegerValue i = 0;
	char code;
	for (MPMediaItemCollection *collection in collections) {
        MPMediaItem* item = [collection representativeItem];
		xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
		string = [item valueForProperty: MPMediaItemPropertyAlbumTitle];
		xsNewHostProperty(xsVar(1), xsID("album"), xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
		code = 3 + KprLibraryGetInitial(the, string);
		string = [item valueForProperty: MPMediaItemPropertyArtist];
		xsNewHostProperty(xsVar(1), xsID("artist"), xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
		xsNewHostProperty(xsVar(1), xsID("songs"), xsInteger([collection count]), xsDefault, xsDontScript);
	
		number = [item valueForProperty: MPMediaItemPropertyPersistentID];
		xsVar(2) = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsString((char*)[[number stringValue] UTF8String]));
		xsResult = xsCall1(xsString( "xkpr://library/thumbnail?id="), xsID("concat"), xsVar(2));
		xsNewHostProperty(xsVar(1), xsID("thumbnail"), xsResult, xsDefault, xsDontScript);
	
		number = [item valueForProperty: MPMediaItemPropertyAlbumPersistentID];
		xsVar(2) = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsString((char*)[[number stringValue] UTF8String]));
		xsResult = xsCall1(xsString("xkpr://library/music/songs?album="), xsID("concat"), xsVar(2));
		xsNewHostProperty(xsVar(1), xsID("contents"), xsResult, xsDefault, xsDontScript);
		
		xsNewHostProperty(xsVar(1), xsID("previous"), xsVar(code), xsDefault, xsDontScript);
		xsVar(code) = xsVar(1);
		i++;
#ifdef TESTING
		if (i == 50)
			break;
#endif
	}
	KprLibrarySort(the, i);
	return err;
}

FskErr KprLibraryGetArtists(xsMachine* the)
{
	FskErr err = kFskErrNone;
	MPMediaQuery *query = [MPMediaQuery artistsQuery];
	MPMediaPropertyPredicate *filter = [MPMediaPropertyPredicate 
		predicateWithValue:[NSNumber numberWithInt:0]
		forProperty: MPMediaItemPropertyIsCloudItem];
	[query addFilterPredicate: filter];
	NSArray *collections = [query collections];
	NSNumber *number;
	NSString *string;
	xsIntegerValue i = 0;
	char code;
	for (MPMediaItemCollection *collection in collections) {
        MPMediaItem* item = [collection representativeItem];
		xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
		string = [item valueForProperty: MPMediaItemPropertyArtist];
 		xsNewHostProperty(xsVar(1), xsID("artist"), xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
        code = 3 + KprLibraryGetInitial(the, string);
		xsNewHostProperty(xsVar(1), xsID("songs"), xsInteger([collection count]), xsDefault, xsDontScript);
       
		number = [item valueForProperty: MPMediaItemPropertyPersistentID];
		xsVar(2) = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsString((char*)[[number stringValue] UTF8String]));
		xsResult = xsCall1(xsString( "xkpr://library/thumbnail?id="), xsID("concat"), xsVar(2));
		xsNewHostProperty(xsVar(1), xsID("thumbnail"), xsResult, xsDefault, xsDontScript);
        
		number = [item valueForProperty: MPMediaItemPropertyArtistPersistentID];
		MPMediaQuery *counter = [MPMediaQuery albumsQuery];
        [counter addFilterPredicate: [MPMediaPropertyPredicate predicateWithValue:number forProperty: MPMediaItemPropertyArtistPersistentID]];
		[counter addFilterPredicate: [MPMediaPropertyPredicate predicateWithValue:[NSNumber numberWithInt:0] forProperty: MPMediaItemPropertyIsCloudItem]];
		xsIntegerValue count = (xsIntegerValue)[[counter collections] count];
		[counter release];
		xsNewHostProperty(xsVar(1), xsID("albums"), xsInteger(count), xsDefault, xsDontScript);
		
		NSNumber *artistId = number;
		if (count == 1) {
			string = [item valueForProperty: MPMediaItemPropertyAlbumTitle];
        	xsNewHostProperty(xsVar(1), xsID("album"), xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
			
			NSNumber *albumId = [item valueForProperty: MPMediaItemPropertyAlbumPersistentID];
			string = [NSString stringWithFormat:@"xkpr://library/music/songs?album=%@&artist=%@", albumId, artistId];
		}
		else {
			string = [NSString stringWithFormat:@"xkpr://library/music/albums?artist=%@", artistId];
		}
		xsResult = xsString((char*)[string UTF8String]);
		xsNewHostProperty(xsVar(1), xsID("contents"), xsResult, xsDefault, xsDontScript);
		
		xsNewHostProperty(xsVar(1), xsID("previous"), xsVar(code), xsDefault, xsDontScript);
		xsVar(code) = xsVar(1);
		i++;
#ifdef TESTING
		if (i == 50)
			break;
#endif
	}
	KprLibrarySort(the, i);
	return err;
}

FskErr KprLibraryGetArtistAlbums(xsMachine* the, char* artist)
{
	FskErr err = kFskErrNone;
	NSNumber *artistId = @(FskStrToFskInt64(artist));
	MPMediaQuery *query = [MPMediaQuery albumsQuery];
	MPMediaPropertyPredicate *predicate = [MPMediaPropertyPredicate 
		predicateWithValue:artistId
		forProperty: MPMediaItemPropertyArtistPersistentID];
	[query addFilterPredicate: predicate];
	MPMediaPropertyPredicate *filter = [MPMediaPropertyPredicate 
		predicateWithValue:@0
		forProperty: MPMediaItemPropertyIsCloudItem];
	[query addFilterPredicate: filter];
	NSArray *collections = [query collections];
	NSNumber *number;
	NSString *string;
	xsSet(xsVar(0), xsID("length"), xsInteger([collections count]));
	xsIntegerValue i = 0;
	for (MPMediaItemCollection *collection in collections) {
        MPMediaItem* item = [collection representativeItem];
        
		xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
		string = [item valueForProperty: MPMediaItemPropertyAlbumTitle];
        xsNewHostProperty(xsVar(1), xsID("album"), xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
		string = [item valueForProperty: MPMediaItemPropertyArtist];
        xsNewHostProperty(xsVar(1), xsID("artist"), xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
        xsNewHostProperty(xsVar(1), xsID("songs"), xsInteger([collection count]), xsDefault, xsDontScript);
        
		number = [item valueForProperty: MPMediaItemPropertyPersistentID];
		xsVar(2) = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsString((char*)[[number stringValue] UTF8String]));
		xsResult = xsCall1(xsString( "xkpr://library/thumbnail?id="), xsID("concat"), xsVar(2));
		xsNewHostProperty(xsVar(1), xsID("thumbnail"), xsResult, xsDefault, xsDontScript);
        
		number = [item valueForProperty: MPMediaItemPropertyAlbumPersistentID];
		string = [NSString stringWithFormat:@"xkpr://library/music/songs?album=%@&artist=%@", number, artistId];
		xsResult = xsString((char*)[string UTF8String]);
		xsNewHostProperty(xsVar(1), xsID("contents"), xsResult, xsDefault, xsDontScript);
		
		xsSetAt(xsVar(0), xsInteger(i), xsVar(1));
		i++;
#ifdef TESTING
		if (i == 50)
			break;
#endif
	}
	xsSet(xsVar(0), xsID("length"), xsInteger(i));
	return err;
}

FskErr KprLibraryGetGenres(xsMachine* the)
{
	FskErr err = kFskErrNone;
	MPMediaQuery *query = [MPMediaQuery genresQuery];
	MPMediaPropertyPredicate *filter = [MPMediaPropertyPredicate 
		predicateWithValue:[NSNumber numberWithInt:0]
		forProperty: MPMediaItemPropertyIsCloudItem];
	[query addFilterPredicate: filter];
	NSArray *collections = [query collections];
	NSNumber *number;
	NSString *string;
	xsIntegerValue i = 0;
	char code;
	for (MPMediaItemCollection *collection in collections) {
        MPMediaItem* item = [collection representativeItem];
		xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
		string = [item valueForProperty: MPMediaItemPropertyGenre];
		xsNewHostProperty(xsVar(1), xsID("genre"), xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
		code = 3 + KprLibraryGetInitial(the, string);
		xsNewHostProperty(xsVar(1), xsID("songs"), xsInteger([collection count]), xsDefault, xsDontScript);
	
		number = [item valueForProperty: MPMediaItemPropertyPersistentID];
		xsVar(2) = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsString((char*)[[number stringValue] UTF8String]));
		xsResult = xsCall1(xsString( "xkpr://library/thumbnail?id="), xsID("concat"), xsVar(2));
		xsNewHostProperty(xsVar(1), xsID("thumbnail"), xsResult, xsDefault, xsDontScript);
	
		number = [item valueForProperty: MPMediaItemPropertyGenrePersistentID];
		xsVar(2) = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsString((char*)[[number stringValue] UTF8String]));
		xsResult = xsCall1(xsString( "xkpr://library/music/songs?genre="), xsID("concat"), xsVar(2));
		xsNewHostProperty(xsVar(1), xsID("contents"), xsResult, xsDefault, xsDontScript);
		
		xsNewHostProperty(xsVar(1), xsID("previous"), xsVar(code), xsDefault, xsDontScript);
		xsVar(code) = xsVar(1);
		i++;
#ifdef TESTING
		if (i == 50)
			break;
#endif
	}
	KprLibrarySort(the, i);
	return err;
}

FskErr KprLibraryGetAlbumSongs(xsMachine* the, char* album)
{
	NSArray *items = KprLibraryFetchAlbumSongs(album);
	KprLibraryBuildAlbumSongsArray(the, items);
	return kFskErrNone;
}

FskErr KprLibraryGetAlbumSongsOfArtist(xsMachine *the, char *album, char *artist)
{
	NSArray *items = KprLibraryFetchAlbumSongs(album);
	
	FskInt64 artistId = FskStrToFskInt64(artist);
	
	NSIndexSet *matched = [items indexesOfObjectsPassingTest:^BOOL(id obj, NSUInteger idx, BOOL *stop) {
		MPMediaItem *song = obj;
		FskInt64 songArtistId = [[song valueForProperty: MPMediaItemPropertyArtistPersistentID] longLongValue];
		if (artistId != songArtistId) return NO;
		return YES;
	}];
	if ([matched count] > 0) {
		items = [items objectsAtIndexes:matched];
	}
	
	KprLibraryBuildAlbumSongsArray(the, items);
	return kFskErrNone;
}

NSArray *KprLibraryFetchAlbumSongs(char* album)
{
	MPMediaQuery* query = [[[MPMediaQuery alloc] init] autorelease];
	MPMediaPropertyPredicate *predicate = [MPMediaPropertyPredicate
										   predicateWithValue:@(FskStrToFskInt64(album))
										   forProperty: MPMediaItemPropertyAlbumPersistentID];
	[query addFilterPredicate: predicate];
	MPMediaPropertyPredicate *filter = [MPMediaPropertyPredicate
										predicateWithValue:@0
										forProperty: MPMediaItemPropertyIsCloudItem];
	[query addFilterPredicate: filter];
	NSArray *items = [query items];
#ifdef TESTING
	if ([items count] > 50) {
		NSRange range = NSMakeRange(0, 50);
		items = [items subarrayWithRange:range];
	}
#endif
	return items;
}

void KprLibraryBuildAlbumSongsArray(xsMachine* the, NSArray *items)
{
	xsIndex album_id = xsID("album");
	xsIndex artist_id = xsID("artist");
	xsIndex concat_id = xsID("concat");
	xsIndex date_id = xsID("date");
	xsIndex duration_id = xsID("duration");
	xsIndex encodeURIComponent_id = xsID("encodeURIComponent");
	xsIndex genre_id = xsID("genre");
	xsIndex kind_id = xsID("kind");
	xsIndex mime_id = xsID("mime");
	xsIndex thumbnail_id = xsID("thumbnail");
	xsIndex title_id = xsID("title");
	xsIndex track_id = xsID("track");
	xsIndex url_id = xsID("url");
	NSDate* date;
	NSNumber *number;
	NSString *string;
	xsSet(xsVar(0), xsID("length"), xsInteger([items count]));
	xsIntegerValue i = 0;
	for (MPMediaItem *song in items) {
		xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
		string = [song valueForProperty: MPMediaItemPropertyAlbumTitle];
		xsNewHostProperty(xsVar(1), album_id, xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
		string = [song valueForProperty: MPMediaItemPropertyArtist];
		xsNewHostProperty(xsVar(1), artist_id, xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
		date = [song valueForProperty: MPMediaItemPropertyReleaseDate];
		xsNewHostProperty(xsVar(1), date_id, xsNumber([date timeIntervalSince1970] * 1000.0), xsDefault, xsDontScript);
		number = [song valueForProperty: MPMediaItemPropertyPlaybackDuration];
		xsNewHostProperty(xsVar(1), duration_id, xsNumber([number doubleValue] * 1000.0), xsDefault, xsDontScript);
		string = [song valueForProperty: MPMediaItemPropertyGenre];
		xsNewHostProperty(xsVar(1), genre_id, xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
		string = [song valueForProperty: MPMediaItemPropertyTitle];
		xsNewHostProperty(xsVar(1), title_id, xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
		number = [song valueForProperty: MPMediaItemPropertyAlbumTrackNumber];
		xsNewHostProperty(xsVar(1), track_id, xsInteger([number intValue]), xsDefault, xsDontScript);

		number = [song valueForProperty: MPMediaItemPropertyPersistentID];
		xsVar(2) = xsCall1(xsGlobal, encodeURIComponent_id, xsString((char*)[[number stringValue] UTF8String]));

		xsNewHostProperty(xsVar(1), kind_id, xsInteger(kprLibraryMPSongKind), xsDefault, xsDontScript);
		xsNewHostProperty(xsVar(1), mime_id, xsNull, xsDefault, xsDontScript);
		xsNewHostProperty(xsVar(1), url_id, xsVar(2), xsDefault, xsDontScript);

		xsResult = xsCall1(xsString( "xkpr://library/thumbnail?id="), concat_id, xsVar(2));
		xsNewHostProperty(xsVar(1), thumbnail_id, xsResult, xsDefault, xsDontScript);

		xsSetAt(xsVar(0), xsInteger(i), xsVar(1));
		i++;
	}
}

FskErr KprLibraryGetArtistSongs(xsMachine* the, char* artist)
{
	FskErr err = kFskErrNone;
	MPMediaQuery* query = [[[MPMediaQuery alloc] init] autorelease];
	MPMediaPropertyPredicate *predicate = [MPMediaPropertyPredicate 
		predicateWithValue:[NSNumber numberWithLongLong:FskStrToFskInt64(artist)]
		forProperty: MPMediaItemPropertyArtistPersistentID];
	[query addFilterPredicate: predicate];
	MPMediaPropertyPredicate *filter = [MPMediaPropertyPredicate 
		predicateWithValue:[NSNumber numberWithInt:0]
		forProperty: MPMediaItemPropertyIsCloudItem];
	[query addFilterPredicate: filter];
	KprLibraryGetSongsAux(the, [query items]);
	return err;
}

FskErr KprLibraryGetGenreSongs(xsMachine* the, char* genre)
{
	FskErr err = kFskErrNone;
	MPMediaQuery* query = [[[MPMediaQuery alloc] init] autorelease];
	MPMediaPropertyPredicate *predicate = [MPMediaPropertyPredicate 
		predicateWithValue:[NSNumber numberWithLongLong:FskStrToFskInt64(genre)]
		forProperty: MPMediaItemPropertyGenrePersistentID];
	[query addFilterPredicate: predicate];
	MPMediaPropertyPredicate *filter = [MPMediaPropertyPredicate 
		predicateWithValue:[NSNumber numberWithInt:0]
		forProperty: MPMediaItemPropertyIsCloudItem];
	[query addFilterPredicate: filter];
	KprLibraryGetSongsAux(the, [query items]);
	return err;
}

FskErr KprLibraryGetSongs(xsMachine* the)
{
	FskErr err = kFskErrNone;
	MPMediaQuery *query = [MPMediaQuery songsQuery];
	MPMediaPropertyPredicate *filter = [MPMediaPropertyPredicate 
		predicateWithValue:[NSNumber numberWithInt:0]
		forProperty: MPMediaItemPropertyIsCloudItem];
	[query addFilterPredicate: filter];
	KprLibraryGetSongsAux(the, [query items]);
	return err;
}

FskErr KprLibraryGetSongsAux(xsMachine* the, NSArray *items)
{
	xsIndex album_id = xsID("album");
	xsIndex artist_id = xsID("artist");
	xsIndex concat_id = xsID("concat");
	xsIndex date_id = xsID("date");
	xsIndex duration_id = xsID("duration");
	xsIndex encodeURIComponent_id = xsID("encodeURIComponent");
	xsIndex genre_id = xsID("genre");
	xsIndex kind_id = xsID("kind");
	xsIndex mime_id = xsID("mime");
	xsIndex previous_id = xsID("previous");
	xsIndex thumbnail_id = xsID("thumbnail");
	xsIndex title_id = xsID("title");
	xsIndex track_id = xsID("track");
	xsIndex url_id = xsID("url");
	NSDate* date;
	NSNumber *number;
	NSString *string;
	xsSet(xsVar(0), xsID("length"), xsInteger([items count]));
	xsIntegerValue i = 0;
	char code;
	for (MPMediaItem *song in items) {
		xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
		string = [song valueForProperty: MPMediaItemPropertyAlbumTitle];
		xsNewHostProperty(xsVar(1), album_id, xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
		string = [song valueForProperty: MPMediaItemPropertyArtist];
		xsNewHostProperty(xsVar(1), artist_id, xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
		date = [song valueForProperty: MPMediaItemPropertyReleaseDate];
		xsNewHostProperty(xsVar(1), date_id, xsNumber([date timeIntervalSince1970] * 1000.0), xsDefault, xsDontScript);
		number = [song valueForProperty: MPMediaItemPropertyPlaybackDuration];
		xsNewHostProperty(xsVar(1), duration_id, xsNumber([number doubleValue] * 1000.0), xsDefault, xsDontScript);
		string = [song valueForProperty: MPMediaItemPropertyGenre];
		xsNewHostProperty(xsVar(1), genre_id, xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
		string = [song valueForProperty: MPMediaItemPropertyTitle];
		xsNewHostProperty(xsVar(1), title_id, xsString(string ? (char*)[string UTF8String] : ""), xsDefault, xsDontScript);
		code = 3 + KprLibraryGetInitial(the, string);
		number = [song valueForProperty: MPMediaItemPropertyAlbumTrackNumber];
		xsNewHostProperty(xsVar(1), track_id, xsInteger([number intValue]), xsDefault, xsDontScript);

		number = [song valueForProperty: MPMediaItemPropertyPersistentID];
		xsVar(2) = xsCall1(xsGlobal, encodeURIComponent_id, xsString((char*)[[number stringValue] UTF8String]));

		xsNewHostProperty(xsVar(1), kind_id, xsInteger(kprLibraryMPSongKind), xsDefault, xsDontScript);
		xsNewHostProperty(xsVar(1), mime_id, xsNull, xsDefault, xsDontScript);
		xsNewHostProperty(xsVar(1), url_id, xsVar(2), xsDefault, xsDontScript);

		xsResult = xsCall1(xsString( "xkpr://library/thumbnail?id="), concat_id, xsVar(2));
		xsNewHostProperty(xsVar(1), thumbnail_id, xsResult, xsDefault, xsDontScript);

		xsNewHostProperty(xsVar(1), previous_id, xsVar(code), xsDefault, xsDontScript);
		xsVar(code) = xsVar(1);
		i++;
#ifdef TESTING
		if (i == 50)
			break;
#endif
	}
	KprLibrarySort(the, i);
	return kFskErrNone;
}

FskErr KprLibraryGetSongThumbnail(KprMessage message, char* song)
{
	FskErr err = kFskErrNone;
	KprImageTarget target = (KprImageTarget)message->stream;
	CGSize size;
	if (target->width && target->height) {
		size.width = target->width;
		size.height = target->height;
	}
	else {
		size.width = 150;
		size.height = 150;
	}
	MPMediaQuery* query = [[[MPMediaQuery alloc] init] autorelease];
	MPMediaPropertyPredicate *idPredicate = [MPMediaPropertyPredicate 
		predicateWithValue:[NSNumber numberWithLongLong:FskStrToFskInt64(song)]
		forProperty: MPMediaItemPropertyPersistentID];
	[query addFilterPredicate: idPredicate];
	NSArray *items = [query items];
	for (MPMediaItem *item in items) {
		MPMediaItemArtwork *artwork = [item valueForProperty: MPMediaItemPropertyArtwork];
        if (artwork) {
            CGRect bounds = artwork.bounds;
            CGFloat half = bounds.size.width / 2, quarter = bounds.size.width / 4;
            SInt32 percent = (gGeneration <= 4) ? 95 : 40;

            if (size.width < ((half * percent) / 100))
                size.width = size.height = quarter;
            else if (size.width < ((half * 3) / 2))
                size.width = size.height = half;
            else
                size.width = size.height = bounds.size.width;

            UIImage *image = [artwork imageWithSize: size];
            if (image) {
				//fprintf(stderr, "%dx%d -> %dx%d %s\n", CGImageGetWidth(image.CGImage), CGImageGetHeight(image.CGImage), target->width, target->height, message->url);
 				bailIfError(FskBitmapNew(size.width, size.height, kFskBitmapFormatDefaultNoAlpha, &target->bitmap));
                FskBitmapWriteBegin(target->bitmap, NULL, NULL, NULL);
                    CGContextDrawImage(FskBitmapGetNativeBitmap(target->bitmap), CGRectMake(0, 0, size.width, size.height), image.CGImage);
                FskBitmapWriteEnd(target->bitmap);
           }
        }
        break;
	}
bail:
	return err;
}

FskErr KprLibraryGetMovies(KprLibrary library, KprMessage message)
{
	static Boolean done;
	FskErr err = kFskErrNone;
	FskMutex mutex = NULL;
	FskCondition condition = NULL;
    bailIfError(FskConditionNew(&condition));
    bailIfError(FskMutexNew(&mutex, "iOS movies"));
	xsBeginHostSandboxCode(library->the, NULL);
	xsVars(3);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	ALAssetsGroupEnumerationResultsBlock assetBlock = ^(ALAsset *asset, NSUInteger index, BOOL *stop) {
		NSString* type = [asset valueForProperty:ALAssetPropertyType];
		if (type == ALAssetTypeVideo) {
			ALAssetRepresentation *representation = [asset defaultRepresentation];
			if (representation) {
				NSURL* url = [representation url];
				if (url) {
					xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
					xsVar(2) = xsString((xsStringValue)[[url absoluteString] UTF8String]);
					xsNewHostProperty(xsVar(1), xsID("content"), xsVar(2), xsDefault, xsDontScript);
			
					NSDate* date = [asset valueForProperty:ALAssetPropertyDate];
					xsNewHostProperty(xsVar(1), xsID("date"), xsNumber([date timeIntervalSince1970] * 1000.0), xsDefault, xsDontScript);
					NSNumber* duration = [asset valueForProperty:ALAssetPropertyDuration];
					xsNewHostProperty(xsVar(1), xsID("duration"), xsNumber([duration doubleValue] * 1000.0), xsDefault, xsDontScript);
					NSString* title = [representation filename];
					xsNewHostProperty(xsVar(1), xsID("title"), xsString((xsStringValue)[title UTF8String]), xsDefault, xsDontScript);
					CGSize dimensions = [representation dimensions];
					xsNewHostProperty(xsVar(1), xsID("width"), xsInteger(dimensions.width), xsDefault, xsDontScript);
					xsNewHostProperty(xsVar(1), xsID("height"), xsInteger(dimensions.height), xsDefault, xsDontScript);
				
					xsNewHostProperty(xsVar(1), xsID("kind"), xsInteger(kprLibraryALVideoKind), xsDefault, xsDontScript);
					xsNewHostProperty(xsVar(1), xsID("mime"), xsNull, xsDefault, xsDontScript);
					xsNewHostProperty(xsVar(1), xsID("url"), xsVar(2), xsDefault, xsDontScript);
					
					xsVar(2) = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsVar(2));
					xsResult = xsCall1(xsString( "xkpr://library/thumbnail?url="), xsID("concat"), xsVar(2));
					xsNewHostProperty(xsVar(1), xsID("thumbnail"), xsResult, xsDefault, xsDontScript);

					xsCall1(xsVar(0), xsID("push"), xsVar(1));
				}
			}
		}
	};
	ALAssetsLibraryGroupsEnumerationResultsBlock groupBlock = ^(ALAssetsGroup *group, BOOL *stop) {
        if (group)
            [group enumerateAssetsUsingBlock:assetBlock];
        else {
			FskMutexAcquire(mutex);
			done = true;
			FskConditionSignal(condition);
			FskMutexRelease(mutex);
        }
	};
	ALAssetsLibraryAccessFailureBlock failureBlock = ^(NSError *error) {
		message->error = error.code;
		FskMutexAcquire(mutex);
		done = true;
		FskConditionSignal(condition);
		FskMutexRelease(mutex);
	};
	done = false;
	[library->library enumerateGroupsWithTypes:(ALAssetsGroupSavedPhotos | ALAssetsGroupLibrary) usingBlock:groupBlock failureBlock:failureBlock];
	FskMutexAcquire(mutex);
	while (!done)
		FskConditionWait(condition, mutex);
	FskMutexRelease(mutex);
	
    KprMessageScriptTarget target = (KprMessageScriptTarget)message->stream;
	target->result = xsMarshall(xsVar(0));
	
	xsEndHostSandboxCode();
bail:
	FskMutexDispose(mutex);
	FskConditionDispose(condition);
	return err;
}


FskErr KprLibraryGetPictures(KprLibrary library, KprMessage message)
{
	static Boolean done;
	FskErr err = kFskErrNone;
	FskMutex mutex = NULL;
	FskCondition condition = NULL;
    bailIfError(FskConditionNew(&condition));
    bailIfError(FskMutexNew(&mutex, "iOS pictures"));
	xsBeginHostSandboxCode(library->the, NULL);
	xsVars(3);
	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

	ALAssetsGroupEnumerationResultsBlock assetBlock = ^(ALAsset *asset, NSUInteger index, BOOL *stop) {
		NSString* type = [asset valueForProperty:ALAssetPropertyType];
		if (type == ALAssetTypePhoto) {
			ALAssetRepresentation *representation = [asset defaultRepresentation];
			if (representation) {
				NSURL* url = [representation url];
				if (url) {
					xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
					xsVar(2) = xsString((xsStringValue)[[url absoluteString] UTF8String]);
					xsNewHostProperty(xsVar(1), xsID("content"), xsVar(2), xsDefault, xsDontScript);
			
					NSDate* date = [asset valueForProperty:ALAssetPropertyDate];
					xsNewHostProperty(xsVar(1), xsID("date"), xsNumber([date timeIntervalSince1970] * 1000.0), xsDefault, xsDontScript);
					NSNumber* orientation = [asset valueForProperty:ALAssetPropertyOrientation];
					xsIntegerValue rotation = 0;
					switch ([orientation integerValue]) {
					case 1: rotation = 0; break;
					case 3: rotation = 180; break;
					case 6: rotation = 270; break;
					case 8: rotation = 90; break;
					}
					xsNewHostProperty(xsVar(1), xsID("rotation"), xsInteger(rotation), xsDefault, xsDontScript);
					NSString* title = [representation filename];
					xsNewHostProperty(xsVar(1), xsID("title"), xsString((xsStringValue)[title UTF8String]), xsDefault, xsDontScript);
					CGSize dimensions = [representation dimensions];
					xsNewHostProperty(xsVar(1), xsID("width"), xsInteger(dimensions.width), xsDefault, xsDontScript);
					xsNewHostProperty(xsVar(1), xsID("height"), xsInteger(dimensions.height), xsDefault, xsDontScript);
				
					xsNewHostProperty(xsVar(1), xsID("kind"), xsInteger(kprLibraryALImageKind), xsDefault, xsDontScript);
					xsNewHostProperty(xsVar(1), xsID("mime"), xsNull, xsDefault, xsDontScript);
					xsNewHostProperty(xsVar(1), xsID("url"), xsVar(2), xsDefault, xsDontScript);
			
					xsVar(2) = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsVar(2));
					xsResult = xsCall1(xsString( "xkpr://library/thumbnail?url="), xsID("concat"), xsVar(2));
					xsNewHostProperty(xsVar(1), xsID("thumbnail"), xsResult, xsDefault, xsDontScript);
					
					xsCall1(xsVar(0), xsID("unshift"), xsVar(1));
				}
			}
		}
	};
	ALAssetsLibraryGroupsEnumerationResultsBlock groupBlock = ^(ALAssetsGroup *group, BOOL *stop) {
        if (group)
            [group enumerateAssetsUsingBlock:assetBlock];
        else {
			FskMutexAcquire(mutex);
			done = true;
			FskConditionSignal(condition);
			FskMutexRelease(mutex);
        }
	};
	ALAssetsLibraryAccessFailureBlock failureBlock = ^(NSError *error) {
		message->error = error.code;
		FskMutexAcquire(mutex);
		done = true;
		FskConditionSignal(condition);
		FskMutexRelease(mutex);
	};
	done = false;
	[library->library enumerateGroupsWithTypes:(ALAssetsGroupSavedPhotos | ALAssetsGroupLibrary) usingBlock:groupBlock failureBlock:failureBlock];
	FskMutexAcquire(mutex);
	while (!done)
		FskConditionWait(condition, mutex);
	FskMutexRelease(mutex);
	
    KprMessageScriptTarget target = (KprMessageScriptTarget)message->stream;
	target->result = xsMarshall(xsVar(0));
	
	xsEndHostSandboxCode();
bail:
	FskMutexDispose(mutex);
	FskConditionDispose(condition);
	return err;
}

FskErr KprLibrarySavePicture(KprLibrary library, KprMessage message)
{
	ALAssetsLibraryWriteImageCompletionBlock completionBlock = ^(NSURL *assetURL, NSError *error) {		
		if (error)
			message->error = error.code;

		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageLibraryEnd, message);
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
	};

	NSData *imageData = [NSData dataWithBytesNoCopy:message->request.body length:message->request.size freeWhenDone:NO];

	//TODO: Add some metadata, such as the photo orientation
	[library->library writeImageDataToSavedPhotosAlbum:imageData metadata:NULL completionBlock:completionBlock];
	return kFskErrNeedMoreTime;
}

FskErr KprLibraryGetPictureThumbnail(KprLibrary library, KprMessage message, char* url)
{
    static FskMutex wait = NULL;
    static CGSize thumbnail, aspectRatioThumbnail;
    static CGSize thumbnailThreshold, aspectRatioThumbnailThreshold;

	ALAssetsLibraryAssetForURLResultBlock resultBlock = ^(ALAsset *asset) {
		KprImageTarget target = (KprImageTarget)message->stream;
		CGImageRef image;
		CGSize size;
        
        if (0 == aspectRatioThumbnailThreshold.height) {
            FskMutexAcquire(wait);

            if (0 == aspectRatioThumbnailThreshold.height) {
                image = asset.thumbnail;
                thumbnail.width = CGImageGetWidth(image);
                thumbnail.height = CGImageGetHeight(image);

                image = asset.aspectRatioThumbnail;
                aspectRatioThumbnail.width = CGImageGetWidth(image);
                aspectRatioThumbnail.height = CGImageGetHeight(image);

                if ((aspectRatioThumbnail.width > thumbnail.width) && (aspectRatioThumbnail.height > thumbnail.height)) {
                    thumbnailThreshold.width = thumbnail.width + ((aspectRatioThumbnail.width - thumbnail.width) / 2);
                    thumbnailThreshold.height = thumbnail.height + ((aspectRatioThumbnail.height - thumbnail.height) / 2);

                    aspectRatioThumbnailThreshold.width = (aspectRatioThumbnail.width * 3) / 2;
                    aspectRatioThumbnailThreshold.height = (aspectRatioThumbnail.height * 3) / 2;
                }
                else {
                    // aspectRatioThumbnail  is smaller than thumbnail. Don't use it.
                    thumbnailThreshold.width = (thumbnail.width * 3) / 2;
                    thumbnailThreshold.height = (thumbnail.height * 3) / 2;

                    aspectRatioThumbnail.width = 1;
                    aspectRatioThumbnail.height = 1;
                    aspectRatioThumbnailThreshold.width = 1;
                    aspectRatioThumbnailThreshold.height = 1;
                }
            }

            FskMutexRelease(wait);
        }

		if (target->width && target->height) {
            if ((target->width <= thumbnailThreshold.width) && (target->height <= thumbnailThreshold.height))
                image = asset.thumbnail;
            else if ((target->width <= aspectRatioThumbnailThreshold.width) && (target->height <= aspectRatioThumbnailThreshold.height))
                image = asset.aspectRatioThumbnail;
            else
                image = [[asset defaultRepresentation] fullScreenImage];
		}
		else
            image = asset.thumbnail;

        size.width = CGImageGetWidth(image);
        size.height = CGImageGetHeight(image);

        FskRectangleRecord bounds;
        FskRectangleSet(&bounds, 0, 0, size.width, size.height);
        if (size.width == size.height)
            ;
        else if (size.width > size.height) {
            bounds.x = (size.height - size.width) / 2;
            size.width = size.height;
        }
        else {
            bounds.y = (size.width - size.height) / 2;
            size.height = size.width;
        }
        
		//fprintf(stderr, "%dx%d -> %dx%d %s\n", CGImageGetWidth(image), CGImageGetHeight(image), target->width, target->height, message->url);
 		FskBitmapNew(size.width, size.height, kFskBitmapFormatDefaultNoAlpha, &target->bitmap);
        FskBitmapWriteBegin(target->bitmap, NULL, NULL, NULL);
            CGContextDrawImage(FskBitmapGetNativeBitmap(target->bitmap), CGRectMake(bounds.x, bounds.y, bounds.width, bounds.height), image);
        FskBitmapWriteEnd(target->bitmap);

		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageLibraryEnd, message)
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
	};
	ALAssetsLibraryAccessFailureBlock failureBlock = ^(NSError *error) {
		message->error = error.code;
		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageLibraryEnd, message)
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
	};
    if (NULL == wait)
        FskMutexNew(&wait, "iOS thumbnail");
    [library->library assetForURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]] resultBlock:resultBlock failureBlock:failureBlock];
	return kFskErrNeedMoreTime;
}

FskErr KprLibraryGetPictureInfo(KprLibrary library, KprMessage message, char* url)
{
	ALAssetsLibraryAssetForURLResultBlock resultBlock = ^(ALAsset *asset) {
		NSString* type = [asset valueForProperty:ALAssetPropertyType];
		if (type == ALAssetTypePhoto)
			KprMessageSetResponseHeader(message, "content-type", "image/jpeg");
		else
			KprMessageSetResponseHeader(message, "content-type", "video/mp4");
		ALAssetRepresentation *representation = [asset defaultRepresentation];
		if (representation) {
			char buffer[16];
			FskStrNumToStr([representation size], buffer, sizeof(buffer));
			KprMessageSetResponseHeader(message, "content-length", buffer);
		}
		else
			message->error = kFskErrNotFound;
		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageLibraryEnd, message)
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
	};
	ALAssetsLibraryAccessFailureBlock failureBlock = ^(NSError *error) {
		message->error = error.code;
		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageLibraryEnd, message)
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
	};
	[gLibrary.library assetForURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]] resultBlock:resultBlock failureBlock:failureBlock];
	return kFskErrNeedMoreTime;
}

/* SERVER */

FskErr KprLibraryServerGetPicture(KprLibrarySession session)
{
	ALAssetsLibraryAssetForURLResultBlock resultBlock = ^(ALAsset *asset) {
		ALAssetRepresentation *representation = [asset defaultRepresentation];
		if (representation) {
			session->data.size = [representation size];
			session->error = FskMemPtrNew(session->data.size, &session->data.buffer);
			if (kFskErrNone == session->error) {
				if (0 == [representation getBytes:session->data.buffer fromOffset:0 length:session->data.size error:NULL])
					session->error = kFskErrBadData;
			}
		}
		else
			session->error = kFskErrNotFound;
		FskHTTPServerRequestResume(session->request);
	};
	ALAssetsLibraryAccessFailureBlock failureBlock = ^(NSError *error) {
		session->error = kFskErrOperationFailed;
		FskHTTPServerRequestResume(session->request);
	};
	[gLibrary.library assetForURL:[NSURL URLWithString:[NSString stringWithUTF8String:session->url]] resultBlock:resultBlock failureBlock:failureBlock];
	return kFskErrNeedMoreTime;
}

FskErr KprLibraryServerGetMovie(KprLibrarySession session)
{
	ALAssetsLibraryAssetForURLResultBlock resultBlock = ^(ALAsset *asset) {
		ALAssetRepresentation *representation = [asset defaultRepresentation];
		if (representation) {
			char* directory = NULL;
			FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeCache, true, NULL, &directory);
			session->file.path = FskStrDoCat(directory, [[representation filename] UTF8String]);
			FskMemPtrDispose(directory);
			if (kFskErrNone == FskFileGetFileInfo(session->file.path, &session->file.info)) {
                session->error = FskFileOpen(session->file.path, kFskFilePermissionReadOnly, &session->file.file);
				FskHTTPServerRequestResume(session->request);
            }
			else {
				AVURLAsset* asset = [AVURLAsset URLAssetWithURL:representation.url options:nil];
				AVAssetExportSession *exporter = [[AVAssetExportSession alloc] initWithAsset: asset presetName: AVAssetExportPresetMediumQuality];
				exporter.outputURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:session->file.path]];
				exporter.shouldOptimizeForNetworkUse = YES;
				exporter.outputFileType = AVFileTypeMPEG4;
	
				[exporter exportAsynchronouslyWithCompletionHandler: ^(void) {
					switch ([exporter status]) {
					case AVAssetExportSessionStatusFailed:
						session->error = kFskErrOperationFailed;
						break;
					case AVAssetExportSessionStatusCancelled:
						session->error = kFskErrOperationCancelled;
						break;
					default:
						session->error = FskFileGetFileInfo(session->file.path, &session->file.info);
						if (kFskErrNone == session->error)
               		 		session->error = FskFileOpen(session->file.path, kFskFilePermissionReadOnly, &session->file.file);
						break;
					}
					FskHTTPServerRequestResume(session->request); 
				}];
			}
		}
		else {
			session->error = kFskErrNotFound;
			FskHTTPServerRequestResume(session->request); 
		}
	};
	ALAssetsLibraryAccessFailureBlock failureBlock = ^(NSError *error) {
		session->error = kFskErrOperationFailed;
		FskHTTPServerRequestResume(session->request);
	};
	[gLibrary.library assetForURL:[NSURL URLWithString:[NSString stringWithUTF8String:session->url]] resultBlock:resultBlock failureBlock:failureBlock];
	return kFskErrNeedMoreTime;
}

static FskErr muxerOutputFile(FskMuxer muxer, void *refCon, const void *data, FskFileOffset offset, UInt32 dataSize)
{
    FskFile fref = refCon;

    if (fref) {
        FskFileSetPosition(fref, &offset);
        return FskFileWrite(fref, dataSize, data, NULL);
    }
    return kFskErrUnimplemented;
}


FskErr KprLibraryServerGetSong(KprLibrarySession session)
{
	FskErr err = kFskErrNone;
	char* directory = NULL;
	err = FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeCache, true, NULL, &directory);
	session->file.path = FskStrDoCat(directory, session->url);
    err = FskFileGetFileInfo(directory, &session->file.info);
	FskMemPtrDispose(directory);
	if (kFskErrNone == FskFileGetFileInfo(session->file.path, &session->file.info)) {
		err = FskFileOpen(session->file.path, kFskFilePermissionReadOnly, &session->file.file);
	}
	else {
		MPMediaQuery* query = [[[MPMediaQuery alloc] init] autorelease];
		MPMediaPropertyPredicate *predicate = [MPMediaPropertyPredicate 
			predicateWithValue:[NSNumber numberWithLongLong:FskStrToFskInt64(session->url)]
			forProperty: MPMediaItemPropertyPersistentID];
		[query addFilterPredicate:predicate];
		NSArray *items = [query items];
		NSURL* url = ([items count]) ? [[items objectAtIndex:0] valueForProperty: MPMediaItemPropertyAssetURL] : NULL;
        if (url) {
        	if (0 == FskStrCompare(session->mime, "audio/mpeg")) {
				FskFile fref;
				FskFileDelete(session->file.path);
				FskFileCreate(session->file.path);
				FskFileOpen(session->file.path, kFskFilePermissionReadWrite, &fref);

				AVURLAsset* asset = [AVURLAsset URLAssetWithURL: url options:NULL];

				NSError* nserr;
				AVAssetReader *assetReader = [AVAssetReader assetReaderWithAsset:asset error:&nserr];
				[assetReader retain];

				NSArray* audioTracks = [asset tracksWithMediaType:AVMediaTypeAudio];
				if ([audioTracks count]) {
					AVAssetTrack* audioTrack = [audioTracks objectAtIndex:0];
				
					NSArray *audioFormatDescriptions = [audioTrack formatDescriptions];
					CMAudioFormatDescriptionRef audioFormatDescription = (CMFormatDescriptionRef)[audioFormatDescriptions objectAtIndex:0];
					const AudioStreamBasicDescription *audioStreamBasicDescription = CMAudioFormatDescriptionGetStreamBasicDescription(audioFormatDescription);
				
					NSMutableDictionary* audioReadSettings = NULL;
					if (audioStreamBasicDescription->mFormatID == kAudioFormatMPEGLayer3) {
						AVAssetReaderTrackOutput *audioTrackOutput = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:audioTrack outputSettings:audioReadSettings];
						[assetReader addOutput:audioTrackOutput];
						[assetReader startReading];


						FskMuxer muxer;
						FskErr err = FskMuxerNew(&muxer, "audio/mpeg", audioStreamBasicDescription->mSampleRate, muxerOutputFile, fref);

						FskMuxerTrack audioMuxerOutput;
						err = FskMuxerTrackNew(muxer, &audioMuxerOutput, "sound", audioStreamBasicDescription->mSampleRate);

						FskMediaPropertyValueRecord channels;
						channels.type = kFskMediaPropertyTypeInteger;
						channels.value.integer = audioStreamBasicDescription->mChannelsPerFrame;
						FskMuxerTrackSetProperty(audioMuxerOutput, kFskMediaPropertyChannelCount, &channels);

						FskMediaPropertyValueRecord sampleRate;
						sampleRate.type = kFskMediaPropertyTypeInteger;
						sampleRate.value.integer = audioStreamBasicDescription->mSampleRate;
						FskMuxerTrackSetProperty(audioMuxerOutput, kFskMediaPropertySampleRate, &sampleRate);

						FskMediaPropertyValueRecord format;
						format.type = kFskMediaPropertyTypeString;
						format.value.str = "x-audio-codec/x-audio-codec/mp3";
						FskMuxerTrackSetProperty(audioMuxerOutput, kFskMediaPropertyFormat, &format);

						//@@ set metadata here

						err = FskMuxerStart(muxer);

						FskTimeRecord start, end;
						FskTimeGetNow(&start);
						size_t totalSize = 0;
						while (true) {
							CMSampleBufferRef sampleBuffer = [audioTrackOutput copyNextSampleBuffer];
							if (NULL == sampleBuffer)
								break;
						
							CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
							size_t size = CMBlockBufferGetDataLength(blockBuffer);
							totalSize += size;

							FskMemPtr buffer;
							if (kFskErrNone != FskMemPtrNew(size, &buffer))
								break;

							CMBlockBufferCopyDataBytes(blockBuffer, 0, size, buffer);

							FskMuxerSampleInfo info;
							UInt32 i, count = CMSampleBufferGetNumSamples(sampleBuffer);
							FskMemPtrNewClear(count * sizeof(FskMuxerSampleInfoRecord), &info);
							for (i = 0; i < count; i++) {
								info[i].sampleCount = 1;
								info[i].sampleSize = CMSampleBufferGetSampleSize(sampleBuffer, i);
								info[i].sampleDuration = 1152;
								info[i].flags = kFskImageFrameTypeSync;
							}

							err = FskMuxerTrackAdd(audioMuxerOutput, buffer, count, info);

							FskMemPtrDispose(info);
							FskMemPtrDispose(buffer);
						}
						FskTimeGetNow(&end);
						printf("bytes %d, ms %d\n", (int)totalSize, (int)(FskTimeInMS(&end) - FskTimeInMS(&start)));
					
						FskMuxerStop(muxer);
						FskMuxerDispose(muxer);
					}
				}

				[assetReader release];
				FskFileClose(fref);
				err = FskFileGetFileInfo(session->file.path, &session->file.info);
                if (kFskErrNone == err)
                    err = FskFileOpen(session->file.path, kFskFilePermissionReadOnly, &session->file.file);
			}
			else {
				AVURLAsset* asset = [AVURLAsset URLAssetWithURL: url options:NULL];
				AVAssetExportSession *exporter = [[AVAssetExportSession alloc] initWithAsset: asset presetName: AVAssetExportPresetAppleM4A];
				NSURL* outputURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:session->file.path]];
				exporter.outputURL = outputURL;
				exporter.shouldOptimizeForNetworkUse = YES;
				exporter.outputFileType = AVFileTypeAppleM4A;
	
				[exporter exportAsynchronouslyWithCompletionHandler: ^(void) {
					switch ([exporter status]) {
						case AVAssetExportSessionStatusFailed: {
//							int error = [[exporter error] code];
							session->error = kFskErrOperationFailed;
						}
						break;
					case AVAssetExportSessionStatusCancelled:
						session->error = kFskErrOperationCancelled;
						break;
					default:
						session->error = FskFileGetFileInfo(session->file.path, &session->file.info);
						if (kFskErrNone == session->error)
							session->error = FskFileOpen(session->file.path, kFskFilePermissionReadOnly, &session->file.file);
						break;
					}
					FskHTTPServerRequestResume(session->request); 
				}];
				err = kFskErrNeedMoreTime;
			}
		}
		else 
			err = kFskErrNotFound;
	}
	return err;
}

FskErr KprLibraryServerGetMovieAndPictureThumbnail(KprLibrarySession session)
{
	ALAssetsLibraryAssetForURLResultBlock resultBlock = ^(ALAsset *asset) {
		session->error = kFskErrNotFound;
		CGImageRef cgImage = [asset thumbnail];
		if (cgImage) {
			UIImage* uiImage = [UIImage imageWithCGImage:cgImage];
			if (uiImage) {
				NSData *data = UIImageJPEGRepresentation(uiImage, 0.6);
				if (data) {
					session->data.size = data.length;
					session->error = FskMemPtrNewFromData(session->data.size, data.bytes, &session->data.buffer);
				}
			}
		}
		FskHTTPServerRequestResume(session->request);
	};
	ALAssetsLibraryAccessFailureBlock failureBlock = ^(NSError *error) {
		session->error = kFskErrOperationFailed;
		FskHTTPServerRequestResume(session->request);
	};
	[gLibrary.library assetForURL:[NSURL URLWithString:[NSString stringWithUTF8String:session->url]] resultBlock:resultBlock failureBlock:failureBlock];
	return kFskErrNeedMoreTime;
}

FskErr KprLibraryServerGetSongThumbnail(KprLibrarySession session)
{
	session->error = kFskErrNotFound;
	MPMediaQuery* query = [[[MPMediaQuery alloc] init] autorelease];
	MPMediaPropertyPredicate *predicate = [MPMediaPropertyPredicate 
		predicateWithValue:[NSNumber numberWithLongLong:FskStrToFskInt64(session->url)]
		forProperty: MPMediaItemPropertyPersistentID];
	[query addFilterPredicate:predicate];
	NSArray *items = [query items];
	MPMediaItemArtwork* artwork = NULL;
	for (MPMediaItem *item in items) {
		artwork = [item valueForProperty: MPMediaItemPropertyArtwork];
		break;
	}
	if (artwork) {
		CGSize size;
		size.width = 320; //session->width;
		size.height = 320; //session->height;
		UIImage* image = [artwork imageWithSize:size];
		if (image) {
			NSData* data = UIImageJPEGRepresentation(image, 0.6);
			if (data) {
				session->data.size = data.length;
				session->error = FskMemPtrNewFromData(session->data.size, data.bytes, &session->data.buffer);
			}
		}
	}
	return kFskErrNone;
}

FskErr KprDataServerOpen(KprLibrarySession session)
{
	FskErr err = kFskErrNone;
	if (FskStrCompareWithLength("xkpr", session->parts.scheme, 4) == 0) {
		err = KprLibrarySessionRedirect(session);
	}
	else if (kprLibraryALImageKind == session->kind) {
		err = KprLibraryServerGetPicture(session);
	}
	else if (kprLibraryALVideoKind == session->kind) {
		err = KprLibraryServerGetMovie(session);
	}
	else if (kprLibraryMPSongKind == session->kind) {
		err = KprLibraryServerGetSong(session);
	}
	else if (kprLibraryALImageThumbnailKind == session->kind) {
		err = KprLibraryServerGetMovieAndPictureThumbnail(session);
	}
	else if (kprLibraryALVideoThumbnailKind == session->kind) {
		err = KprLibraryServerGetMovieAndPictureThumbnail(session);
	}
	else if (kprLibraryMPSongThumbnailKind == session->kind) {
		err = KprLibraryServerGetSongThumbnail(session);
	}
	FskInstrumentedItemPrintfNormal(session, "open %s (%d)", session->url, err);
    return err;
}

static void Library_sniffMIMEComplete(KprMessage message, void* it)
{
	KprFunctionTarget self = it;
	char* mime = KprMessageGetResponseHeader(message, "content-type");
	char* size = KprMessageGetResponseHeader(message, "content-length");
	{
		xsBeginHostSandboxCode(self->the, self->code);
		xsVars(2);
		xsVar(0) = (mime) ? xsString(mime) : xsString("");
		xsVar(1) = (size) ? xsString(size) : xsUndefined;
		(void)xsCallFunction2(self->slot, xsGlobal, xsVar(0), xsVar(1));
		xsEndHostSandboxCode();
	}
}

void Library_sniffMIME(xsMachine *the)
{
	xsIntegerValue kind = xsToInteger(xsArg(0));
	if (kprLibraryPodcastKind == kind) {
		KPRLibrarySniffPodcast(the);
	}
	else if ((kprLibraryALImageKind == kind) || (kprLibraryALVideoKind == kind)) {
		KprMessage message;
		KprFunctionTarget target;
		xsArg(1) = xsCall1(xsGlobal, xsID("encodeURIComponent"), xsArg(1));
		xsArg(1) = xsCall1(xsString("xkpr://library/info?url="), xsID("concat"), xsArg(1));
		xsThrowIfFskErr(KprMessageNew(&message, xsToString(xsArg(1))));
		xsThrowIfFskErr(KprFunctionTargetNew(&target, the, &xsArg(2)));
		xsThrowIfFskErr(KprMessageInvoke(message, Library_sniffMIMEComplete, KprFunctionTargetDispose, target));
	}
	else if (kprLibraryMPSongKind == kind) {
		xsResult = xsString("audio/mp4");
        xsStringValue reference = xsToString(xsArg(1));
		MPMediaQuery* query = [[[MPMediaQuery alloc] init] autorelease];
		MPMediaPropertyPredicate *predicate = [MPMediaPropertyPredicate 
			predicateWithValue:[NSNumber numberWithLongLong:FskStrToFskInt64(reference)]
			forProperty: MPMediaItemPropertyPersistentID];
		[query addFilterPredicate:predicate];
		NSArray *items = [query items];
		NSURL* url = ([items count]) ? [[items objectAtIndex:0] valueForProperty: MPMediaItemPropertyAssetURL] : NULL;
		if (url) {
			AVURLAsset* asset = [AVURLAsset URLAssetWithURL: url options:NULL];
			NSArray* audioTracks = [asset tracksWithMediaType:AVMediaTypeAudio];
   		 	if ([audioTracks count]) {
				AVAssetTrack* audioTrack = [audioTracks objectAtIndex:0];
				NSArray *audioFormatDescriptions = [audioTrack formatDescriptions];
				CMAudioFormatDescriptionRef audioFormatDescription = (CMFormatDescriptionRef)[audioFormatDescriptions objectAtIndex:0];
				const AudioStreamBasicDescription *audioStreamBasicDescription = CMAudioFormatDescriptionGetStreamBasicDescription(audioFormatDescription);
				if (audioStreamBasicDescription->mFormatID == kAudioFormatMPEGLayer3) {
					xsResult = xsString("audio/mpeg");
				}
			}
		}
		(void)xsCallFunction1(xsArg(2), xsGlobal, xsResult);
	}
	else {
		(void)xsCallFunction1(xsArg(2), xsGlobal, xsUndefined);
	}
}
