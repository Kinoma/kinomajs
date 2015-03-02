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
#include "kpr.h"
#include "kprImage.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprURL.h"
#include "kprUtilities.h"

#if TARGET_OS_IPHONE

#import <AssetsLibrary/AssetsLibrary.h>
#import <AVFoundation/AVFoundation.h>
#import <AVFoundation/AVAsset.h>
#import <AVFoundation/AVAssetExportSession.h>

static void KprALServiceStart(KprService self, FskThread thread, xsMachine* the);
static void KprALServiceStop(KprService self);
static void KprALServiceCancel(KprService self, KprMessage message);
static void KprALServiceInvoke(KprService self, KprMessage message);

KprServiceRecord gALService = {
	NULL,
	kprServicesThread,
	"assets-library:",
	NULL,
	NULL,
	KprServiceAccept,
	KprALServiceCancel,
	KprALServiceInvoke,
	KprALServiceStart,
	KprALServiceStop,
	NULL,
	NULL,
	NULL
};

typedef struct {
	KprMessage message;
	ALAssetsLibrary* library;
	FskGrowableStorage buffer;
} KprALTargetRecord, *KprALTarget;

FskErr KprALTargetNew(KprALTarget* it, KprMessage message)
{
	KprALTarget self;
	FskErr err = kFskErrNone;
	bailIfError(FskMemPtrNewClear(sizeof(KprALTargetRecord), it));
	self = *it;
	self->message = message;
	self->library = [[ALAssetsLibrary alloc] init];
	bailIfError(FskGrowableStorageNew(1024, &self->buffer));
bail:
	return err;
}

void KprALTargetCancel(KprMessage message, void* it)
{
//	KprALTarget self = it;
}

void KprALTargetDispose(void* it)
{
	KprALTarget self = it;
	FskGrowableStorageDispose(self->buffer);
	[self->library release];
	FskMemPtrDispose(self);
}

static void  KprALTargetEnumerateGroupsCallback(KprALTarget target, ALAssetsGroup *group)
{
	static char* s0 = "{\"name\":\"";
	static char* s1 = "\",\"type\":\"directory\",\"url\":\"";
	static char* s2 = "\"},";
	FskErr err = kFskErrNone;
	KprMessage message = target->message;
	UInt32 length;
	char* pointer;
	char* name =  NULL;
	char* url =  NULL;
	if (group == NULL) {
 		bailIfError(FskGrowableStorageAppendItem(target->buffer, "]", 1));
		length = FskGrowableStorageGetSize(target->buffer);	
		FskGrowableStorageGetPointerToItem(target->buffer, 0, (void **)&pointer);
		bailIfError(FskMemPtrNew(length, &message->response.body));
		FskMemCopy(message->response.body, pointer, length);
		message->response.size = length;
		KprMessageTransform(message, gALService.machine);
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
	}
	else {
		NSString* _name = [group valueForProperty:ALAssetsGroupPropertyName];
		const char* name = [_name UTF8String];
		NSURL* _url = [group valueForProperty:ALAssetsGroupPropertyURL];
		const char* url = [[_url absoluteString] UTF8String];
		bailIfError(FskGrowableStorageAppendItem(target->buffer, s0, FskStrLen(s0)));
		bailIfError(FskGrowableStorageAppendItem(target->buffer, name, FskStrLen(name)));
		bailIfError(FskGrowableStorageAppendItem(target->buffer, s1, FskStrLen(s1)));
		bailIfError(FskGrowableStorageAppendItem(target->buffer, url, FskStrLen(url)));
		bailIfError(FskGrowableStorageAppendItem(target->buffer, s2, FskStrLen(s2)));
 	}
 bail:
	FskMemPtrDispose(url);
	FskMemPtrDispose(name);
}

