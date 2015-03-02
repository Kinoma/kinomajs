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
#define __FSKWINDOW_PRIV__

#import "FskCocoaWebView.h"
#import "FskCocoaSupportPhone.h"
#include "FskWindow.h"

#if TARGET_OS_IPHONE
	#define IS_MAC 0
	#define IS_IOS 1

	#import <UIKit/UIKit.h>
	#define WebView UIWebView

#elif TARGET_OS_MAC
	#define IS_MAC 1
	#define IS_IOS 0

	#import <WebKit/WebKit.h>
#else
	#error "Unknown target. This place is Cocoa only."
#endif

#if IS_MAC

@interface WebViewWindow : NSPanel

@end

@interface WebViewJSInterface : NSObject

- (void)log:(NSString *)message;

@end

#endif

@interface CocoaWebController : NSObject
#if IS_IOS
<UIWebViewDelegate>
#endif
{
	FskBrowser _browser;
#if IS_MAC
	WebView *_view;
	WebViewWindow *_window;
	NSWindow *_parent;
	NSRect _frame;

	WebViewJSInterface *_jsInterface;
#else
	UIWebView *_view;
	FskWindow _window;
#endif
}

@property(nonatomic, assign) FskBrowser browser;
@property(readonly) WebView *view;

- (void)activated:(BOOL)activateIt;
- (void)setFrame:(FskRectangle)rect;
- (void)attachToWindow:(FskWindow)window;
- (void)detachFromWindow;
- (NSString *)getURL;
- (void)setURL:(NSString *)url;
- (NSString *)evaluateScript:(NSString *)script;
- (void)reload;
- (void)goBack;
- (void)goForward;
- (BOOL)canBack;
- (BOOL)canForward;

@end

#define NSSTR(str) [NSString stringWithCString:(str) encoding:NSUTF8StringEncoding]

FskErr FskCocoaWebViewCreate(FskBrowser browser)
{
	CocoaWebController *controller = [[CocoaWebController alloc] init];
	if (!controller) return kFskErrMemFull;
	
	controller.browser = browser;
	browser->native = controller;
	
	return kFskErrNone;
}

void FskCocoaWebViewDispose(FskCocoaWebView fskWebView)
{
	CocoaWebController *controller = (CocoaWebController *)fskWebView;
	[controller release];
}

void FskCocoaWebViewActivated(FskCocoaWebView fskWebView, Boolean activeIt)
{
	CocoaWebController *controller = (CocoaWebController *)fskWebView;
	[controller activated:activeIt];
}

void FskCocoaWebViewSetFrame(FskCocoaWebView fskWebView, FskRectangle area)
{
	CocoaWebController *controller = (CocoaWebController *)fskWebView;
	
	[controller setFrame:area];
}

void FskCocoaWebViewAttach(FskCocoaWebView fskWebView, FskWindow fskWindow)
{
	CocoaWebController *controller = (CocoaWebController *)fskWebView;
	[controller attachToWindow:fskWindow];
}

void FskCocoaWebViewDetach(FskCocoaWebView fskWebView)
{
	CocoaWebController *controller = (CocoaWebController *)fskWebView;
	
	[controller detachFromWindow];
}

char *FskCocoaWebViewGetURL(FskCocoaWebView fskWebView)
{
	CocoaWebController *controller = (CocoaWebController *)fskWebView;
	return FskStrDoCopy((char *)[[controller getURL] UTF8String]);
}

void FskCocoaWebViewSetURL(FskCocoaWebView fskWebView, char *url)
{
	CocoaWebController *controller = (CocoaWebController *)fskWebView;
	[controller setURL:NSSTR(url)];
}

char *FskCocoaWebViewEvaluateScript(FskCocoaWebView fskWebView, char *script)
{
	CocoaWebController *controller = (CocoaWebController *)fskWebView;
	return FskStrDoCopy((char *)[[controller evaluateScript:NSSTR(script)] UTF8String]);
}

void FskCocoaWebViewReload(FskCocoaWebView fskWebView)
{
	CocoaWebController *controller = (CocoaWebController *)fskWebView;
	[controller reload];
}

void FskCocoaWebViewBack(FskCocoaWebView fskWebView)
{
	CocoaWebController *controller = (CocoaWebController *)fskWebView;
	[controller goBack];
}

void FskCocoaWebViewForward(FskCocoaWebView fskWebView)
{
	CocoaWebController *controller = (CocoaWebController *)fskWebView;
	[controller goForward];
}

Boolean FskCocoaWebViewCanBack(FskCocoaWebView fskWebView)
{
	CocoaWebController *controller = (CocoaWebController *)fskWebView;
	return [controller canBack];
}

