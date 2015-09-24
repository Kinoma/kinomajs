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

import java.io.IOException;
import java.io.FileNotFoundException;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.AutoFocusCallback;
import android.util.Log;
import android.view.SurfaceView;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.view.SurfaceHolder;
import android.graphics.ImageFormat;
import java.io.File;
import java.io.FileOutputStream;
import java.util.Date;
import java.text.SimpleDateFormat;
import android.media.MediaActionSound;
import android.os.Environment;
import org.json.JSONObject;
import org.json.JSONArray;
import java.util.List;
import org.json.JSONException;
import java.lang.reflect.Array;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
import java.lang.StringBuffer;
import java.util.Iterator;
import android.graphics.Rect;
import java.util.ArrayList;


public class FskCamera implements SurfaceHolder.Callback{
	private final String TAG = "FskCamera";
	Camera mCamera = null;
	int mCameraId = 0;
	MediaActionSound mMediaActionSound;
	SurfaceTexture mSurfaceTexture = null;
	SurfaceView mSurfaceView;
	SurfaceHolder mHolder;
	boolean mUseTexture = true;
	Context mContext;

	int mCameraCount = 0;
	int mPreviewWidth = 0;
	int mPreviewHeight = 0;
	int mPreviewFormat = 0;

	public native void nativeInit();
	public native void setNativeCallback(Camera cam);
	public native void unsetNativeCallback(Camera cam);
	public native void nativeDataCallback(byte[] data, Camera cam, int type);
	private int mNativeContext;

	public FskCamera(Context context, SurfaceView view) {
		mContext = context;

		if (view != null) {
			Log.i(TAG, "Using SurfaceView");
			mSurfaceView = view;
			mUseTexture = false;

			mHolder = mSurfaceView.getHolder();
			mHolder.addCallback(this);
			mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

		}

		nativeInit();
		Log.i(TAG, "FskCamera Created");
	}

	public void init() {
		//Android document says no need to manually play such sound, the framework will play the appropriate sounds when call takePicture().
		//Not working for us.
		mMediaActionSound = new MediaActionSound();
		mMediaActionSound.load(MediaActionSound.SHUTTER_CLICK);

		if (android.os.Build.VERSION.SDK_INT >= 9)
			mCameraCount = Camera.getNumberOfCameras();
		else
			mCameraCount = 1;

		mCameraId = 0;

		setup(mCameraId);
	}

