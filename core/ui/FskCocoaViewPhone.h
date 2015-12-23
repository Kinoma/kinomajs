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
#import "FskWindow.h"
#import "xs.h"

#define MULTI_TOUCHES 1
#define TEXT_INPUT_SYSTEM   1

#pragma mark TextStorage

#if TEXT_INPUT_SYSTEM

@protocol CocoaTextStorageDeleage <NSObject>

- (void)storageChangedInRange:(NSRange)range withText:(NSString *)text;

@end

@interface CocoaTextStorage : NSObject

@property (nonatomic, assign) id<CocoaTextStorageDeleage> delegate;
@property (nonatomic, readonly) NSString *text;
@property (nonatomic, assign) NSRange selectedRange;
@property (nonatomic, readonly, assign) NSRange markedRange;
@property (nonatomic, readonly) NSUInteger length;

- (void)unmark;
- (BOOL)isMarked;
- (NSUInteger)caretPosition;
- (void)setCaretPosition:(NSUInteger)pos;
- (BOOL)hasSelection;

- (BOOL)isInTextRange:(NSInteger)pos;

- (NSString *)text;
- (NSString *)subtextWithRange:(NSRange)range;
- (void)setText:(NSString *)text withSelectedRange:(NSRange)range;

- (void)insertText:(NSString *)text;
- (void)deleteBackword;
- (void)insertMarkedText:(NSString *)text withSelection:(NSRange)selection;

@end

#endif

@interface FskCocoaView : UIView
#if TEXT_INPUT_SYSTEM
<UITextInput>
#else
<UIKeyInput>
#endif

@property (nonatomic, readonly) BOOL isMainView;
@property (nonatomic, assign) BOOL keyboardActive;
@property(nonatomic) UIReturnKeyType returnKeyType;
#if TEXT_INPUT_SYSTEM
@property (nonatomic, readonly) BOOL keyboardUpdating;
@property (nonatomic, assign) xsMachine *machine;
@property (nonatomic, assign) xsSlot obj;
@property (nonatomic, assign) UITextAutocapitalizationType autocapitalizationType;
@property (nonatomic, assign) UITextAutocorrectionType autocorrectionType;
@property (nonatomic, assign) UIKeyboardType keyboardType;
#endif

	// init
- (id)initWithFrame:(CGRect)frame screenScale:(CGFloat)screenScale isMainView:(BOOL)isMainView;

	// getters/setters
- (FskWindow)fskWindow;
- (void)setFskWindow:(FskWindow)fskWindow;
#if !kFskCocoaCopyBitsUseOpenGL && !FSKBITMAP_OPENGL
- (void)setWindowClipCGPath:(CGMutablePathRef)windowClipCGPath;
#endif
- (FskPointRecord)lastTouchedPoint;

	// NSView overrides
#if !kFskCocoaCopyBitsUseOpenGL && !FSKBITMAP_OPENGL
- (void)drawRect:(CGRect)rect;
#endif
- (BOOL)canBecomeFirstResponder;
- (BOOL)canResignFirstResponder;
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event;

	// methods
- (void)copyBits:(FskBitmap)fskBitmap sourceRect:(FskRectangle)sourceRect destRect:(FskRectangle)destRect;
#if !kFskCocoaCopyBitsUseOpenGL && !FSKBITMAP_OPENGL
- (void)clipWindow;
- (void)clearBitmapContextWithRect:(CGRect)destCGRect;
#endif
- (void)handleTouches:(NSSet *)touches;

-(void)beginDraw;
-(void)endDraw;

- (void)keyboardDidHide:(NSNotification *)notification;
- (void)keyboardDidShow:(NSNotification *)notification;
- (void)keyboardWillHide:(NSNotification *)notification;
- (void)keyboardWillShow:(NSNotification *)notification;

- (void)resetStorage;
- (void)setStorageText:(NSString *)text selection:(NSRange)range;

@end
