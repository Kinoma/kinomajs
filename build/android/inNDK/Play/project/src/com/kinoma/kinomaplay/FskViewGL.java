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
import com.kinoma.kinomaplay.FskView;
import android.view.Window;

// GL stuff
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;


class FskViewGL extends FskView implements IFskView, SurfaceHolder.Callback {
    public native int setFskSurface(Surface jSurface);
    public native int unsetFskSurface();
    public native int doFskSurfaceChanged(int width, int height);
    public native int doSizeAboutToChange(int oldwidth, int oldheight, int newwidth, int newheight);

	private static String TAG = "Kinoma GL";
	private static final boolean DEBUG = false;

    public FskViewGL(Context context, AttributeSet attrs) {
        super(context, attrs);
 		// Log.i("Kinoma GL", "FskViewGL constructor completed super FskView constructor");
   }

	@Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    	super.surfaceChanged(holder, format, width, height);
		// Log.i("Kinoma GL", "surfaceChanged isCreating:" + holder.isCreating() + " w:" + width + " h:" + height);
		// Log.i("Kinoma GL", "surfaceChanged - about to tell Fsk 'doFskSurfaceChanged'");
		doFskSurfaceChanged(width, height);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		//super.onSizeChanged(w, h, oldw, oldh); -- We don't want to do this because it causes something to be called twice.
		// Log.i("Kinoma GL", "onSizeChanged - about to tell Fsk 'doFskSurfaceChanged'");
		doFskSurfaceChanged(w, h);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
    	super.surfaceCreated(holder);
     	Surface jSurface = holder.getSurface();
        int i = setFskSurface(jSurface);
        // Log.i(TAG, "FskViewGL.surfaceCreated - returned " + i + " from setFskSurface");
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
		// Log.i("Kinoma GL", "surfaceDestroyed - call native unsetFskSurface");
		unsetFskSurface();
    	super.surfaceDestroyed(holder);
    }


//    @Override
//	public void onDrawFrame(GL10 gl) {
//    	doDrawFrame();
//    	gl.eglSwapBuffers(mEglDisplay, mEglSurface);
//	}
}
