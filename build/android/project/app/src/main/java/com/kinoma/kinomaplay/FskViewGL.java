/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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

	private static String TAG = "kinoma";
	private static final boolean DEBUG = false;

	public FskViewGL(Context context, AttributeSet attrs) {
		super(context, attrs);
//		Log.d(TAG, "FskViewGL constructor completed super FskView constructor");
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		super.surfaceCreated(holder);
		Surface jSurface = holder.getSurface();
		int i = setFskSurface(jSurface);
		Log.d(TAG, "FskViewGL.surfaceCreated - returned " + i + " from setFskSurface");
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
//		Log.d(TAG, "surfaceDestroyed - call native unsetFskSurface");
		unsetFskSurface();
		super.surfaceDestroyed(holder);
	}

}