Boolean FskCocoaWebViewCanForward(FskCocoaWebView fskWebView)
{
	CocoaWebController *controller = (CocoaWebController *)fskWebView;
	return [controller canForward];
}

@implementation CocoaWebController

@synthesize browser=_browser;

- (id)init
{
	self = [super init];
	if (self) {
#if IS_MAC
		_view = [[WebView alloc] initWithFrame:NSZeroRect];
		_view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
		_view.UIDelegate = self;
		_view.policyDelegate = self;
		_view.frameLoadDelegate = self;

#else
		_view = [[UIWebView alloc] initWithFrame:CGRectZero];
		_view.delegate = self;
#endif
	}
	
	return self;
}

- (void)dealloc
{
#if IS_MAC
	[_jsInterface release];

	_view.frameLoadDelegate = nil;
	_view.policyDelegate = nil;
	_view.UIDelegate = nil;
#else
	_view.delegate = nil;
#endif
	[_view release];
	[super dealloc];
}

- (WebView *)view
{
	return _view;
}

- (void)activated:(BOOL)activateIt
{
#if IS_MAC
	if (activateIt) {
		[_parent addChildWindow:_window ordered:NSWindowAbove];
	} else {
		[_parent removeChildWindow:_window];
		[_window close];
	}
#else
	_view.hidden = (activateIt == NO);
#endif
}

- (void)setFrame:(FskRectangle)area
{
#if IS_MAC
	NSRect frame = [[_window parentWindow] frame];
	frame = [[_window parentWindow] contentRectForFrameRect:frame];
	
	frame.origin.x += area->x;
	frame.origin.y += frame.size.height - area->y - area->height;
	
	frame.size = NSMakeSize(area->width, area->height);
	
	_frame = frame;
	[_window setFrame:_frame display:NO];
#else
	UInt32 statusBarHeight = FskCocoaApplicationGetStatusBarHeight();
	CGRect frame = CGRectMake(area->x, statusBarHeight + area->y, area->width, area->height);
	_view.frame = frame;
#endif
}

- (void)attachToWindow:(FskWindow)fskWindow
{
#if IS_MAC
	NSWindow *window = (NSWindow *)(fskWindow->nsWindow);
	
	_window = [[WebViewWindow alloc] initWithContentRect:_frame styleMask:NSUtilityWindowMask backing:NSBackingStoreBuffered defer:NO screen:nil];
	_window.hidesOnDeactivate = NO;
	
	NSView *rootView = _window.contentView;
	_view.frame = rootView.bounds;
	[_window.contentView addSubview:self.view];
	
	_parent = window;
	[_parent addChildWindow:_window ordered:NSWindowAbove];
#else
	_window = fskWindow;
	
	[(UIWindow *)fskWindow->uiWindow addSubview:self.view];
#endif
}

- (void)detachFromWindow
{
#if IS_MAC
	[self.view removeFromSuperview];
	
	[_parent removeChildWindow:_window];
	[_window close];
	[_window release];
	_window = nil;
	_parent = nil;
#else
	[self.view removeFromSuperview];
#endif
}

- (NSString *)getURL
{
#if IS_MAC
	return [_view mainFrameURL];
#else
	NSURLRequest *request = _view.request;
	NSURL *url = request.URL;
	return [url absoluteString];
#endif
}

- (void)setURL:(NSString *)url
{
#if IS_MAC
	[_view setMainFrameURL:url];
#else
	NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:url]];
	[_view loadRequest:request];
#endif
}

- (NSString *)evaluateScript:(NSString *)script
{
#if IS_MAC
	return [_view stringByEvaluatingJavaScriptFromString:script];
	
#else
	return [_view stringByEvaluatingJavaScriptFromString:script];
#endif
}

- (void)reload
{
#if IS_MAC
	[_view reload:self];
#else
	[_view reload];
#endif
}

- (void)goBack
{
#if IS_MAC
    [_view goBack:self];
#else
    [_view goBack];
#endif
}

- (void)goForward
{
#if IS_MAC
    [_view goForward:self];
#else
    [_view goForward];
#endif
}

- (BOOL)canBack
{
	return [_view canGoBack];
}

- (BOOL)canForward
{
	return [_view canGoForward];
}

#if IS_MAC

#pragma mark - WebUIDelegate

- (BOOL)webViewIsResizable:(WebView *)sender
{
	return NO;
}

- (WebView *)webView:(WebView *)sender createWebViewWithRequest:(NSURLRequest *)request
{
	[[sender mainFrame] loadRequest:request];
	return sender;
}

- (void)webView:(WebView *)sender setFrame:(NSRect)frame
{
	// ignore
}

