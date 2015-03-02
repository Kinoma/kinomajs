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
package com.kinoma.kinomaplay;

import android.content.Context;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Window;

class FskView extends SurfaceView implements IFskView, SurfaceHolder.Callback {
    public native int setFskSurface(Surface jSurface);
    public native int unsetFskSurface();
    public native int doFskSurfaceChanged(int width, int height);
    public native int doSizeAboutToChange(int oldwidth, int oldheight, int newwidth, int newheight);
 
    int mCanvasWidth = 0;
    int mCanvasHeight = 0;
    SurfaceHolder mHolder;
    int mInSizeChanged = 0;
    Boolean mInitialized = false;
	KinomaPlay owner;
	Boolean mNeedsActivation = false;
	
	public void setOwner(KinomaPlay owner) { 
		this.owner = owner;
	}
   
    public FskView(Context context, AttributeSet attrs) {
        super(context, attrs);

        SurfaceHolder holder = getHolder();
        holder.addCallback(this);

        setFocusable(true);     // make sure we get key events
    }

    @Override
    public void setInitialized(Boolean inited) {
    	 mInitialized = inited;
    }

     
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		if (null == mHolder || mInitialized == false)
    		return;
		
        if (mCanvasWidth != width || mCanvasHeight != height) {
    		mCanvasWidth = width;
    		mCanvasHeight = height;

    		// notify fsk that size has changed
    	}
// 		Log.i("Kinoma", "surfaceChanged - about to tell Fsk 'doFskSurfaceChanged'");

        if (0 == mInSizeChanged) {
        	doFskSurfaceChanged(width, height);
        }

    	if (mNeedsActivation) {
    		mNeedsActivation = false;
			owner.callFsk(owner.kJNIWindowActivated, "");
    	}
    }

    
    @Override protected void onSizeChanged(int w, int h, int oldw, int oldh) {

    	if (mHolder == null) {
        	return;					// initialization time
        }
    	mInSizeChanged++;
 
   		doSizeAboutToChange(oldw, oldh, w, h);			// let Kinoma have a crack at it before the size changes
//    	Log.i("Kinoma", "onSizeChanged - after telling Fsk 'about' w:" + w + " h:" + h + " oldw:" + oldw + " oldh:" + oldh);
    	super.onSizeChanged(w, h, oldw, oldh);
//    	Log.i("Kinoma", "onSizeChanged - after 'super'");

    
       	Rect keyrect = new Rect();
       	Rect oldRect = new Rect();
    	Window win = owner.getWindow();
    	win.getDecorView().getWindowVisibleDisplayFrame(oldRect);
    	
    	owner.mVerticalOffset = oldRect.top;

    	int kx, ky, kw, kh, dh;
//		Log.i("Kinoma", "Orientation: " + owner.mOrientation + " w:" + owner.mDisplay.getWidth() + " h:" + owner.mDisplay.getHeight());
		kw = owner.mDisplay.getWidth();
		dh = owner.mDisplay.getHeight();
		kh = dh - oldRect.bottom;
		kx = 0;
		ky = oldRect.bottom - owner.mVerticalOffset;
		if (owner.mInputMethodShown) {
			if (kh == 0) {
// Log.i("Kinoma", "onSizeChanged - input method shown, but kh is 0, shut down kbd.");
				owner.doIMEEnable(0);
			}
			else if (h >= oldh) {
				h = oldh;
			}
		}
	
		owner.mKeyboardRect.set(kx, ky, kx + kw, ky + kh);
//	   	Log.i("Kinoma", "onSizeChanged - keyboard rect [" + kx + "," + ky + "," + kw + "," + kh + "]");

    	doFskSurfaceChanged(w, h);
		owner.wantsKeyboard();
    	mInSizeChanged--;
    }
   
    public void surfaceCreated(SurfaceHolder holder) {
     	mHolder = holder;
     	Surface jSurface = holder.getSurface();
        setFskSurface(jSurface);
    }
    
    public void surfaceDestroyed(SurfaceHolder holder) {
    	unsetFskSurface();
        mHolder = null;
    }


}
