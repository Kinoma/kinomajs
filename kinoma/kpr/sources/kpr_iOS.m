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
#import <MediaPlayer/MediaPlayer.h>

#import "kpr_iOS.h"

#import "FskCocoaApplicationPhone.h"
#import "FskCocoaViewControllerPhone.h"

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

/*
 * Metadata updater
 */

@interface KprSystemNowPlayingInfo : NSObject

+ (KprSystemNowPlayingInfo *)sharedInstance;

@property (nonatomic, readonly) BOOL idling;
@property(retain, atomic) NSDictionary *metadata;

- (void)reset;
- (void)update:(NSDictionary *)info append:(BOOL)append;

- (void)startIdling;
- (void)stopIdling;

@end

void
KprSystemNowPlayingInfoSetIdling(Boolean idling)
{
	KprSystemNowPlayingInfo *updater = [KprSystemNowPlayingInfo sharedInstance];

	if (idling)
	{
		if (!updater.idling)
		{
			[updater startIdling];
		}
	}
	else
	{
		if (updater.idling)
		{
			[updater stopIdling];
		}
	}
}

Boolean
KprSystemNowPlayingInfoGetIdling(void)
{
	return [KprSystemNowPlayingInfo sharedInstance].idling ? true : false;
}

static void
appendArtworkToInfo(NSData *data, NSMutableDictionary *info)
{
	if (data != nil) {
		UIImage *image = [UIImage imageWithData:data];
		if (image) {
			MPMediaItemArtwork *artwork = [[MPMediaItemArtwork alloc] initWithImage:image];
			info[MPMediaItemPropertyArtwork] = artwork;
			[artwork release];
		}
	}
}

void
KprSystemNowPlayingInfoSetMetadata(KprMedia media, FskMediaPropertyValue artwork)
{
	NSMutableDictionary *info;

	if (media == NULL)
	{
		[[KprSystemNowPlayingInfo sharedInstance] update:nil append:NO];
		return;
	}

	info = [NSMutableDictionary dictionaryWithCapacity:10];

	if (media->artist) {
		info[MPMediaItemPropertyArtist] = [NSString stringWithCString:media->artist encoding:NSUTF8StringEncoding];
	}

	if (media->album) {
		info[MPMediaItemPropertyAlbumTitle] = [NSString stringWithCString:media->album encoding:NSUTF8StringEncoding];
	}

	if (media->title) {
		info[MPMediaItemPropertyTitle] = [NSString stringWithCString:media->title encoding:NSUTF8StringEncoding];
	}

	if (media->duration) {
		info[MPMediaItemPropertyPlaybackDuration] = @(media->duration);
	}

	if (media->cover && (artwork->type == kFskMediaPropertyTypeImage)) {
		char* mime = (char*)artwork->value.data.data;
		UInt32 mimeLen = FskStrLen(mime) + 1;
		NSData *data = [NSData dataWithBytes:artwork->value.data.data + mimeLen length:artwork->value.data.dataSize - mimeLen];
		appendArtworkToInfo(data, info);
	}

	[[KprSystemNowPlayingInfo sharedInstance] update:info append:NO];
}

void
KprSystemNowPlayingInfoSetUPnPMetadata(KprUPnPMetadata metadata)
{
	NSMutableDictionary *info;

	if (metadata == NULL)
	{
		[[KprSystemNowPlayingInfo sharedInstance] update:nil append:NO];
		return;
	}

	info = [NSMutableDictionary dictionaryWithCapacity:10];

	if (metadata->artist) {
		info[MPMediaItemPropertyArtist] = [NSString stringWithCString:metadata->artist encoding:NSUTF8StringEncoding];
	}

	if (metadata->album) {
		info[MPMediaItemPropertyAlbumTitle] = [NSString stringWithCString:metadata->album encoding:NSUTF8StringEncoding];
	}

	if (metadata->title) {
		info[MPMediaItemPropertyTitle] = [NSString stringWithCString:metadata->title encoding:NSUTF8StringEncoding];
	}

	if (metadata->creator) {
		info[MPMediaItemPropertyComposer] = [NSString stringWithCString:metadata->creator encoding:NSUTF8StringEncoding];
	}

	if (metadata->genre) {
		info[MPMediaItemPropertyGenre] = [NSString stringWithCString:metadata->genre encoding:NSUTF8StringEncoding];
	}

	if (metadata->track) {
		info[MPMediaItemPropertyAlbumTrackNumber] = @(metadata->track);
	}

	if (metadata->duration) {
		info[MPMediaItemPropertyPlaybackDuration] = @(metadata->duration);
	}

	if (metadata->artworkUri) {
		NSURL *url = [NSURL URLWithString:[NSString stringWithCString:metadata->artworkUri encoding:NSUTF8StringEncoding]];
		NSURLRequest *request = [NSURLRequest requestWithURL:url];
		[NSURLConnection sendAsynchronousRequest:(NSURLRequest*) request queue:[NSOperationQueue mainQueue] completionHandler:^(NSURLResponse* response, NSData* data, NSError* connectionError) {
			if (connectionError == nil) {
				appendArtworkToInfo(data, info);
			}
		}];
	}

	[[KprSystemNowPlayingInfo sharedInstance] update:info append:NO];
}