static void  KprALTargetGroupForURLCallback(KprALTarget target, ALAssetsGroup *group)
{
	static char* s0 = "{\"mime\":\"image/jpeg\",\"type\":\"file\",\"url\":\"";
	static char* s2 = "\"},";
	if (group == NULL) 
		return;
	ALAssetsGroupEnumerationResultsBlock enumerationBlock = ^(ALAsset *asset, NSUInteger index, BOOL *stop) {
		FskErr err = kFskErrNone;
		if (asset == NULL) {
			FskErr err = kFskErrNone;
			KprMessage message = target->message;
			UInt32 length;
			char* pointer;
			bailIfError(FskGrowableStorageAppendItem(target->buffer, "]", 1));
			length = FskGrowableStorageGetSize(target->buffer);	
			FskGrowableStorageGetPointerToItem(target->buffer, 0, (void **)&pointer);
			bailIfError(FskMemPtrNew(length, &message->response.body));
			FskMemCopy(message->response.body, pointer, length);
			message->response.size = length;
			KprMessageTransform(message, gALService.machine);
			FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
		}
		else {
			ALAssetRepresentation *defaultRepresentation = [asset defaultRepresentation];
			NSString *uti = [defaultRepresentation UTI];
			NSURL *_url = [[asset valueForProperty:ALAssetPropertyURLs] valueForKey:uti];
			const char* url = [[_url absoluteString] UTF8String];
			bailIfError(FskGrowableStorageAppendItem(target->buffer, s0, FskStrLen(s0)));
			bailIfError(FskGrowableStorageAppendItem(target->buffer, url, FskStrLen(url)));
			bailIfError(FskGrowableStorageAppendItem(target->buffer, s2, FskStrLen(s2)));
		}
 bail:
 		return;
	};
 	FskGrowableStorageAppendItem(target->buffer, "[", 1);
	[group enumerateAssetsUsingBlock:enumerationBlock];
}

static void  KprALTargetAssetForURLCallback(KprALTarget target, ALAsset *asset)
{
	FskErr err = kFskErrNone;
	KprMessage message = target->message;
	KprImageTarget stream = (KprImageTarget)message->stream;
	char* directory = NULL;
	char* path = NULL;
	char* url = NULL;
	if (stream) {
		CGImageRef imageRef;
		if (stream->width && stream->height)
			imageRef = [asset thumbnail];
		else
			imageRef = [[asset defaultRepresentation] fullScreenImage];
		bailIfNULL(imageRef);
		FskRectangleRecord srcRect, dstRect, tmpRect;
		FskRectangleSet(&srcRect, 0, 0, CGImageGetWidth(imageRef), CGImageGetHeight(imageRef));
		if (stream->width && stream->height) {
			FskRectangleSet(&dstRect, 0, 0, stream->width, stream->height);
			if (stream->aspect & kprImageFit)
				FskRectangleScaleToFit(&srcRect, &dstRect, &tmpRect);
			else
				FskRectangleScaleToFill(&srcRect, &dstRect, &tmpRect);
			dstRect = tmpRect;
		}
		else
			dstRect = srcRect;
		bailIfError(FskBitmapNew(dstRect.width, dstRect.height, kFskBitmapFormatDefaultNoAlpha, &stream->bitmap));
		FskBitmapWriteBegin(stream->bitmap, NULL, NULL, NULL);
		CGContextDrawImage(FskBitmapGetNativeBitmap(stream->bitmap), CGRectMake(dstRect.x, dstRect.y, dstRect.width, dstRect.height), imageRef);
		FskBitmapWriteEnd(stream->bitmap);
    }
    else {
		FskFileInfo info;
		ALAssetRepresentation *representation = [asset defaultRepresentation];
		FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeTemporary, true, NULL, &directory);
		path = FskStrDoCat(directory, [[representation filename] UTF8String]);
		url = FskStrDoCat("file://", path);
		KprMessageSetResponseHeader(message, "location", url);
		KprMessageSetResponseHeader(message, "content-type", "video/mp4");
		if (kFskErrNone != FskFileGetFileInfo(path, &info)) {
			AVURLAsset* asset = [AVURLAsset URLAssetWithURL:representation.url options:nil];
			AVAssetExportSession *exporter = [[AVAssetExportSession alloc] initWithAsset: asset presetName: AVAssetExportPresetMediumQuality];
			exporter.outputURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
			exporter.shouldOptimizeForNetworkUse = YES;
			exporter.outputFileType = AVFileTypeMPEG4;

			[exporter exportAsynchronouslyWithCompletionHandler: ^(void) {
				switch ([exporter status]) {
				case AVAssetExportSessionStatusFailed:
					message->error = kFskErrOperationFailed;
					break;
				case AVAssetExportSessionStatusCancelled:
					message->error = kFskErrOperationCancelled;
					break;
				default:
					break;
				}
				FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
			}];
			err = kFskErrNeedMoreTime;
		}
    }
