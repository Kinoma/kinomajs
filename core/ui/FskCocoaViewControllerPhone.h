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
#import <QuartzCore/QuartzCore.h>

#if SUPPORT_EXTERNAL_SCREEN && TEST_EXTERNAL_SCREEN
#import "FskBitmap.h"
#endif	/* SUPPORT_EXTERNAL_SCREEN && TEST_EXTERNAL_SCREEN */
#import "FskWindow.h"

@class FskCocoaView;

@interface FskCocoaViewController : UIViewController

@property (nonatomic, readonly) FskCocoaView *cocoaView;
@property (nonatomic, readonly) FskWindow fskWindow;
@property (nonatomic, readonly) BOOL empty;

@property (nonatomic, assign) BOOL notifyViewOrientation;
@property (nonatomic, readonly) CGFloat screenScale;

#if SUPPORT_EXTERNAL_SCREEN && TEST_EXTERNAL_SCREEN
@property (nonatomic, assign) FskBitmap bitmap;
#endif	/* TEST_EXTERNAL_SCREEN && SUPPORT_EXTERNAL_SCREEN */

- (id)initWithFrame:(CGRect)frame screenScale:(CGFloat)screenScale;

- (void) setDisplayLinkActive:(BOOL)flag;
- (void) pauseDisplayLink:(BOOL)flag;
- (BOOL) isDisplayLinkRunning;
- (BOOL) isDisplayLinkActive;

- (void)throwKeyEvent:(char)keyCode keyDown:(BOOL)down functionKey:(UInt32)functionKey;

@end