void
KprSystemNowPlayingInfoSetNativeMetadata(void *metadataIn, Boolean append)
{
	NSMutableDictionary *info = (NSMutableDictionary *)metadataIn;

	[[KprSystemNowPlayingInfo sharedInstance] update:info append:append];
}

void
KprSystemNowPlayingInfoSetTime(double duration, double position)
{
	NSDictionary *info = @{MPMediaItemPropertyPlaybackDuration: @(duration), MPNowPlayingInfoPropertyElapsedPlaybackTime: @(position), MPNowPlayingInfoPropertyPlaybackRate: @1.0};
	[[KprSystemNowPlayingInfo sharedInstance] update:info append:YES];
}

@implementation KprSystemNowPlayingInfo {
	NSTimer *_timer;
}

static KprSystemNowPlayingInfo *metadataUpdater = NULL;

+ (KprSystemNowPlayingInfo *)sharedInstance
{
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		metadataUpdater = [[KprSystemNowPlayingInfo alloc] init];
	});
	return metadataUpdater;
}

- (id)init
{
	self = [super init];
	if (self) {
		[self reset];
	}
	return self;
}

- (void)reset
{
	self.metadata = @{};
}

- (void)update:(NSDictionary *)info append:(BOOL)append
{
	if (append == NO) {
		[self reset];
	}

	NSMutableDictionary	*metadata = [NSMutableDictionary dictionaryWithDictionary:self.metadata];
	[metadata addEntriesFromDictionary:info];
	self.metadata = metadata;

	[MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = metadata;
}

- (void)startIdling
{
	if (_timer) return;

	_timer = [[NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(idle:) userInfo:nil repeats:YES] retain];
	_idling = YES;
}

- (void)stopIdling
{
	if (_timer == nil) return;

	[_timer invalidate];
	[_timer release];
	_timer = nil;
	_idling = NO;
}

- (void)idle:(NSTimer *)timer
{
	FskCocoaViewController *viewController = [FskCocoaApplication sharedApplication].mainViewController;
	if ([viewController isDisplayLinkActive] == NO) {
		FskEvent fskEvent;
		FskTimeRecord updateTime;
		FskWindow win = viewController.fskWindow;

	    FskTimeGetNow(&updateTime);
		FskErr err = FskEventNew(&fskEvent, kFskEventWindowBeforeUpdate, &updateTime, kFskEventModifierNotSet);
		if (err == kFskErrNone) FskWindowEventSend(win, fskEvent);
	}
}

- (void)dealloc
{
	self.metadata = nil;

	[self stopIdling];

    [super dealloc];
}

@end

/*
 * Tone Generator
 */
@interface ToneGenerator : NSObject

+ (ToneGenerator *)sharedInstance;

@property(assign) double frequency;
@property(assign) double sampleRate;
@property(assign) double amplitude;

- (BOOL)play;
- (void)stop;
- (BOOL)isPlaying;

@end

void KprToneGeneratorPlay()
{
	[[ToneGenerator sharedInstance] play];
}

void KprToneGeneratorStop()
{
	[[ToneGenerator sharedInstance] stop];
}

@implementation ToneGenerator {
	AudioComponentInstance toneUnit;
	double theta;
	BOOL _playing;
}

+ (ToneGenerator *)sharedInstance
{
	static ToneGenerator *shared = nil;
	static dispatch_once_t token;
	dispatch_once(&token, ^{
		shared = [[ToneGenerator alloc] init];
		shared.frequency = 220;
		shared.amplitude = 0;
	});
	return shared;
}

- (instancetype)init
{
	self = [super init];
	if (self) {
		_frequency = 440;
		_amplitude = 0.25;
		_sampleRate = 44100;

		// Configure the search parameters to find the default playback output unit
		// (called the kAudioUnitSubType_RemoteIO on iOS but
		// kAudioUnitSubType_DefaultOutput on Mac OS X)
		AudioComponentDescription defaultOutputDescription;
		defaultOutputDescription.componentType = kAudioUnitType_Output;
		defaultOutputDescription.componentSubType = kAudioUnitSubType_RemoteIO;
		defaultOutputDescription.componentManufacturer = kAudioUnitManufacturer_Apple;
		defaultOutputDescription.componentFlags = 0;
		defaultOutputDescription.componentFlagsMask = 0;

		// Get the default playback output unit
		AudioComponent defaultOutput = AudioComponentFindNext(NULL, &defaultOutputDescription);
		NSAssert(defaultOutput, @"Can't find default output");

		// Create a new unit based on this that we'll use for output
		OSStatus err = AudioComponentInstanceNew(defaultOutput, &toneUnit);
		NSAssert1(toneUnit, @"Error creating unit: %d", (int)err);

		// Set our tone rendering function on the unit
		AURenderCallbackStruct input;
		input.inputProc = RenderTone;
		input.inputProcRefCon = (__bridge void *)(self);
		err = AudioUnitSetProperty(toneUnit,
								   kAudioUnitProperty_SetRenderCallback,
								   kAudioUnitScope_Input,
								   0,
								   &input,
								   sizeof(input));
		NSAssert1(err == noErr, @"Error setting callback: %d", (int)err);

		// Set the format to 32 bit, single channel, floating point, linear PCM
		const int four_bytes_per_float = 4;
		const int eight_bits_per_byte = 8;
		AudioStreamBasicDescription streamFormat;
		streamFormat.mSampleRate = self.sampleRate;
		streamFormat.mFormatID = kAudioFormatLinearPCM;
		streamFormat.mFormatFlags =
		kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
		streamFormat.mBytesPerPacket = four_bytes_per_float;
		streamFormat.mFramesPerPacket = 1;
		streamFormat.mBytesPerFrame = four_bytes_per_float;
		streamFormat.mChannelsPerFrame = 1;
		streamFormat.mBitsPerChannel = four_bytes_per_float * eight_bits_per_byte;
		err = AudioUnitSetProperty (toneUnit,
									kAudioUnitProperty_StreamFormat,
									kAudioUnitScope_Input,
									0,
									&streamFormat,
									sizeof(AudioStreamBasicDescription));
		NSAssert1(err == noErr, @"Error setting stream format: %d", (int)err);

		// Stop changing parameters on the unit
		err = AudioUnitInitialize(toneUnit);
		NSAssert1(err == noErr, @"Error initializing unit: %d", (int)err);
	}
	return self;
}

- (void)dealloc
{
	if ([self isPlaying]) [self stop];
	AudioUnitUninitialize(toneUnit);
	AudioComponentInstanceDispose(toneUnit);
	[super dealloc];
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"Tone %4.1f Hz x %d%%", self.frequency, (int)(self.amplitude * 100)];
}

