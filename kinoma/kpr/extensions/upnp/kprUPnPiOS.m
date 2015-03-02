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
#include "FskCocoaSupportPhone.h"
#include "FskCocoaViewControllerPhone.h"
#include "FskCocoaSupportCommon.h"
#include "FskTime.h"

#include "kpr.h"
#include "kprUPnP.h"
#include "kprUtilities.h"

#import <UIKit/UIKit.h>
#import <AudioUnit/AudioUnit.h>
#import <AVFoundation/AVFoundation.h>
#import <MediaPlayer/MediaPlayer.h>
#import <dispatch/dispatch.h>

static BOOL idling = NO;

@interface ToneGenerator : NSObject

+ (ToneGenerator *)sharedInstance;

@property(assign) double frequency;
@property(assign) double sampleRate;
@property(assign) double amplitude;

- (BOOL)play;
- (void)stop;
- (BOOL)isPlaying;

@end

@interface KprUPnPMetadataUpdater : NSObject

+ (KprUPnPMetadataUpdater *)sharedInstance;

@property(retain, atomic) NSDictionary *metadata;

- (void)update:(NSDictionary *)info append:(BOOL)append;

- (void)startIdling;
- (void)stopIdling;

@end

static BOOL gUPnPNeedPositionUpdate;

void KprUPnPControllerServiceActionCallbackPlay_iOS(KprUPnPController self, const char* serviceType)
{
	// NSLog(@"========= START PLAYING ============");
	
	FskCocoaAudioSessionSetupFakePlaying();
	
	[[ToneGenerator sharedInstance] play];
	
	[[KprUPnPMetadataUpdater sharedInstance] startIdling];
}

void KprUPnPControllerServiceActionCallbackStop_iOS(KprUPnPController self, const char* serviceType)
{
	// NSLog(@"========= STOP PLAYING ============");
	
	[[ToneGenerator sharedInstance] stop];
	
	[[KprUPnPMetadataUpdater sharedInstance] stopIdling];

	FskCocoaAudioSessionTearDown();
}

void KprUPnPControllerServiceActionCallbackGetPositionInfo_iOS(KprUPnPController self, const char* serviceType, double duration, double position)
{
	if (gUPnPNeedPositionUpdate) {
		NSDictionary *info = @{MPMediaItemPropertyPlaybackDuration: @(duration), MPNowPlayingInfoPropertyElapsedPlaybackTime: @(position), MPNowPlayingInfoPropertyPlaybackRate: @1.0};
		[[KprUPnPMetadataUpdater sharedInstance] update:info append:YES];
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
	KprUPnPMetadataUpdater *updater = [KprUPnPMetadataUpdater sharedInstance];
	
	NSMutableDictionary *info = [NSMutableDictionary dictionaryWithCapacity:10];
	
	gUPnPNeedPositionUpdate = YES;
	
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
				UIImage *image = [UIImage imageWithData:data];
				if (image) {
					MPMediaItemArtwork *artwork = [[MPMediaItemArtwork alloc] initWithImage:image];
					[updater update:@{MPMediaItemPropertyArtwork:artwork} append:YES];
					[artwork release];
				}
			}
		}];
	}
	
	[updater update:info append:NO];
}

void KprUPnPControllerUtility_iOS(KprUPnPController self, char* actionName)
{
	if (FskStrCompare(actionName, "finalize") == 0) {
		[[ToneGenerator sharedInstance] stop];

		[[KprUPnPMetadataUpdater sharedInstance] stopIdling];

		[[KprUPnPMetadataUpdater sharedInstance] update:nil append:NO];

		FskCocoaAudioSessionTearDown();
	}
}

@implementation KprUPnPMetadataUpdater {
	NSTimer *_timer;
}

static KprUPnPMetadataUpdater *metadataUpdater = NULL;

+ (KprUPnPMetadataUpdater *)sharedInstance
{
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		metadataUpdater = [[KprUPnPMetadataUpdater alloc] init];
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
	idling = YES;
}

- (void)stopIdling
{
	if (_timer == nil) return;
	
	[_timer invalidate];
	[_timer release];
	_timer = nil;
	idling = NO;
}

- (void)idle:(NSTimer *)timer
{
	FskCocoaViewController *viewController = [FskCocoaViewController sharedViewController];
	if ([viewController isDisplayLinkActive] == NO) {
		FskEvent fskEvent;
		FskTimeRecord updateTime;
		FskWindow win = FskWindowGetActive();
		
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

void UPnP_Controller_isBackgroundPlaying(xsMachine *the)
{	
	if (idling)
		xsResult = xsTrue;
	else
		xsResult = xsFalse;
}