	public void setup(int camId) {
		if (android.os.Build.VERSION.SDK_INT >= 9)
			mCamera = Camera.open(camId);
		else
			mCamera = Camera.open();

		Camera.Parameters para = mCamera.getParameters();

		//GL is not available below API 9, low fps for better performance
		if (android.os.Build.VERSION.SDK_INT < 9) {
			para.setPreviewFrameRate(15);
			para.setPreviewSize (320, 240);
		}
		//Default preview size too small, set it at least 640x480
		else if ((para.getPreviewSize().width * para.getPreviewSize().height) < (640 * 480)){
			List<Camera.Size> previewSizeList = para.getSupportedPreviewSizes();
			for (int i=0; i<previewSizeList.size(); i++) {
				Camera.Size size = previewSizeList.get(i);

				int pixels = size.width * size.height;
				if (pixels >= 640*480) {
					para.setPreviewSize(size.width, size.height);
					Log.i(TAG, "Setting Preview size to " + size.width + "x" + size.height);
					break;
				}
			}
		}

		mPreviewWidth = para.getPreviewSize().width;
		mPreviewHeight = para.getPreviewSize().height;
		mPreviewFormat = para.getPreviewFormat();

		List<Camera.Size> pictureSizeList = para.getSupportedPictureSizes();
		for (int i=0; i<pictureSizeList.size(); i++) {
			Camera.Size size = pictureSizeList.get(i);

			//Set the default size around 2MP
			int pixels = size.width * size.height;
			if (pixels > 1800000 && pixels < 2600000) {
				para.setPictureSize(size.width, size.height);
				Log.i(TAG, "Setting Picture size to " + size.width + "x" + size.height);
				break;
			}
		}

		//A special case of a null focus area list means the driver is free to select focus targets as it wants
		//Some device (e.g. vivo Xplay) need a set before getting a proper value
		if (0 != para.getMaxNumFocusAreas()) {
			para.setFocusAreas(null);
		}

		mCamera.setParameters(para);
		Log.i(TAG, mCamera.getParameters().flatten());

		//Log.d(TAG, "Setting native callbacks");
		//setNativeCallback(mCamera);

		//We now use java callback, allocate a buffer and add it in every callback
		Log.d(TAG, "Setting Java callbacks");
		//int bitsPerPixel = ImageFormat.getBitsPerPixel(mPreviewFormat);
		//int size = mPreviewWidth * mPreviewWidth * bitsPerPixel / 8;
		//byte[] buffer = new byte[size];
		//mCamera.addCallbackBuffer(buffer);
		//mCamera.setPreviewCallbackWithBuffer(previewCallback);
		//mCamera.setPreviewCallback(previewCallback);

		if (mUseTexture) {
			Log.d(TAG, "Creating a SurfaceTexture");
			mSurfaceTexture = new SurfaceTexture(123);
			try {
				mCamera.setPreviewTexture(mSurfaceTexture);
			}
			catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		else {
			Log.d(TAG, "Using SurfaceView from xml");
			//mSurfaceView = new SurfaceView(mContext);
			//mSurfaceView.setWidth(1);
			//mSurfaceView.setHeight(1);

			try {
				mCamera.setPreviewDisplay(mHolder);
			}
			catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}

	}

	public void finish() {
		mCamera.release();
		mMediaActionSound.release();
		mCamera = null;
		mSurfaceTexture = null;
		mSurfaceView = null;
	}

	public void startPreview() {
		mCamera.startPreview();
		mCamera.setPreviewCallback(previewCallback);
	}

	public void stopPreview() {
		mCamera.setPreviewCallback(null);
		mCamera.stopPreview();
	}

	public void takePicture() {
		Log.d(TAG, "takePicture");
		mCamera.takePicture(null, null, jpegCallback);
		mMediaActionSound.play(MediaActionSound.SHUTTER_CLICK);
	}

	public void autoFocus() {
		Log.d(TAG, "autoFocus");
		mCamera.autoFocus(autoFocusCallback);
	}

	public void switchCamera() {
		Log.d(TAG, "switchCamera");

		if (mCameraCount <= 1) {
			Log.d(TAG, "Only 1 camera, can not switch, bail!");
			return;
		}

		if (mCameraId == 0) {
			mCameraId = 1;
		}
		else {
			mCameraId = 0;
		}

		stopPreview();
		mCamera.release();

		setup(mCameraId);
		startPreview();
	}

	public JSONObject genSize(String size) {
		String[] sizeArray = size.split("x", 2);
		JSONObject object = new JSONObject();

		try {
			object.put("width", sizeArray[0]);
			object.put("height", sizeArray[1]);
		}
		catch (JSONException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		return object;
	}

	public JSONObject genRange(String[] range) {
		JSONObject object = new JSONObject();

		try {
			object.put("min", range[0]);
			object.put("max", range[1]);
		}
		catch (JSONException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		return object;
	}

	public JSONObject genCameraArea(String[] area) {
		JSONObject object = new JSONObject();

		try {
			object.put("left", area[0]);
			object.put("top", area[1]);
			object.put("right", area[2]);
			object.put("bottom", area[3]);
			object.put("weight", area[4]);
		}
		catch (JSONException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		return object;
	}

	public JSONArray genArray(String value) {
		JSONArray array = new JSONArray();

		String[] valueArray = value.split(",");

		for (int i=0; i<valueArray.length; i++) {
			if (valueArray[i].matches("(\\d+)x(\\d+)")) {
				array.put(genSize(valueArray[i]));
			}
			else {
				array.put(valueArray[i]);
			}
		}
		return array;
	}

	public JSONArray genObjectArray(String value) {
		JSONArray array = new JSONArray();
		String[] objectArray = value.split("\\(|\\),\\(|\\)");

		for (int i=0; i<objectArray.length; i++) {
			Log.i(TAG, objectArray[i]);

			String[] valueArray = objectArray[i].split(",");

			if (valueArray.length == 2) {
				array.put(genRange(valueArray));
			}
			else if (valueArray.length == 5) {
				array.put(genCameraArea(valueArray));
			}
		}

		return array;
	}

	public String getParametersJSON() {
		JSONObject jsonObject = new JSONObject();

		try {
			JSONArray cameraArray = new JSONArray();

			//If we have open a camera already, we can not open another at the same time
			int cameraObjectCount;
			if (mCamera != null) {
				cameraObjectCount = 1;
			}
			else {
				cameraObjectCount = mCameraCount;
			}

			for (int n=0; n<cameraObjectCount; n++) {
				Camera cam;
				if (mCamera != null) {
					cam = mCamera;
				}
				else {
					if (android.os.Build.VERSION.SDK_INT >= 9)
						cam = Camera.open(n);
					else
						cam = Camera.open();
				}

				JSONObject cameraObject = new JSONObject();

				//CameraInfo
				if (android.os.Build.VERSION.SDK_INT >= 9) {
					Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
					cam.getCameraInfo(n, cameraInfo);

					cameraObject.put("facing", cameraInfo.facing);
					cameraObject.put("orientation", cameraInfo.orientation);

				}

				//Paramters
				String paraString = cam.getParameters().flatten();
				String[]paraArray = paraString.split(";");

				for (int i=0; i<paraArray.length; i++) {

					String[] pair = paraArray[i].split("=");

					if (pair.length != 2) { 
						Log.i(TAG, "Not a key=value pair, continue!");
						continue;
					}

					String key = pair[0];
					String value = pair[1];

					//Key : "preview-size"=>"previewSize"
					Pattern p = Pattern.compile("(?:\\-)(\\S)(?:(?:^\\-)*)");
					Matcher m = p.matcher(key);
					StringBuffer sb = new StringBuffer();
					while (m.find())
						m.appendReplacement(sb, m.group(1).toUpperCase());
					m.appendTail(sb);

					key = sb.toString();

					//Object Array
					if (value.matches("(\\(\\S+\\),)+\\(\\S+\\)")) {
						cameraObject.put(key, genObjectArray(value));
					}
					//Array
					else if (value.matches("([^\\(\\)]+,)+([^\\(\\)]+)")) {
						cameraObject.put(key, genArray(value));
					}
					//Camera Area Object or Range Object
					else if (value.matches("\\((\\d+,)+(\\d+)\\)")) {
						String s = value.substring(1, value.length()-1);
						String[] valueArray = s.split(",");

						if (valueArray.length == 2) {
							cameraObject.put(key, genRange(valueArray));
						}
						else if (valueArray.length == 5) {
							cameraObject.put(key, genCameraArea(valueArray));
						}
					}
					//Size Object
					else if (value.matches("(\\d+)x(\\d+)")) {
						cameraObject.put(key, genSize(value));
					}
					//Value
					else {
						cameraObject.put(key,value);
					}
				}

				if (mCamera == null) {
					cam.release();
				}

				cameraArray.put(cameraObject);
			}

			jsonObject.put("cameras", cameraArray);
			jsonObject.put("cameraCount", mCameraCount);
		}
		catch (JSONException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		String JSONString = jsonObject.toString();

		Log.i(TAG, "Camera parameter JSON string:" + JSONString);

		return JSONString;
	}

	public List<Camera.Area> getFocusAreas(JSONArray array) {
		ArrayList<Camera.Area> focusAreas = new ArrayList<Camera.Area>();

		try {
			for (int i=0; i<array.length(); i++) {
				JSONObject obj = array.getJSONObject(i);
				Rect focusArea = new Rect(obj.getInt("left"),
				                          obj.getInt("top"),
				                          obj.getInt("right"),
				                          obj.getInt("bottom"));
				focusAreas.add(new Camera.Area(focusArea, obj.getInt("weight")));
			}
		}
		catch (JSONException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		return focusAreas;
	}

	public void setParametersJSON(String json) {
		JSONObject paraObject;
		StringBuffer StringBuffer = new StringBuffer();
		Camera.Parameters para = mCamera.getParameters();

		try {
			paraObject= new JSONObject(json);

			Iterator it = paraObject.keys();
			while(it.hasNext()) {
				String key = (String)it.next();

				if (key.equals("focusAreas")) {
					Log.i(TAG, "Setting focus areas");
					JSONArray array = paraObject.getJSONArray(key);
					List<Camera.Area> focusAreas = getFocusAreas(array);
					para.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
					para.setFocusAreas(focusAreas);
				}
				else if (key.equals("pictureSize")){
					JSONObject obj = paraObject.getJSONObject(key);
					para.setPictureSize(obj.getInt("width"), obj.getInt("height"));
				}
				else if (key.equals("previewSize")) {
					JSONObject obj = paraObject.getJSONObject(key);
					mPreviewWidth = obj.getInt("width");
					mPreviewHeight = obj.getInt("height");

					stopPreview();
					//mCamera.setPreviewCallback(previewCallback);
					//mCamera.setPreviewCallbackWithBuffer(null);

					//int bitsPerPixel = ImageFormat.getBitsPerPixel(mPreviewFormat);
					//int size = mPreviewWidth * mPreviewWidth * bitsPerPixel / 8;
					//byte[] buffer = new byte[size];
					//mCamera.addCallbackBuffer(buffer);
					//mCamera.setPreviewCallbackWithBuffer(previewCallback);

					para.setPreviewSize(mPreviewWidth, mPreviewHeight);
					mCamera.setParameters(para);
					startPreview();
					return;
				}
				else {
					String value = paraObject.getString(key);
					StringBuffer.append(key).append("=").append(value).append(";");
					para.unflatten(StringBuffer.toString());
				}
			}
		}
		catch (JSONException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return;
		}

		mCamera.setParameters(para);

		Log.i(TAG, mCamera.getParameters().flatten());
	}

	public void chooseCamera(int index) {
		Log.d(TAG, "chooseCamera");

		if (mCameraCount <= 1) {
			Log.d(TAG, "Only 1 camera, can not switch, bail!");
			return;
		}

		if ( (index < 0) || (index >= mCameraCount) ) {
			Log.d(TAG, "camera indec out of range, bail!");
			return;
		}

        mCameraId = index;
		stopPreview();
		mCamera.release();

		setup(mCameraId);
		startPreview();
	}

	//We only support one area for now
	public String getFocusArea() {
		Camera.Parameters para = mCamera.getParameters();

		if (0 == para.getMaxNumFocusAreas()) {
			Log.d(TAG, "Focus Area is not supported");
			return null;
		}

		List<Camera.Area> areas = para.getFocusAreas();

		//A special case of a null focus area list means the driver is free to select focus targets as it wants.
		if (areas == null) {
			Log.d(TAG, "Focus Areas is empty!");
			return null;
		}

		//Just get the 1st one
		Camera.Area area = areas.get(0);

		Log.d(TAG, area.rect.toShortString());

		return area.rect.toShortString();
	}

	public void surfaceCreated(SurfaceHolder holder) {
		mHolder = holder;
		Log.d(TAG, "Surface Created");
	}

	public void surfaceDestroyed(SurfaceHolder holder) {
		Log.d(TAG, "Surface Destroyed");
	}

    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
	    Log.d(TAG, "Surface Changed");
	    mHolder = holder;
    }

    PreviewCallback previewCallback = new PreviewCallback() {
		public void onPreviewFrame(byte[] data, Camera camera) {
			//Log.d(TAG, "onPreviewFrame");

			nativeDataCallback(data, camera, 0);
			//camera.addCallbackBuffer(data);
		}
	};

	PictureCallback jpegCallback = new PictureCallback() {
		public void onPictureTaken (byte[] data, Camera camera) {

            //Log.d(TAG, "into onPictureTaken()");
            nativeDataCallback(data, camera, 1);
/*
            File pictureFileDir = getDir();
            if (!pictureFileDir.exists() && !pictureFileDir.mkdirs()) {
              Log.d(TAG, "Can't create directory to save image.");
              return;
            }
            SimpleDateFormat dateFormat = new SimpleDateFormat("yyyymmddhhmmss");
            String date = dateFormat.format(new Date());
            String photoFile = "Picture_" + date + ".jpg";
            String filename = pictureFileDir.getPath() + File.separator + photoFile;
            File pictureFile = new File(filename);
            Log.d(TAG, "Writting File" + filename);
            try {
              FileOutputStream fos = new FileOutputStream(pictureFile);
              fos.write(data);
              fos.close();
            } catch (Exception error) {
              Log.d(TAG, "File" + filename + "not saved: " + error.getMessage());
            }
*/

		   Log.d(TAG, "onPictureTaken");

		   startPreview();
		}

      private File getDir() {
        File sdDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
        return new File(sdDir, "KinomaCamera");
      }
	};


	AutoFocusCallback autoFocusCallback = new AutoFocusCallback() {
		public void  onAutoFocus(boolean success, Camera camera){
		   Log.d(TAG, "AutoFocus succeed:" + success);
		}
	};
}