bail:
	FskMemPtrDispose(url);
	FskMemPtrDispose(path);
	FskMemPtrDispose(directory);
	if (kFskErrNeedMoreTime != err)
		FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
}

void KprALServiceStart(KprService self, FskThread thread, xsMachine* the)
{
	self->machine = the;
	self->thread = thread;
}

void KprALServiceStop(KprService self UNUSED)
{
}

void KprALServiceCancel(KprService service UNUSED, KprMessage message UNUSED)
{
}

void KprALServiceInvoke(KprService service UNUSED, KprMessage message)
{
	FskErr err = kFskErrNone;
	KprALTarget target;
	if (KprMessageContinue(message)) {
		bailIfError(KprALTargetNew(&target, message));
		if ((message->parts.authorityLength == 8) && (!FskStrCompareWithLength(message->parts.authority, "pictures", 8))) {
			ALAssetsLibraryGroupsEnumerationResultsBlock usingBlock = ^(ALAssetsGroup *group, BOOL *stop) {
				KprALTargetEnumerateGroupsCallback(target, group);
			};
			ALAssetsLibraryAccessFailureBlock failureBlock = ^(NSError *error) {
				KprALTargetEnumerateGroupsCallback(target, NULL);
			};
			NSUInteger groupTypes = ALAssetsGroupAll;
			bailIfError(FskGrowableStorageAppendItem(target->buffer, "[", 1));
			[target->library enumerateGroupsWithTypes:groupTypes usingBlock:usingBlock failureBlock:failureBlock];
		}
		else if ((message->parts.authorityLength == 5) && (!FskStrCompareWithLength(message->parts.authority, "group", 5))) {
			ALAssetsLibraryGroupResultBlock resultBlock = ^(ALAssetsGroup *group) {
				KprALTargetGroupForURLCallback(target, group);
			};
			ALAssetsLibraryAccessFailureBlock failureBlock = ^(NSError *error) {
				KprALTargetGroupForURLCallback(target, NULL);
			};
			[target->library groupForURL:[NSURL URLWithString:[NSString stringWithUTF8String:message->url]] resultBlock:resultBlock failureBlock:failureBlock];
		}
		else if ((message->parts.authorityLength == 5) && (!FskStrCompareWithLength(message->parts.authority, "asset", 5))) {
			ALAssetsLibraryAssetForURLResultBlock resultBlock = ^(ALAsset *asset) {
				KprALTargetAssetForURLCallback(target, asset);
			};
			ALAssetsLibraryAccessFailureBlock failureBlock = ^(NSError *error) {
				KprALTargetAssetForURLCallback(target, NULL);
			};
			[target->library assetForURL:[NSURL URLWithString:[NSString stringWithUTF8String:message->url]] resultBlock:resultBlock failureBlock:failureBlock];
		}
		else
			err = kFskErrUnimplemented;
	bail:
		if (err) {
			message->error = err;
			FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
		}
	}
}

#endif