- (BOOL)play
{
	// Start playback
	OSStatus err = AudioOutputUnitStart(toneUnit);
	NSAssert1(err == noErr, @"Error starting unit: %d", (int)err);
	_playing = (err == noErr);
	return _playing;
}

- (void)stop
{
	AudioOutputUnitStop(toneUnit);
	_playing = NO;
}

- (BOOL)isPlaying
{
	return _playing;
}

- (void)renderIntoBufferList:(AudioBufferList *)bufferList count:(UInt32)count
{
	// Fixed amplitude is good enough for our purposes
	const double amplitude = self.amplitude;

	// This is a mono tone generator so we only need the first buffer
	const int channel = 0;

	Float32 *buffer = (Float32 *)bufferList->mBuffers[channel].mData;

	if (amplitude > 0) {
		const double theta_increment = 2.0 * M_PI * self.frequency / self.sampleRate;

		// Generate the samples
		for (UInt32 frame = 0; frame < count; frame++) {
			buffer[frame] = sin(theta) * amplitude;

			theta += theta_increment;
			if (theta > 2.0 * M_PI) {
				theta -= 2.0 * M_PI;
			}
		}
	} else {
		memset(buffer, 0, count);
	}
}

static OSStatus RenderTone(
						   void *inRefCon,
						   AudioUnitRenderActionFlags 	*ioActionFlags,
						   const AudioTimeStamp 		*inTimeStamp,
						   UInt32 						inBusNumber,
						   UInt32 						inNumberFrames,
						   AudioBufferList 			*ioData)
{
	// Get the tone parameters out of the view controller
	ToneGenerator *generator = (__bridge ToneGenerator *)inRefCon;
	[generator renderIntoBufferList:ioData count:inNumberFrames];
	return noErr;
}

@end

#endif
