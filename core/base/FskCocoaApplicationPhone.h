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
#import <UIKit/UIKit.h>
#import "Fsk.h"

@class FskCocoaViewController;

extern NSString *FskCocoaApplicationViewOrientationDidChangeNotification;

@interface FskCocoaApplication : NSObject <UIApplicationDelegate>

@property (nonatomic, readonly) FskCocoaViewController *mainViewController;

+ (FskCocoaApplication *)sharedApplication;

	// methods
- (void)runFskRunLoop;

#if USE_TEXT_INPUT_VIEW
- (void)showTextInputWithTitle:(NSString*)title initialValue:(NSString*)initialValue machine:(void*)the;
- (void)textEntered:(NSString*)textEntered machine:(void*)the;;
#endif

- (void)startViewOrientationNotification;
- (void)stopViewOrientationNotification;
- (SInt32)viewRotation;
- (FskCocoaViewController *)getEmptyController;

#if SUPPORT_REMOTE_NOTIFICATION
- (void)handleRemoteNotification;
#endif	/* SUPPORT_REMOTE_NOTIFICATION */

#if SUPPORT_EXTERNAL_SCREEN
- (void)addExternalScreen:(UIScreen *)extScreen;
- (void)removeExternalScreen:(UIScreen *)extScreen;
- (void)updateExternalScreen:(UIScreen *)extScreen;
#if TEST_EXTERNAL_SCREEN
- (FskCocoaViewController *)getExternalViewControllerAtIndex:(NSUInteger)index;
#endif
#endif

@end
