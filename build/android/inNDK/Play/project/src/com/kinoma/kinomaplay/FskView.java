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

	private static String TAG = "kinoma";

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
Log.i("FSKVIEW", "Constructor");
        setZOrderOnTop(true);

        SurfaceHolder holder = getHolder();
        holder.addCallback(this);

        setFocusable(true);     // make sure we get key events
    }

    @Override
    public void setInitialized(Boolean inited) {
Log.i("FSKVIEW", "setInitialized");
    	 mInitialized = inited;
    }


	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
Log.i("FSKVIEW", "surfaceChanged");
		if (null == mHolder || mInitialized == false)
			return;

//		Log.d(TAG, "FskView.surfaceChanged(holder:" + holder + " format:" + format + " w:" + width + " h:" + height + ")");
		if (mCanvasWidth != width || mCanvasHeight != height) {
			mCanvasWidth = width;
			mCanvasHeight = height;

			// notify fsk that size has changed
		}
//		Log.d(TAG, "\tFskView.surfaceChanged - about to tell Fsk 'doFskSurfaceChanged'");

		if (0 == mInSizeChanged) {
			doFskSurfaceChanged(width, height);
		}

		if (mNeedsActivation) {
			mNeedsActivation = false;
			owner.callFsk(owner.kJNIWindowActivated, "");
		}
//		Log.d(TAG, "FskView.surfaceChanged - returning");
	}


	@Override protected void onSizeChanged(int w, int h, int oldw, int oldh) {
Log.i("FSKVIEW", "onSizeChanged");
		super.onSizeChanged(w, h, oldw, oldh);
	}


    public void surfaceCreated(SurfaceHolder holder) {
Log.i("FSKVIEW", "surfaceCreated");
     	mHolder = holder;
     	Surface jSurface = holder.getSurface();
        setFskSurface(jSurface);
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
Log.i("FSKVIEW", "surfaceDestroyed");
    	unsetFskSurface();
        mHolder = null;
    }


}