#pragma mark - WebFrameLoadDelegate

- (void)webView:(WebView *)sender didCommitLoadForFrame:(WebFrame *)frame
{
	NSLog(@"webView:%@ didCommitLoadForFrame:%@ %@", sender, frame, [_view mainFrameURL]);
	FskBrowserInvokeDidStartLoadCallback(self.browser);
}

- (void)webView:(WebView *)sender didCancelClientRedirectForFrame:(WebFrame *)frame
{
	NSLog(@"webView:%@ didCancelClientRedirectForFrame:%@ %@", sender, frame, [_view mainFrameURL]);
}

- (void)webView:(WebView *)sender didChangeLocationWithinPageForFrame:(WebFrame *)frame
{
	NSLog(@"webView:%@ didChangeLocationWithinPageForFrame:%@ %@", sender, frame, [_view mainFrameURL]);
}

- (void)webView:(WebView *)sender didClearWindowObject:(WebScriptObject *)windowObject forFrame:(WebFrame *)frame
{
	NSLog(@"webView:%@ didClearWindowObject:%@ forFrame:%@ %@", sender, windowObject, frame, [_view mainFrameURL]);
}

- (void)webView:(WebView *)sender didFailLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
	NSLog(@"webView:%@ didFailLoadWithError:%@ forFrame:%@ %@", sender, error, frame, [_view mainFrameURL]);
}

- (void)webView:(WebView *)sender didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
	NSLog(@"webView:%@ didFailProvisionalLoadWithError:%@ forFrame:%@ %@", sender, error, frame, [_view mainFrameURL]);
}

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
	NSLog(@"webView:%@ didFinishLoadForFrame:%@ %@", sender, frame, [_view mainFrameURL]);
	FskBrowserInvokeDidLoadCallback(self.browser);
}

- (void)webView:(WebView *)sender didReceiveIcon:(NSImage *)image forFrame:(WebFrame *)frame
{
	NSLog(@"webView:%@ didReceiveIcon:%@ forFrame:%@ %@", sender, image, frame, [_view mainFrameURL]);
}

- (void)webView:(WebView *)sender didReceiveServerRedirectForProvisionalLoadForFrame:(WebFrame *)frame
{
	NSLog(@"webView:%@ didReceiveServerRedirectForProvisionalLoadForFrame:%@ %@", sender, frame, [_view mainFrameURL]);
}

- (void)webView:(WebView *)sender didReceiveTitle:(NSString *)title forFrame:(WebFrame *)frame
{
	NSLog(@"webView:%@ didReceiveTitle:\"%@\" forFrame:%@ %@", sender, title, frame, [_view mainFrameURL]);
}

- (void)webView:(WebView *)sender didStartProvisionalLoadForFrame:(WebFrame *)frame
{
	NSLog(@"webView:%@ didStartProvisionalLoadForFrame:%@ %@", sender, frame, [_view mainFrameURL]);
}

- (void)webView:(WebView *)sender willCloseFrame:(WebFrame *)frame
{
	NSLog(@"webView:%@ willCloseFrame:%@ %@", sender, frame, [_view mainFrameURL]);
}

- (void)webView:(WebView *)sender willPerformClientRedirectToURL:(NSURL *)URL delay:(NSTimeInterval)seconds fireDate:(NSDate *)date forFrame:(WebFrame *)frame
{
	NSLog(@"webView:%@ willPerformClientRedirectToURL:%@ delay:%d fireDate:%@ forFrame:%@ %@", sender, URL, (int)seconds, date, frame, [_view mainFrameURL]);
}

- (void)webView:(WebView *)webView decidePolicyForNavigationAction:(NSDictionary *)actionInformation
		request:(NSURLRequest *)request
		  frame:(WebFrame *)frame
decisionListener:(id<WebPolicyDecisionListener>)listener
{
	NSURL *url = [request URL];
	Boolean handle = FskBrowserInvokeShouldHandleURLCallback(self.browser, (char *)[[url absoluteString] UTF8String]);

	if (!handle) {
		[listener ignore];
	} else {
		[listener use];
	}
}

#else

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
	NSURL *url = [request URL];
	Boolean handle = FskBrowserInvokeShouldHandleURLCallback(self.browser, (char *)[[url absoluteString] UTF8String]);
	return handle;
}

- (void)webViewDidStartLoad:(UIWebView *)webView
{
	FskBrowserInvokeDidStartLoadCallback(self.browser);
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
	FskBrowserInvokeDidLoadCallback(self.browser);
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
	NSLog(@"webView:didFailLoadWithError:%@", error);
}

#endif

@end

#if IS_MAC

@implementation WebViewWindow

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

@end

#endif
