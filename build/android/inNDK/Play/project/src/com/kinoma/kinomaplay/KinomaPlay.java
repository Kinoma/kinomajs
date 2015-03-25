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

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.ByteArrayOutputStream;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.URLEncoder;
import java.text.Collator;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.locks.ReentrantLock;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.res.XmlResourceParser;
//#ifdefined C2D_MESSAGE
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
//#endif
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Rect;
//#ifdefined ACCESS_FINE_LOCATION
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.LocationProvider;
//#endif
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.http.SslError;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
//#ifdefined C2D_MESSAGE
import android.os.AsyncTask;
//#endif
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.provider.Contacts;
import android.provider.Contacts.People;
import android.provider.ContactsContract;
import android.provider.MediaStore;
import android.provider.MediaStore.Images;
import android.provider.Settings.Secure;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.SignalStrength;
import android.telephony.TelephonyManager;
import android.telephony.gsm.SmsManager;
import android.telephony.gsm.SmsMessage;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.SparseArray;
import android.view.Display;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.SurfaceView;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.webkit.ConsoleMessage;
import android.webkit.ConsoleMessage.MessageLevel;
import android.view.Choreographer;
import android.webkit.CookieSyncManager;
import android.webkit.HttpAuthHandler;
import android.webkit.JavascriptInterface;
import android.webkit.SslErrorHandler;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.AbsoluteLayout;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

//#ifdefined C2D_MESSAGE
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.gcm.GoogleCloudMessaging;
import com.google.android.gms.common.GooglePlayServicesUtil;
//#endif

public class KinomaPlay extends Activity
{
	final int DO_TEST_EXPIRATION = 0;
	final int ExpiryYear = 2014;
		final int ExpiryMonth = (7 - 1);	// starts at 0
		final int ExpiryDay = 31;
	final int DO_VERSION_CHECK = 1;
		final int LatestVersionMaj = 5;
		final int LatestVersionMin = 0;

	final int kJNIStartFsk = 1;
	final int kJNIIdleFsk = 2;
	final int kJNIStopFsk = 3;
	final int kJNISetupForCallbacks = 4;
	final int kJNIFskInvalWindow = 5;
	final int kJNILowMemory = 6;
	final int kJNINetworkChanged = 7;
    final int kJNIFskReceivedSMS = 8;
    final int kJNIWindowDeactivated = 9;
    final int kJNIWindowActivated = 10;
    final int kJNIVolumeMounted = 11;
    final int kJNIVolumeEjected = 12;
    final int kJNIPackagesChanged = 13;
    final int kJNIExtendAudioBuffer = 14;
    final int kJNIReduceAudioBuffer = 15;
    public native int callFsk(int param, String msg);

    final int kSetVolume = 1;
    final int kSetOpenGL = 2;
    final int kGetOpenGL = 3;
    final int kSetKeyboard = 4;
    public native int callFskInt(int param, int val);
    Boolean mBuildWantsGL = false;
//#ifdefined C2D_MESSAGE
	Boolean mRemoteNotificationRegistered = false;
    int getRemoteNotificationType() {
		SharedPreferences prefs = getPreferences();
		int type = prefs.getInt(kRemoteNotificationTypeKey, 0);
		Log.i("kinoma", "RemoteNotificationType=" + type);
		return type;
	}
    void setRemoteNotificationType(int type) {
		SharedPreferences prefs = getPreferences();
		Editor editor = prefs.edit();
		editor.putInt(KinomaPlay.kRemoteNotificationTypeKey, type);
		editor.commit();
	}
    String mBuildRemoteNotificationID = null;
	public final static int kNotificationIDRemote = 1;
    static final String kConfigName = "KinomaPlayConfig";
    static final String kRemoteNotificationMessageKey = "RemoteNotificationMessage";
    static final String kRemoteNotificationTypeKey = "RemoteNotificationType";
//#endif

    final int kFskKeyboardTypeAlphanumeric = 0x100;
    final int kFskKeyboardTypePhone12Keys = 0x200;
    final int kFskKeyboardTypeAlphaAndPhone12Keys = 0x400;
    final int kFskKeyboardTypeVirtual = 0x800;

    final int kAndroidSoftKeyboardTypeHTC = 0x1;
    final int kAndroidSoftKeyboardTypeGoogle = 0x2;
    int mSoftKeyboardType = 0;
    Boolean mThereWasSomeMouseAction = false;

	int mSDKVersion = 0;

	public native static void setStaticDeviceInfo(String model, String osVersion,
			int buttonsReversed, int needsOrientationRotation, int touchCapable,
			String imei, String uuid, int hDpi, int vDpi, int densityDpi,
			int screenWidth, int screenHeight, int statusBarHeight,
			String dataDir, String appPath, String externalPath);
	public native static void setSpecialPaths(String musicDir, String podcastsDir, String picturesDir, String moviesDir, String downloadsDir, String dcimDir);
	public native static void setDeviceUsername(String deviceName);

//#ifdefined ACCESS_FINE_LOCATION
	public native static void fskSetGPSInfo(double lat, double lng, double alt, double heading, double speed,
			int status, int visible, double UTC, double acc);
	public native static void fskSetGPSStatus(int status);
//#endif

	public native static boolean doFskKeyEvent(int keyCode, int modifiers, int action, int param, int repeat);
	public native static boolean doFskMotionEvent(int action, int x, int y, int deltaMS);
	public native static boolean doFskMotionMultiple(int ptAndTicks[]);
	public native static boolean doFskMotionMultipleTouch(int ptAndTicks[]);
	public native static boolean doFskMotionTouch(int action, int pointer, int x, int y, int ms);

	public native static void doFskOnTextChanged(String str, int start, int before, int count);

//#ifdefined C2D_MESSAGE
	public native static void doFskOnRemoteNotificationRegistered(String str);
	public native static void doFskOnRemoteNotification(String str);
	public native static void checkLaunched();
	public static boolean getLaunched() {
		try {
			checkLaunched();
		} catch (UnsatisfiedLinkError error) {
			// Application may not be launched
			return false;
		}
		return true;
	}
//#endif

	public native static void fskPhoneStateChanged(int what, int state);
	public native static void fskPhoneSSIDChanged(String ssid);
	public native static void fskPhoneOperatorChanged(String operator);

	public native static void fskSetVolumeMax(int max);

//#ifdefined C2D_MESSAGE
	public native static String fskGetEnvironment(String key);
//#endif

	public native static void setIMEEnabled(int enabled);

	public native static void setFskKeyboardType(int kbdType);
	public native static void setDeviceOrientation(int orientation);
	public native static void setAndroidLanguage(String language);
	public native static void setAndroidBasetime(int s, int ms);

	public native static int doPause();
	public native static int doResume();

	private final int kFskCallState = 1;
	private final int kFskCellDataConnectionState = 2;
	private final int kFskMessageWaitingState = 3;
	private final int kFskServiceState = 4;
	private final int kFskSignalStrength = 5;
	private final int kFskBatteryLevel = 6;
	private final int kFskBatteryPlugged = 7;
//  private final int kFskBatteryStatus = 8;
	private final int kFskBacklightOn = 9;
	private final int kFskCellDataConnectionType = 10;
	private final int kFskDataConnectionState = 11;
	private final int kFskNetworkEnabled = 12;
	private final int kFskNetworkType = 13;
	private final int kFskDataSignalStrength = 14;
	private final int kFskNetworkWifiAddress = 9999;

	private final int kNetIDWifi = 1;
	private final int kNetIDPhone = 0;

	private String android_id;

	int gMajorVersion;
	int gMinorVersion;

	Display mDisplay;
	private IFskView mFskView;

	private EditText mFskEditText[];
	private int mFskEditTextCur = 0;
	private boolean mFskEditTextIgnoreChanges = false;

	private WebView mWebView;
	LinearLayout mWebMainView;	// contains the webview and a button

//#ifdefined CAMERA
    private FskCamera mCamera;
//#endif

	private View mMain;
	private View mWebLayout;

	String[]         mLauncherAppicationList = null;
	String           mLauncherStartApplication = null;
	String           mLauncherStartService = null;

	int mVerticalOffset = 0;	// for global mouse positioning
	Boolean mButtonsReversed = false;

	String mOperator = "";
	String mOldWifiSSID = "";

	private Bitmap mSplashBitmap;

	private PackageManager mPackageManager;

	boolean mIsEmulator = false;

    private AudioManager mAudioManager;
    private ComponentName mRemoteControlResponder;
    private static Method mRegisterMediaButtonEventReceiver;
    private static Method mUnregisterMediaButtonEventReceiver;

    private IntentFilter mNoisyAudioIntent;
    private BroadcastReceiver mNoisyAudioStreamResponder;

    private boolean mHasGetRotation = false;

//#ifdefined ACCESS_FINE_LOCATION
	private LocationManager locationManager;
	private LocationListener listenerCoarse;
	private LocationListener listenerFine;
	private boolean locationAvailable = true;
	private int coarseGpsStatus = 0;
	private int fineGpsStatus = 0;
	private boolean mCheckGPSOnResume = false;
	private boolean mGpsUpdaterInstalled = false;
	private boolean mGpsPendingDispose = false;

	private Location gLastCoarseLocation;
	private Location gLastFineLocation;
//#endif

	private PhoneStateListener mPhoneStateListener;
	private WifiManager mWifiManager;
	private WifiManager.WifiLock mWifiLock;
	private String mIMEI = null;

//#ifdefined ACCESS_NETWORK_STATE
	private TelephonyManager mTelephonyManager;
//#endif

	private PowerManager mPowerManager;
	PowerManager.WakeLock mWakeLock;
	boolean mHasLock = false;
	boolean mScreenOn = true;

	private InputMethodManager mInputMethodMgr;
	boolean mInputMethodShown = false;
	private boolean mWasInputMethodShowing = false;
	boolean mNeedsKeyboardHeight;
	Rect mKeyboardRect = new Rect();
	boolean gPendingIMEClose = false;

	WindowManager mWindowManager;
	private boolean mIsFullscreen = false;
	int mOrientation = 0;

	private FskWebViewClient mWebViewClient;
	private int mViewingWeb = 0;

	private int mApplistLoaded = 0;

	private boolean gInitialized = false;
//#ifdefined C2D_MESSAGE
	static boolean active = false;
//#endif
	private boolean mPaused = false;
	private ReentrantLock mPreloadApps = new ReentrantLock();

	private final int kTimerMessage = 1;
	private final int kIdleMessage = 2;
//#ifdefined ACCESS_FINE_LOCATION
	private final int kGPSOffMessage = 3;
//#endif
	private final int kAirplaneModeOff = 4;
	private final int kResumeMessage = 5;
	private final int kDismissKeyboard = 6;
	private final int kFullscreenMessage = 7;


	private int mAirplaneModeOffCheck = 0;
	private boolean mAirplaneModeOn = false;

	int lastCellSignalStrength = 0;
	String gLastLanguage = "";

	private final int endInitPhase = 9;
	private int mInitPhase = -5;
	private boolean mIdlePending = false;

	private ContentResolver mResolver;

	private Play2Android mWakeMe;
	private Intent gSvc;

	private String kKinomaTrampolineName = "com.kinoma.trampoline";

	long mStartTime;

    int mOldCallState = -13;
    int mOldCellConnectionState = -13;
    int mOldDataConnectionNetworkType = -13;
    int mOldDataConnectionState = -13;
    int mOldWifiIPAddress = -13;
    int mOldNetworkType = -13;
	int mOldServiceState = -13;
	int mOldWifiSignalLevel = -13;

//#ifdefined C2D_MESSAGE
	private final static int PLAY_SERVICES_RESOLUTION_REQUEST = 9000;
//#endif

	/*******************************************************/
	public String getUsername(){
		BluetoothAdapter adapter = null;
		try {
			adapter = BluetoothAdapter.getDefaultAdapter();
		}
		catch (Exception e) {

		}

		if (null == adapter)
			return Build.DEVICE;

		String name = null;
		try {
			name = adapter.getName();
		}
		catch (Exception e) {

		}
		if (null == name)
			return Build.DEVICE;

	    return name;
	}

    private BroadcastReceiver btReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(BluetoothAdapter.ACTION_LOCAL_NAME_CHANGED)) {
            	Log.i("Kinoma", "BT DeviceName changed:" + getUsername());
            } else if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED) &&
                    (intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR) ==
                            BluetoothAdapter.STATE_ON)) {
            	Log.i("Kinoma", "BT State changed");
            }
        }
    };

	private void readKconfig(Activity activity)
	    throws XmlPullParserException, IOException
	{
		StringBuffer stringBuffer = new StringBuffer();
		Resources res = activity.getResources();
		XmlResourceParser xpp = res.getXml(R.xml.kconfig);
		xpp.next();
		int eventType = xpp.getEventType();
		Boolean gotVariable = false;
		Boolean platformDoit = false;
		String variableName = "";
		String variableValue = "";

		while (eventType != XmlPullParser.END_DOCUMENT) {
			if(eventType == XmlPullParser.START_DOCUMENT) {
	//			Log.i("kinoma", "--- Start XML ---");
			}
			else if(eventType == XmlPullParser.START_TAG) {
				String tagName = xpp.getName();
				gotVariable = false;

				platformDoit = true;	// if there's no "platform" attribute, then assume this is okay
//				Log.i("kinoma", "START_TAG: " + tagName);
//				Log.i("kinoma", "attribute count: " + xpp.getAttributeCount());
				if (tagName.contentEquals("variable")) {
					gotVariable = true;
					for (int i=0; i< xpp.getAttributeCount(); i++) {
						String value = xpp.getAttributeValue(i);
						String name = xpp.getAttributeName(i);

						if (name.contentEquals("platform")) {			// if there's a platform tag, make sure it's android.
							if (-1 == value.indexOf("android")) {
								platformDoit = false;
							}
						}
						if (name.contentEquals("name")) {
							variableName = value;
						}
						if (name.contentEquals("value")) {
							variableValue = value;
						}
//						Log.i("kinoma", "attribute: " + xpp.getAttributeName(i) + ", value: " + xpp.getAttributeValue(i));
					}
				}
			}
			else if(eventType == XmlPullParser.END_TAG) {
//				Log.i("kinoma", "END_TAG: "+xpp.getName());
				if (gotVariable && platformDoit) {
					if (variableName.contentEquals("useGL")) {
						if (variableValue.contentEquals("1"))
							mBuildWantsGL = true;
					}
				}
				gotVariable = false;
				variableName = "";
				variableValue = "";
			}
			eventType = xpp.next();
		}
	}

	private static final long MILLISECONDS_PER_SECOND = 1000;
	private static final long NANOSECONDS_PER_MILLISECOND = 1000000;
	private static final long INTERVAL_TOLERANCE_MS = 2;
	private static final long MINIMUM_INTERVAL_MS = 12;
	private boolean mContinuousDrawing = false;
	public native static void setWindowUpdateInterval(long interval);
	public native static void fskWindowUpdate(long updateTime);
	private FrameCallback mFrameCallback = null;
	public Choreographer mChoreographer = null;
	private long mVsyncIntervalMS = 0;
	private long mWindowUpdateIntervalMS = 0;
	public class FrameCallback implements Choreographer.FrameCallback {
		private static final String tag = "FrameCallback";
		private long lastTime = 0;
		private long lastUpdateTime = 0;

		public void doFrame (long frameTimeNanos) {
			long frameTimeMs = frameTimeNanos/NANOSECONDS_PER_MILLISECOND;
			long delta = frameTimeMs - lastTime;
			lastTime = frameTimeMs;

			if (mContinuousDrawing) {
				long updateDelta = frameTimeMs - lastUpdateTime;

				if ((updateDelta - mWindowUpdateIntervalMS) > -INTERVAL_TOLERANCE_MS) {
					//Log.d(tag, "doFrame");
					fskWindowUpdate(lastTime + mVsyncIntervalMS);

					lastUpdateTime = frameTimeMs;
				}
			}

			mChoreographer.postFrameCallback(this);

			if (delta - mWindowUpdateIntervalMS > INTERVAL_TOLERANCE_MS)
				Log.d(tag, "delta[" + delta + "] > mWindowUpdateIntervalMS[" + mWindowUpdateIntervalMS + "]");
		}
	}

	public void setContinuousDrawing(boolean continuousDrawing)
	{
		mContinuousDrawing = continuousDrawing;
	}

	/*******************************************************/
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
	    String stringXmlContent;
	    try {
	    	readKconfig(this);
	    } catch (XmlPullParserException e) {
	    	e.printStackTrace();
	    } catch (IOException e) {
	    	e.printStackTrace();
	    }

		setContentView(R.layout.splashscreen);

		String[] v = Build.VERSION.RELEASE.split("\\D");

		gMajorVersion = Integer.parseInt(v[0]);
		gMinorVersion = Integer.parseInt(v[1]);
		try {
			if ((gMajorVersion > 2) ||
				(gMajorVersion == 2 && gMinorVersion >= 2))
				mHasGetRotation = true;
		}
		catch(NumberFormatException nfe) {
			mHasGetRotation = false;
		}

		if (1 == DO_VERSION_CHECK) {
			try {
				if ( (gMajorVersion > LatestVersionMaj) ||
					((gMajorVersion == LatestVersionMaj) && (gMinorVersion > LatestVersionMin)) )
					doBadVersion(gMajorVersion, gMinorVersion);
			}
			catch(NumberFormatException nfe) {
				doBadVersion(0, 0);
			}
		}

		if (1 == DO_TEST_EXPIRATION) {
			Boolean expired = true;
			Calendar c = Calendar.getInstance();

			while (1 == 1) {
				if (c.get(Calendar.YEAR) > ExpiryYear)
					break;
				if (c.get(Calendar.YEAR) < ExpiryYear) {
					expired = false;
					break;
				}
				if (c.get(Calendar.MONTH) > ExpiryMonth)
					break;
				if (c.get(Calendar.MONTH) < ExpiryMonth) {
					expired = false;
					break;
				}
				if (c.get(Calendar.DATE) <= ExpiryDay) {
					expired = false;
					break;
				}
				break;
			}
			if (expired)
				doBadDate();
		}

		System.loadLibrary("Fsk");
		try {
			System.loadLibrary("KinomaLibG");
		}
		catch (UnsatisfiedLinkError e)
		{
			try {
				System.loadLibrary("KinomaLibF");
			}
			catch (UnsatisfiedLinkError e2)
			{
				Log.i("Kinoma", "can't load OS version glueLib");
			}

		}

		initializeRemoteControlRegistrationMethods();

//#ifdefined READ_PHONE_STATE
		mTelephonyManager = (TelephonyManager)getSystemService(TELEPHONY_SERVICE);
		mIMEI = mTelephonyManager.getDeviceId();
//#endif

		if (null == mIMEI)
			mIMEI = "000000000000000";
		if (0 == mIMEI.compareTo("000000000000000") && 0 == Build.BRAND.compareTo("generic"))
			mIsEmulator = true;

		ViewGroup		mainViewGroup = (ViewGroup) LayoutInflater.from(this).inflate(R.layout.main, null);
		if ( !mIsEmulator && mBuildWantsGL && ((gMajorVersion > 2) || ((gMajorVersion == 2) && (gMinorVersion >= 3)) ) )
		{
			FskViewGL			fskView = new FskViewGL( this, null );
			Log.i("kinoma", "using OpenGL");
			mainViewGroup.addView( fskView, LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT );
			fskView.setOwner( this );
			mFskView = fskView;
			callFskInt(kSetOpenGL, 1);

		}
		else
		{
			FskView				fskView = new FskView( this, null );
			mainViewGroup.addView( fskView, LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT );
			fskView.setOwner( this );
			mFskView = fskView;
			callFskInt(kSetOpenGL, 0);
		}

		mMain = mainViewGroup;

		try{
			System.loadLibrary("StagefrightOMXCodec_I");
		}
		catch (UnsatisfiedLinkError e_i)
		{
			try {
				System.loadLibrary("StagefrightOMXCodec_H");
			}
			catch (UnsatisfiedLinkError e_h)
			{
				try {
					System.loadLibrary("StagefrightOMXCodec_G");
				}
				catch (UnsatisfiedLinkError e_g)
				{
					try {
						System.loadLibrary("StagefrightOMXCodec_F");
					}
					catch (UnsatisfiedLinkError e_f)
					{
						Log.i("Kinoma", "can't load OS version StagefrightOMXCodec for I, H, G or F");
					}
				}
			}
		}

		mSDKVersion = android.os.Build.VERSION.SDK_INT;

//#ifdefined CAMERA
        try{
            System.loadLibrary("FskCameraAndroid_java");
            Log.i("Kinoma", "load OS version FskCameraAndroid_java for J");

            if (mSDKVersion < 14) {
                Log.i("Kinoma", "API version < 14 using SurfaceView");
                SurfaceView view=(SurfaceView)mMain.findViewById(R.id.camera_preview);
                Log.i("Kinoma", "Getting SurfaceView:" + view);
                mCamera = new FskCamera(this, view);
            }
            else
                mCamera = new FskCamera(this, null);
        }
        catch (UnsatisfiedLinkError ec_j)
        {
            Log.i("Kinoma", "can't load OS version FskCameraAndroid for J" + ec_j);
        }
//#endif
/*
        try {
            System.loadLibrary("FskAndroidJavaEncoder");
            Log.i("Kinoma", "load FskAndroidJavaEncoder");
        } catch (UnsatisfiedLinkError aje) {
            Log.i("Kinoma", "can't load FskAndroidJavaEncoder" + aje);
        }
*/

        try {
            System.loadLibrary("FskAndroidJavaDecoder");
            Log.i("Kinoma", "load FskAndroidJavaDecoder");
        } catch (UnsatisfiedLinkError ajd) {
            Log.i("Kinoma", "can't load FskAndroidJavaDecoder" + ajd);
        }

		switch (mSDKVersion) {
			case 17:
			case 16://android.os.Build.VERSION_CODES.JELLY_BEAN, defined in sdk 4.0+
			try{
				System.loadLibrary("IOMXCodec_I");
				Log.i("Kinoma", "load OS version IOMXCodec_I for JellyBean");
			}
			catch (UnsatisfiedLinkError e_i1)
			{
				Log.i("Kinoma", "can't load OS version IOMXCodec_I for JellyBean");
			}
				break;
			case android.os.Build.VERSION_CODES.ICE_CREAM_SANDWICH:
			case 15://android.os.Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1:
				try{
					System.loadLibrary("IOMXCodec_I");
					Log.i("Kinoma", "load OS version IOMXCodec for I");
				}
				catch (UnsatisfiedLinkError e_i1)
				{
					Log.i("Kinoma", "can't load OS version IOMXCodec for I");
				}
				break;
			case android.os.Build.VERSION_CODES.HONEYCOMB:
			case android.os.Build.VERSION_CODES.HONEYCOMB_MR1:
			case android.os.Build.VERSION_CODES.HONEYCOMB_MR2:
				try {
					System.loadLibrary("IOMXCodec_H");
					Log.i("Kinoma", "load OS version IOMXCodec for H");
				}
				catch (UnsatisfiedLinkError e_h)
				{
					Log.i("Kinoma", "can't load OS version IOMXCodec for H");
				}
				break;
			case android.os.Build.VERSION_CODES.GINGERBREAD:
			case android.os.Build.VERSION_CODES.GINGERBREAD_MR1:
				try {
					System.loadLibrary("IOMXCodec_G");
					Log.i("Kinoma", "load OS version IOMXCodec for G");
				}
				catch (UnsatisfiedLinkError e_h)
				{
					Log.i("Kinoma", "can't load OS version IOMXCodec for G");
				}
				break;
			case android.os.Build.VERSION_CODES.FROYO:
				try {
					System.loadLibrary("IOMXCodec_F");
					Log.i("Kinoma", "load OS version IOMXCodec for F");
				}
				catch (UnsatisfiedLinkError e_h)
				{
					Log.i("Kinoma", "can't load OS version IOMXCodec for F");
				}
				break;
		}

		//Choreographer was imported after API 16
		if (mSDKVersion >= 16) {
			try {
				mChoreographer = Choreographer.getInstance();
				mFrameCallback = new FrameCallback();
				//Post the FrameCallback in onResume
				//mChoreographer.postFrameCallback(mFrameCallback);
			}
			catch (IllegalStateException e) {
				Log.i("Kinoma", "can't get the choreographer instance");
			}
		}

		mFskEditText = new EditText[2];
		mFskEditText[0] = (EditText)mMain.findViewById(R.id.text_view);
		mFskEditText[1] = (EditText)mMain.findViewById(R.id.text_view2);
		for (mFskEditTextCur = 0; mFskEditTextCur < 2; mFskEditTextCur++) {
			mFskEditText[mFskEditTextCur].setVisibility(View.INVISIBLE);
			mFskEditText[mFskEditTextCur].setOnEditorActionListener(new OnEditorActionListener() {
				public boolean onEditorAction(TextView arg0, int arg1, KeyEvent event) {
//				Log.i("Kinoma", "FskEditText onEditorAction " + arg0.toString() + " "  + arg1);
					if (arg1 == EditorInfo.IME_ACTION_DONE) {
						doIMEEnable(0);
						return true;
					}
					return false;
				}
			});

			mFskEditText[mFskEditTextCur].addTextChangedListener(new TextWatcher() {
				CharSequence b;
				public void onTextChanged(CharSequence s, int start, int before, int count) {
					if (!mFskEditTextIgnoreChanges) {
//						Log.i("Kinoma", "onTextChanged - new string: " + s.toString() + " start: " + start + " before: " + before + " count: " + count);
						doFskOnTextChanged(s.toString(), start, before, count);
					}
				}
				public void afterTextChanged(Editable arg0) { }
				public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
			});
		}
		mFskEditTextCur = 0;

		mResolver = getContentResolver();

		mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
		mRemoteControlResponder = new ComponentName(getPackageName(), RemoteControlReceiver.class.getName());
		mNoisyAudioStreamResponder = new NoisyAudioStreamReceiver();
		mNoisyAudioIntent = new IntentFilter(AudioManager.ACTION_AUDIO_BECOMING_NOISY);


        this.setVolumeControlStream(AudioManager.STREAM_MUSIC);
 //       Log.i("Kinoma", " set volume max to be " + mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC));
        fskSetVolumeMax(mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC));
		mLastVol = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
		callFskInt(kSetVolume, mLastVol);

//        IntentFilter filter;
//        filter = new IntentFilter(Intent.ACTION_PACKAGE_ADDED);
//        filter.addAction(Intent.ACTION_PACKAGE_REMOVED);
//        filter.addAction(Intent.ACTION_PACKAGE_CHANGED);
//        filter.addDataScheme("package");
//        registerReceiver(mPackageReceiver, filter);

        Intent intent = getIntent();
        if ("KinomaPlay.installAndGo".equals(intent.getAction())) {
        	Bundle extras = intent.getExtras();
        	if (extras != null)
        	{
        	   mLauncherAppicationList = extras.getStringArray( "apps" );
        	   mLauncherStartApplication = extras.getString( "launch.app" );
        	   mLauncherStartService = extras.getString( "launch.service" );
        	}
        }
        else if ("KinomaPlay.launch".equals(intent.getAction())) {
        	Bundle extras = intent.getExtras();
        	if (extras != null) {
        		mLauncherStartApplication = extras.getString( "launch.app" );
        		mLauncherStartService = extras.getString( "launch.service" );
        	}
        }

        CookieSyncManager.createInstance(getBaseContext());
	}

	private void doBadDate() {
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setMessage("This test version of Kinoma Play has expired.")
			.setCancelable(false)
			.setPositiveButton("Quit",
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int id){
						finish();
					} } ) ;
		AlertDialog alert = builder.create();
		alert.show();
	}

	/*******************************************************/
	private void doBadVersion(int maj, int min){
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
			builder.setMessage("Our apologies. We have not yet verified compatibility "
						+ "with this device [" + Build.MODEL + "] or version ["
						 + maj + "." + min + "].")
				.setCancelable(false)
				.setPositiveButton("Quit",
					new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int id){
							finish();
						} } ) ;
			builder.setNegativeButton("Try it",
					new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int id){
							;
						} } ) ;
			AlertDialog alert = builder.create();
			alert.show();
		}

/*******************************************************/
  @Override
   public void onStart()
   {
	  	mStartTime = android.os.SystemClock.uptimeMillis();

	  	super.onStart();

        if (mIsEmulator)
        	return;

        IntentFilter ejectFilter = new IntentFilter();
		ejectFilter.addAction(Intent.ACTION_MEDIA_MOUNTED);
		ejectFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
		ejectFilter.addAction(Intent.ACTION_MEDIA_REMOVED);
		ejectFilter.addAction(Intent.ACTION_MEDIA_EJECT);
		ejectFilter.addDataScheme("file");
		registerReceiver(ejectReceiver, ejectFilter);
    }


	/*******************************************************/
  public boolean trampolineInstalled() {
	  List<ApplicationInfo> packages;

	  packages = mPackageManager.getInstalledApplications(0);
      for (ApplicationInfo packageInfo : packages) {
    	  if (packageInfo.packageName.equals(kKinomaTrampolineName))
    		  return true;
      }
      return false;
  }


	/*******************************************************/
	@SuppressLint("NewApi")
	void doInit(int phase) {

//		Log.i("Kinoma", "doInit phase " + phase);
		switch (phase) {
			case 0:
				gSvc = new Intent(this, KinomaService.class);	//  Try this to try to stop being killed in bkgd.
				startService(gSvc);

				mPackageManager = getPackageManager();
				mWindowManager = getWindowManager();
				mDisplay = mWindowManager.getDefaultDisplay();

				if (mFrameCallback != null) {
					float refreshRate = mDisplay.getRefreshRate();

					//Assuming it is a reasonable value, but some device may not, set it to a normal value, 60Hz
					if (refreshRate <= 5) refreshRate = 60;

					mVsyncIntervalMS = (long) (MILLISECONDS_PER_SECOND / refreshRate);

					//We don't want to update too frequently, make the interval double
					if (mVsyncIntervalMS < MINIMUM_INTERVAL_MS)
						mWindowUpdateIntervalMS = mVsyncIntervalMS * 2;
					else
						mWindowUpdateIntervalMS = mVsyncIntervalMS;


					Log.i("Kinoma", "Setting mWindowUpdateIntervalMS=" + mWindowUpdateIntervalMS);
					setWindowUpdateInterval(mWindowUpdateIntervalMS);
				}

				DisplayMetrics dm = new DisplayMetrics();
				mDisplay.getMetrics(dm);

				Configuration config = getResources().getConfiguration();

				String prefsDir = getBaseContext().getFileStreamPath("").getAbsolutePath() + "/";
				String appPath = null;
				try {
					PackageInfo selfPkg = mPackageManager.getPackageInfo(getBaseContext().getPackageName(), 0);
					appPath = selfPkg.applicationInfo.sourceDir;
				} catch (NameNotFoundException e) {
					// TODO Auto-generated catch block
					  e.printStackTrace();
				}

				Rect rect = new Rect();
		    	Window win = this.getWindow();
		    	win.getDecorView().getWindowVisibleDisplayFrame(rect);
		    	mVerticalOffset = rect.top;

		    	String userName = getUsername();
		    	Log.i("Kinoma", "user:" + userName);

		    	if (false == mIsEmulator) {
		    		IntentFilter btfilter = new IntentFilter();
		    		btfilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
		    		btfilter.addAction(BluetoothAdapter.ACTION_LOCAL_NAME_CHANGED);
		    		getBaseContext().registerReceiver(btReceiver, btfilter);
		    	}

		        setDeviceUsername(userName);
		    	mButtonsReversed = (areButtonsMirrored(Build.MODEL) != 0);

				Locale myLocale = Locale.getDefault();
				Log.i("Kinoma","Locale language set to " + myLocale.getLanguage().toUpperCase());
				gLastLanguage = myLocale.getLanguage().toUpperCase();
				setAndroidLanguage(gLastLanguage);

//#ifdefined READ_PHONE_STATE
				android_id = Secure.getString(mResolver, Secure.ANDROID_ID);
//#endif
				if (null == android_id)
					android_id = "unknown";

				int s, ms;
				s = (int)(mStartTime/1000);
				ms = (int)(mStartTime % 1000);
				setAndroidBasetime(s, ms);
				Log.i("Kinoma", "BaseTime: " + mStartTime + " == " + s + " s and " + ms + " ms");

				if (mSDKVersion > 7) {
					String musicDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
					String picturesDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
					String podcastsDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
					String moviesDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
					String downloadsDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
					String dcimDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();

					setSpecialPaths(musicDir, podcastsDir, picturesDir, moviesDir, downloadsDir, dcimDir);
				}

				setStaticDeviceInfo(Build.MODEL, "android." + Build.VERSION.RELEASE,
						0, /* mButtonsReversed ? 1 : 0, */
						(android.os.Build.VERSION.SDK_INT < 11) ? 1 : 0,	/* needsOrientationRotate */
						config.touchscreen, mIMEI, android_id, (int)dm.xdpi, (int)dm.ydpi, dm.densityDpi,
						mDisplay.getWidth(), mDisplay.getHeight(), mVerticalOffset, prefsDir, appPath, Environment.getExternalStorageDirectory().getAbsolutePath());

				Log.i("Kinoma", "Product:" + Build.PRODUCT + " Manufacturer:" + Build.MANUFACTURER + " Model:" + Build.MODEL);
				Log.i("Kinoma", " Device:" + Build.DEVICE + " Brand:" + Build.BRAND + (mButtonsReversed ? " [REVERSED BUTTONS]" : ""));
				Log.i("Kinoma", " Release:" + Build.VERSION.RELEASE + " Major version:" + gMajorVersion);
				if (	Build.MANUFACTURER.contains("HTC")
					||  Build.DEVICE.contains("Nexus S")    ) {
					mSoftKeyboardType = kAndroidSoftKeyboardTypeHTC;
				}
				else {
					mSoftKeyboardType = kAndroidSoftKeyboardTypeGoogle;
				}


				mOrientation = mDisplay.getOrientation();
//				Log.i("Kinoma", "startup - orientation is " + mOrientation);
				setDeviceOrientation(mOrientation);

				// mWakeMe = new Play2Android(mainRunloop);
				mWakeMe = new Play2Android(this.getApplicationContext(), mainRunloop);
				break;
			case 1:
				callFsk(kJNIStartFsk, "");
//				Log.i("Kinoma", "after StartFsk");
				break;
			case 2:
				mFskView.setInitialized(true);
				gInitialized = true;

//#ifdefined C2D_MESSAGE
				String value = fskGetEnvironment("remoteNotification");
				if ((value != null) && !value.contentEquals("0")) {
					setRemoteNotificationType(Integer.parseInt(value));
				} else {
					setRemoteNotificationType(0);
				}
				value = fskGetEnvironment("remoteNotificationID");
				if (value != null) {
					Log.i("Kinoma", "doInit - remoteNotificationID is valid");
					mBuildRemoteNotificationID = value;
				}

				Log.i("Kinoma", "doInit - about to registerRemoteNotification");
				registerRemoteNotification();
//#endif
				break;
			case 3:
				callFsk(kJNISetupForCallbacks, "");	// sets up callback for doLaunch (web)
				break;
			case 4:
				mInputMethodMgr = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
//#ifdefined READ_PHONE_STATE
				mPhoneStateListener = new MyPhoneStateListener();
				mTelephonyManager.listen(mPhoneStateListener,
						PhoneStateListener.LISTEN_CALL_STATE
						| PhoneStateListener.LISTEN_DATA_CONNECTION_STATE
						| PhoneStateListener.LISTEN_MESSAGE_WAITING_INDICATOR
						| PhoneStateListener.LISTEN_SERVICE_STATE
						| PhoneStateListener.LISTEN_SIGNAL_STRENGTHS);

				registerReceiver(airplaneModeReceiver, new IntentFilter(Intent.ACTION_AIRPLANE_MODE_CHANGED) );
//#endif

				break;
			case 5:
				registerReceiver(batteryReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
				mPowerManager = (PowerManager)getSystemService(Context.POWER_SERVICE);

				IntentFilter screenModeFilter = new IntentFilter();
				screenModeFilter.addAction(Intent.ACTION_SCREEN_OFF);
				screenModeFilter.addAction(Intent.ACTION_SCREEN_ON);
				registerReceiver(checkScreenModeReceiver, screenModeFilter);

//#ifdefined RECEIVE_SMS
				registerReceiver(smsReceiver, new IntentFilter("android.provider.Telephony.SMS_RECEIVED") );
//#endif
				break;
			case 6:
				break;
			case 7:
//#ifdefined ACCESS_WIFI_STATE
		        registerReceiver(new ConnectionChangeReceiver(),
	        		new IntentFilter("android.net.conn.CONNECTIVITY_CHANGE"));

				IntentFilter wifiStatusFilter = new IntentFilter();
				wifiStatusFilter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
				wifiStatusFilter.addAction(WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION);
				wifiStatusFilter.addAction(WifiManager.RSSI_CHANGED_ACTION);
		        registerReceiver(mWifiReceiver, wifiStatusFilter);
//#endif

		        mWifiManager = (WifiManager)getSystemService(Context.WIFI_SERVICE);
		        mWifiLock = mWifiManager.createWifiLock(/*mWifiManager.WIFI_MODE_FULL_HIGH_PERF, */ "Kinoma WifiLock"); // what level OS is FULL_HIGH_PERF?

		        break;

			case 8:
Log.i("Kinoma", "8 - 1");
				setContentView(mMain);
Log.i("Kinoma", "8 - 2");
				callFsk(kJNIFskInvalWindow, "");
				if (null != mLauncherStartApplication) {
					if (null != mLauncherAppicationList)
						doInstallAndGo(mLauncherAppicationList, mLauncherStartApplication, mLauncherStartService);
					else
						doLaunch(mLauncherStartApplication, mLauncherStartService);
				}
Log.i("Kinoma", "8 - 3");
				break;

			case 9:
//				Log.i("Kinoma", "finished initialization");
				break;
		}

		mainRunloop.sendEmptyMessage(kIdleMessage);
	}

	final String buttonsMirroredList[] = {
		    "ADR6350",              // HTC Incredible 2
		    "ADR6300",              // HTC Incredible
		    "T-Mobile G2",
		    "DROIDX",               // Motorola Droid X
		    "MB860",
		    "SCH-I800",             // Samsung Tab
	    	"SAMSUNG-SCH-I897",     // Samsung Galaxy
		    "KU9500",               // LG
		    "SU950",                // LG
		    "HTC Desire",
		    "LG GW620",
		    "Android Dev Phone 1",
		    "Eris",
		    "HTC Magic",
		    "SGH-T959",
		    "SPH-M900",
		    "DROID PRO",
		    "HTC Tattoo",
		    "PC36100",              // EVO (Charles)
		    // Not "Droid",
		    // Not "dkb",
		    // Not "dkbtd",         // ASUS
		    // Not "OMS_TTD",       // OPhone ASUS
		    // Not "brownstone",    // Marvell tablet proto
		    // Not "Nexus One",
		    // Not "Nexus S",
		    // Not "Behold II",
		    // Not "T-Mobile myTouch 3G Slide"
		    // Not "Milestone XT720"
		    // Not "T-01C"
		    // Not "SC-01C"
		    // Not "SC-01B"
		    // Not "SC-02B"
		    // Not "asari"
		    // Not "LT-NA7"
		    // Not "X06HT"
		    // Not "ViewPad7"
		    // Not "T7"
		    // Not "GT-P1000"
		    // Not "GT-P1010"
		    // Not "saarb_emmc"
		    // Not "MotoA953
		    // Not "Touchscreen 16-9"
		    // Not "avlite"
		    ""
	};

	private int areButtonsMirrored(String deviceName) {
		int i = 0;
		while (buttonsMirroredList[i] != "") {
			if (deviceName.contentEquals(buttonsMirroredList[i]))
				return 1;
			i++;
		}
		return 0;
	}

    private boolean isCdma() {
//#ifdefined ACCESS_NETWORK_STATE
        return ((mTelephonyManager != null) && (mTelephonyManager.getPhoneType() == TelephonyManager.PHONE_TYPE_CDMA));
//#endif
    }

//#ifdefined READ_PHONE_STATE
	public class MyPhoneStateListener extends PhoneStateListener {
		public void onCallStateChanged(int state, String incomingNumber) {
//			Log.i("Kinoma", "PhoneState - call state:" + state + " incomingNumber:" + incomingNumber);
			if (state != mOldCallState) {
				fskPhoneStateChanged(kFskCallState, state);
				mOldCallState = state;
			}
		}

		public void onDataConnectionStateChanged(int state) {
//			Log.i("Kinoma", "PhoneState - onDataConnectionStateChanged - state: " + state);
			if (state != mOldCellConnectionState) {
				fskPhoneStateChanged(kFskCellDataConnectionState, state);
				mOldCellConnectionState = state;

				if (TelephonyManager.DATA_CONNECTED == state) {
					int networkType = mTelephonyManager.getNetworkType();

					if (networkType != mOldNetworkType) {
						fskPhoneStateChanged(kFskCellDataConnectionType, networkType);
						mOldNetworkType = networkType;
					}
				}
			}
			else if (TelephonyManager.DATA_DISCONNECTED == state) {
//				fskPhoneOperatorChanged("");	// this will "No service" the status bar, even if status is available,
												//just inactive because Wifi is on
			}
		}

		public void onMessageWaitingIndicatorChanged(boolean mwi) {
			fskPhoneStateChanged(kFskMessageWaitingState, mwi ? 1 : 0);
		}

		public void onServiceStateChanged(ServiceState serviceState) {
			int state = serviceState.getState();
//			Log.i("Kinoma", "PhoneState - onServiceStateChanged - state: " + state);
			if (state != mOldServiceState) {
				mOldServiceState = state;
				fskPhoneStateChanged(kFskServiceState, state);
			}
			String oper = serviceState.getOperatorAlphaLong();
			if (oper != null)
			 	fskPhoneOperatorChanged(oper);
		 	else
				fskPhoneOperatorChanged("");
		}

		public void onSignalStrengthsChanged(SignalStrength signalStrength) {
			int signal;

			if (mAirplaneModeOn) {
				fskPhoneStateChanged(kFskSignalStrength, 0);
				return;
			}

			if (signalStrength.isGsm()) {
				signal = signalStrength.getGsmSignalStrength();
//				Log.i("Kinoma", "gsmSignalStrength: " + signal);
				if (signal < 32)
					signal = (100 * signal) / 32;
				else
					signal = 0;

			}
			else {
		        if (!isCdma()) {
		            int asu = signalStrength.getGsmSignalStrength();

		            // ASU ranges from 0 to 31 - TS 27.007 Sec 8.5
		            // asu = 0 (-113dB or less) is very weak
		            // signal, its better to show 0 bars to the user in such cases.
		            // asu = 99 is a special case, where the signal strength is unknown.
		            if (asu <= 0 || asu == 99) signal = 0;
		            else if (asu >= 16) signal = 100;
		            else if (asu >= 8)  signal = 75;
		            else if (asu >= 4)  signal = 50;
		            else signal = 25;
		        } else {
		            int cdmaDbm = signalStrength.getCdmaDbm();
		            int cdmaEcio = signalStrength.getCdmaEcio();
		            int levelDbm = 0;
		            int levelEcio = 0;

		            if (cdmaDbm >= -75) levelDbm = 100;
		            else if (cdmaDbm >= -85) levelDbm = 75;
		            else if (cdmaDbm >= -95) levelDbm = 50;
		            else if (cdmaDbm >= -100) levelDbm = 25;
		            else levelDbm = 0;

		            // Ec/Io are in dB*10
		            if (cdmaEcio >= -90) levelEcio = 100;
		            else if (cdmaEcio >= -110) levelEcio = 75;
		            else if (cdmaEcio >= -130) levelEcio = 50;
		            else if (cdmaEcio >= -150) levelEcio = 25;
		            else levelEcio = 0;

		            signal = (levelDbm < levelEcio) ? levelDbm : levelEcio;
		        }
			}

			if (lastCellSignalStrength != signal) {
				fskPhoneStateChanged(kFskSignalStrength, signal);
				lastCellSignalStrength = signal;
			}
		}
	}
//#endif

	/*******************************************************/
//#ifdefined ACCESS_WIFI_STATE
    public class ConnectionChangeReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
	        ConnectivityManager connectivityManager = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
	        NetworkInfo activeNetInfo = connectivityManager.getActiveNetworkInfo();
	        if ((null == activeNetInfo) || (activeNetInfo.isAvailable() == false)) {
//	        	Log.i("Kinoma", "ConnectionChangeReceiver - no network active or not available");
				fskPhoneStateChanged(kFskNetworkEnabled, 0);
				fskPhoneStateChanged(kFskNetworkWifiAddress, 0);
	        }
	        else {
	    		fskPhoneStateChanged(kFskNetworkType, activeNetInfo.getType());
	    		if (activeNetInfo.getType() == ConnectivityManager.TYPE_MOBILE) {
	        		NetworkInfo mobNetInfo = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
	        		if (mobNetInfo.isConnected() == true) {
//			        	Log.i("Kinoma", "ConnectionChangeReceiver - TYPE_MOBILE - isConnected");
	        			fskPhoneStateChanged(kFskCellDataConnectionState, 2);
	        		}
	        		else if (mobNetInfo.isConnectedOrConnecting() == true) {
//			        	Log.i("Kinoma", "ConnectionChangeReceiver - TYPE_MOBILE - isConnectedOrConnecting");
	        			fskPhoneStateChanged(kFskCellDataConnectionState, 1);
	        		}
	        	}
	        	else {
	        		NetworkInfo mobNetInfo = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
	        		if (mobNetInfo.isConnected() == true) {
//			        	Log.i("Kinoma", "ConnectionChangeReceiver - TYPE_WIFI - isConnected");
        				WifiManager wifiManager = (WifiManager) getSystemService(WIFI_SERVICE);
        				WifiInfo wifiInfo = wifiManager.getConnectionInfo();
        				String ssid = wifiInfo.getSSID();
        				if (false == ssid.equals(mOldWifiSSID)) {
		    				fskPhoneSSIDChanged(ssid);
        					mOldWifiSSID = ssid;
        				}
        				int ipAddress = wifiInfo.getIpAddress();
						if (ipAddress != mOldWifiIPAddress) {
		    				fskPhoneStateChanged(kFskNetworkWifiAddress, ipAddress);
							mOldWifiIPAddress = ipAddress;
						}
						if (2 != mOldDataConnectionState) {
        					fskPhoneStateChanged(kFskDataConnectionState, 2);
							mOldDataConnectionState = 2;
						}
 	        		}
	        		else if (mobNetInfo.isConnectedOrConnecting() == true) {
//			        	Log.i("Kinoma", "ConnectionChangeReceiver - TYPE_WIFI - isConnectedOrConnecting");
	        			fskPhoneStateChanged(kFskDataConnectionState, 1);
						mOldDataConnectionState = 1;
	        		}
	           	}
            }
        }
    }
//#endif

    /*******************************************************/

    private BroadcastReceiver mPackageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
        	final String action = intent.getAction();

        	Log.i("Kinoma", "new package stuff - action:" + action);
        	preloadApps(0);
            callFsk(kJNIPackagesChanged, mLaunchables);
        }
    };

    /*******************************************************/

	private BroadcastReceiver mWifiReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
        	final String action = intent.getAction();
			int state;

        	if (action.equals(WifiManager.RSSI_CHANGED_ACTION)) {
        		if (mAirplaneModeOn) {
        			fskPhoneStateChanged(kFskDataSignalStrength, 0);
        		}
        		else {
        			final int newRssi = intent.getIntExtra(WifiManager.EXTRA_NEW_RSSI, -200);
        			int newSignalLevel = WifiManager.calculateSignalLevel(newRssi, 5);
        			newSignalLevel *= 20;
					if (newSignalLevel != mOldWifiSignalLevel) {
						mOldWifiSignalLevel = newSignalLevel;
	        			fskPhoneStateChanged(kFskDataSignalStrength, newSignalLevel);
					}
        		}
            }
        	else if (action.equals(WifiManager.WIFI_STATE_CHANGED_ACTION)) {
                final boolean enabled = intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE,
                       WifiManager.WIFI_STATE_UNKNOWN) == WifiManager.WIFI_STATE_ENABLED;
//	        	Log.i("Kinoma", "mWifiReceiver - WIFI_STATE_CHANGED_ACTION - enabled" + enabled);
               	state = enabled ? ((mOldDataConnectionState == 2) ? 2 : 1) : 0;
				if (state != mOldDataConnectionState) {
					mOldDataConnectionState = state;
	               	fskPhoneStateChanged(kFskDataConnectionState, state);
				}
            }
        	else if (action.equals(WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION)) {
                final boolean enabled = intent.getBooleanExtra(WifiManager.EXTRA_SUPPLICANT_CONNECTED, false);
//	        	Log.i("Kinoma", "mWifiReceiver - SUPPLICANT_CONNECTION_CHANGE_ACTION - enabled" + enabled);
               	state = enabled ? ((mOldDataConnectionState == 2) ? 2 : 1) : 0;
				if (state != mOldDataConnectionState) {
					mOldDataConnectionState = state;
              		fskPhoneStateChanged(kFskDataConnectionState, state);
				}
        	}
        }
    };

    /*******************************************************/
    private BroadcastReceiver airplaneModeReceiver = new BroadcastReceiver() {
    	@Override
    		public void onReceive(Context context, Intent intent) {
    			boolean state = intent.getBooleanExtra("state", false);
    			if (state) {
    				mAirplaneModeOn = true;
    				fskPhoneOperatorChanged("");
    				fskPhoneStateChanged(kFskDataSignalStrength, 0);
    				fskPhoneStateChanged(kFskSignalStrength, 0);
    			}
    			else {
    				mAirplaneModeOn = false;
    				mAirplaneModeOffCheck = 0;
    				mainRunloop.sendEmptyMessageDelayed(kAirplaneModeOff, 3000);
    			}
    		}
		};

	    private BroadcastReceiver ejectReceiver = new BroadcastReceiver() {
	    	@Override
	    		public void onReceive(Context context, Intent intent) {
	    			final String action = intent.getAction();
	    			String path = intent.getDataString();
//	    			Log.i("Kinoma", "got action " + action + " in ejectReceiver");

	    			if (action.equals(Intent.ACTION_MEDIA_MOUNTED)) {
//	    				Log.i("Kinoma", "got a MOUNTED for " + path);
	    			    callFsk(kJNIVolumeMounted, path);
	    			}
	    			else if (action.equals(Intent.ACTION_MEDIA_EJECT)
	    					|| action.equals(Intent.ACTION_MEDIA_UNMOUNTED)) {
//	    				Log.i("Kinoma", "got an EJECT for " + path);
	    			    callFsk(kJNIVolumeEjected, path);
	    			}
	    		}
			};

		    /*******************************************************/

//#ifdefined ACCESS_FINE_LOCATION
private void showGpsOptions(){
	Intent gpsOptionsIntent = new Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS);
	startActivity(gpsOptionsIntent);
	mCheckGPSOnResume = true;
}

/*******************************************************/

private void createGpsDisabledAlert(){
	AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setMessage("Your GPS is disabled! Would you like to enable it?")
			.setCancelable(false)
			.setPositiveButton("Enable GPS",
				new DialogInterface.OnClickListener(){
					public void onClick(DialogInterface dialog, int id){
						showGpsOptions();
					}
				});
		builder.setNegativeButton("Do nothing",
				new DialogInterface.OnClickListener(){
					public void onClick(DialogInterface dialog, int id){
						dialog.cancel();
					}
				});
		AlertDialog alert = builder.create();
		alert.show();
	}

    /*******************************************************/

    private void installGPS() {
//		Log.i("Kinoma", "installGPS");

		mCheckGPSOnResume = false;

    	Criteria fine = new Criteria();
    	fine.setAccuracy(Criteria.ACCURACY_FINE);
    	fine.setCostAllowed(false);
    	Criteria coarse = new Criteria();
    	coarse.setAccuracy(Criteria.ACCURACY_COARSE);
    	coarse.setCostAllowed(false);

    	locationManager = (LocationManager)getSystemService(LOCATION_SERVICE);
    	if (!locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)
    		&& !locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER)) {
//    		Log.i("Kinoma", "no GPS Provider");
    		createGpsDisabledAlert();
			double lat = 37.411015, lng = -121.982044, alt = 10.0, heading = 0.0, speed = 0.0, UTC = 0.0, acc = 1024.0;
			int visible = 1;
			fineGpsStatus = -1;
			UTC = System.currentTimeMillis() / 1000.0;
			fskSetGPSInfo(lat, lng, alt, heading, speed, 0, visible, UTC, acc);
    	}

   		if (mGpsPendingDispose) {
 // 			Log.i("Kinoma", "was GpsPendingDispose - cancel the dispose");
    		mGpsPendingDispose = false;
			mainRunloop.removeMessages(kGPSOffMessage);
    	}

    	if (listenerFine == null && listenerCoarse == null)
    		createLocationListeners();

    	if (listenerFine != null || listenerCoarse != null) {
    		mGpsUpdaterInstalled = true;
    	}

    	String coarseProvider = locationManager.getBestProvider(coarse, true);
    	String fineProvider = locationManager.getBestProvider(fine, true);

    	if (listenerCoarse != null && coarseProvider != null) {
 //   		locationManager.requestLocationUpdates(coarseProvider, 1*60000, 1000, listenerCoarse);
    		locationManager.requestLocationUpdates(coarseProvider, 0, 0, listenerCoarse);
    	}
    	if (listenerFine != null && fineProvider != null) {
//    		locationManager.requestLocationUpdates(fineProvider,  1*60000, 25, listenerFine);
    		locationManager.requestLocationUpdates(fineProvider, 0, 0, listenerFine);
    	}
    }

    /** Determines whether one Location reading is better than the current Location fix
     * @param location  The new Location that you want to evaluate
     * @param currentBestLocation  The current Location fix, to which you want to compare the new one
     */
 private static final int TWO_MINUTES = 1000 * 60 * 2;
 protected boolean isBetterLocation(Location location, Location currentBestLocation) {
       if (currentBestLocation == null) {
           // A new location is always better than no location
           return true;
       }

       // Check whether the new location fix is newer or older
       long timeDelta = location.getTime() - currentBestLocation.getTime();
       boolean isSignificantlyNewer = timeDelta > TWO_MINUTES;
       boolean isSignificantlyOlder = timeDelta < -TWO_MINUTES;
       boolean isNewer = timeDelta > 0;

       // If it's been more than two minutes since the current location, use the new location
       // because the user has likely moved
       if (isSignificantlyNewer) {
           return true;
       // If the new location is more than two minutes older, it must be worse
       } else if (isSignificantlyOlder) {
           return false;
       }

       // Check whether the new location fix is more or less accurate
       int accuracyDelta = (int) (location.getAccuracy() - currentBestLocation.getAccuracy());
       boolean isLessAccurate = accuracyDelta > 0;
       boolean isMoreAccurate = accuracyDelta < 0;
       boolean isSignificantlyLessAccurate = accuracyDelta > 200;

       // Check if the old and new location are from the same provider
       boolean isFromSameProvider = isSameProvider(location.getProvider(),
               currentBestLocation.getProvider());

       // Determine location quality using a combination of timeliness and accuracy
       if (isMoreAccurate) {
           return true;
       } else if (isNewer && !isLessAccurate) {
           return true;
       } else if (isNewer && !isSignificantlyLessAccurate && isFromSameProvider) {
           return true;
       }
       return false;
   }

   /** Checks whether two providers are the same */
   private boolean isSameProvider(String provider1, String provider2) {
       if (provider1 == null) {
         return provider2 == null;
       }
       return provider1.equals(provider2);
   }
    /*******************************************************/
    private void createLocationListeners() {
       	listenerFine = new LocationListener() {
    		public void onStatusChanged(String provider, int status, Bundle extras) {
    			Log.i("Kinoma", "GPS listenerFine onStatusChanged" + provider + " - status: " + status);
    			switch (status) {
    				case LocationProvider.AVAILABLE:
    					locationAvailable = true;
    					fineGpsStatus = 0;
    					break;
    				case LocationProvider.OUT_OF_SERVICE:
    					locationAvailable = false;
    					fineGpsStatus = 1;
    					break;
    				case LocationProvider.TEMPORARILY_UNAVAILABLE:
    					locationAvailable = false;
    					fineGpsStatus = 2;
    					break;
    			}
    		}
    		public void onProviderDisabled(String provider) {
//    			Log.i("Kinoma", "GPS listenerFine Provider disabled" + provider);
    		}
    		public void onProviderEnabled(String provider) {
//   			Log.i("Kinoma", "GPS listenerFine Provider enabled" + provider);
    		}
    		public void onLocationChanged(Location location) {
    			double lat, lng, alt, heading = 0.0, speed = 0.0, UTC, acc = 0.0;
    			int visible = 1;

    			if (null == location)
    				return;

    			if (isBetterLocation(location, gLastFineLocation)) {
    				gLastFineLocation = location;
    			}
    			else {
//    				Log.i("Kinoma", "GPS onLocationChanged (fine) - location no better");
    				return;
    			}

    			lat = location.getLatitude();
    			lng = location.getLongitude();
    			alt = location.getAltitude();
    			if (location.hasBearing())
    				heading = location.getBearing();
    			if (location.hasSpeed())
    				speed = location.getSpeed();
    			UTC = (double)location.getTime() / 1000.0;
    			if (location.getExtras() != null)
    				visible = location.getExtras().getInt("satellites");
    			if (location.hasAccuracy())
    				acc = location.getAccuracy();

  			Log.i("Kinoma", "GPS listenerFine onLocationChanged lat:" + lat + " lng:" + lng +
   					" alt:" + alt + " bearing:" + heading + " speed:" + speed + " acc:" + acc);
    			fskSetGPSInfo(lat, lng, alt, heading, speed, fineGpsStatus, visible, UTC, acc);

            }
    	};

    if (1 == 1) {
    	listenerCoarse = new LocationListener() {
    		public void onStatusChanged(String provider, int status, Bundle extras) {
    			Log.i("Kinoma", "GPS listenerCoarse onStatusChanged" + provider + " - status: " + status);
    			switch (status) {
    				case LocationProvider.AVAILABLE:
    					locationAvailable = true;
    					coarseGpsStatus = 0;
    					break;
    				case LocationProvider.OUT_OF_SERVICE:
    					locationAvailable = false;
    					coarseGpsStatus = 1;
    					break;
    				case LocationProvider.TEMPORARILY_UNAVAILABLE:
    					locationAvailable = false;
    					coarseGpsStatus = 2;
    					break;
    			}
    		}
    		public void onProviderDisabled(String provider) {
 //   			Log.i("Kinoma", "GPS listenerCoarse Provider disabled" + provider);
    		}
    		public void onProviderEnabled(String provider) {
 //   			Log.i("Kinoma", "GPS listenerCoarse Provider enabled" + provider);
    		}
    		public void onLocationChanged(Location location) {
    			double lat, lng, alt, heading = 0.0, speed = 0.0, UTC, acc = 0.0;
    			int visible = 1;

    			if (null == location)
    				return;

    			if (isBetterLocation(location, gLastCoarseLocation)) {
    				gLastCoarseLocation = location;
    			}
    			else {
    				Log.i("Kinoma", "GPS onLocationChanged (coarse) - location no better");
    				return;
    			}

    			lat = location.getLatitude();
    			lng = location.getLongitude();
    			alt = location.getAltitude();
    			if (location.hasBearing())
    				heading = location.getBearing();
    			if (location.hasSpeed())
    				speed = location.getSpeed();
    			UTC = (double)location.getTime() / 1000.0;
    			if (location.getExtras() != null)
    				visible = location.getExtras().getInt("satellites");
    			if (location.hasAccuracy())
    				acc = location.getAccuracy();

 //   			Log.i("Kinoma", "GPS listenerCoarse onLocationChanged lat:" + lat + " lng:" + lng +
 //  					" alt:" + alt + " bearing:" + heading + " speed:" + speed + " acc:" + acc);

    			fskSetGPSInfo(lat, lng, alt, heading, speed, coarseGpsStatus, visible, UTC, acc);
    		}
    	};
    }

     }
//#endif

    /*******************************************************/
	int lastLevel = 0;
	int lastPlugged = 0;

	private BroadcastReceiver batteryReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {

			int level = intent.getIntExtra("level", 0);
			int plugged = intent.getIntExtra("plugged", 0);
			//	   int status = intent.getIntExtra("status", BatteryManager.BATTERY_STATUS_UNKNOWN);
			if (level != lastLevel) {
				fskPhoneStateChanged(kFskBatteryLevel, level);
				lastLevel = level;
			}
			if (plugged != lastPlugged) {
				fskPhoneStateChanged(kFskBatteryPlugged, plugged);
				lastPlugged = plugged;
			}
		}
	};

	/*******************************************************/
	private BroadcastReceiver checkScreenModeReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			if (intent.getAction().equals(Intent.ACTION_SCREEN_OFF)) {
				mScreenOn = false;
				if (!mPaused)
					fskPhoneStateChanged(kFskBacklightOn, 0);
			}
			else if (intent.getAction().equals(Intent.ACTION_SCREEN_ON)) {
				mScreenOn = true;
				if (!mPaused)
					fskPhoneStateChanged(kFskBacklightOn, 1);
			}
		}
	};

	/*******************************************************/
//#ifdefined READ_SMS
	private String getMessagesFromIntent(Intent intent) {
		String injectMsg = "";
	    Bundle bundle = intent.getExtras();
	    try{
	    	Object pdus[] = (Object [])bundle.get("pdus");
	        for(int n=0; n < pdus.length; n++) {
	        	byte[] byteData = (byte[])pdus[n];
	        	SmsMessage msg;
	        	Uri uri;
	        	msg = SmsMessage.createFromPdu(byteData);
	        	injectMsg += "x-kp5://log?what=sms.receive&body=";
	        	injectMsg += Uri.encode(msg.getDisplayMessageBody());
	        	injectMsg += "&sender=";
	        	injectMsg += msg.getDisplayOriginatingAddress();		//@@ may be null
	        	injectMsg += "&when=";
	        	injectMsg += (msg.getTimestampMillis() / 1000);
	        }
	    }
	    catch(Exception e)
	    {
	       Log.e("Kinoma", "GetSMSMessages fail", e);
	    }
	    return injectMsg;
    }
//#endif

    /*******************************************************/

//#ifdefined RECEIVE_SMS
	private BroadcastReceiver smsReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent)
		{
			if (!intent.getAction().equals("android.provider.Telephony.SMS_RECEIVED")) {
				return;
			}

			String injectMsg = "";
		    Bundle bundle = intent.getExtras();
		    try{
		    	Object pdus[] = (Object [])bundle.get("pdus");
		        for(int n=0; n < pdus.length; n++) {
		        	byte[] byteData = (byte[])pdus[n];
		        	SmsMessage msg;
		        	Uri uri;
		        	msg = SmsMessage.createFromPdu(byteData);
		        	injectMsg += "x-kp5://log?what=sms.receive&body=";
//					Log.i("Kinoma", "MessageListener: " +  msg.getDisplayMessageBody());

		        	injectMsg += Uri.encode(msg.getDisplayMessageBody());
		        	injectMsg += "&sender=";
		        	injectMsg += msg.getDisplayOriginatingAddress();		//@@ may be null
		        	injectMsg += "&when=";
		        	injectMsg += (msg.getTimestampMillis() / 1000);

		        	callFsk(kJNIFskReceivedSMS, injectMsg);
		        }
		    }
		    catch(Exception e)
		    {
		       Log.e("Kinoma", "SMS GetMessages fail", e);
		    }

		}
	};
//#endif


//	final Runnable mDoDelayedInit = new Runnable() {
//		public void run() {
//			Log.i("mDoDelayedInit", "starting preloadApps");

//			Thread t = new Thread() {
//				public void run() {
//					mPreloadApps.lock();
//					if (0 == mApplistLoaded);
//						preloadApps(1);
//					mPreloadApps.unlock();
//				}
//			};
//			t.setPriority(Thread.MIN_PRIORITY);
//			t.start();
//		}
//	};

    /*******************************************************/


	@Override
	public void onLowMemory() {
		Log.i("Kinoma", "onLowMemory");
		super.onLowMemory();
		if (gInitialized)
			callFsk(kJNILowMemory, "");
	}

    /*******************************************************/


	void pauseWakeLock() {
//		Log.i("Kinoma", "Pausing -- has wakelock type " + mHasWakeLock + " - remember that");
		if (mHasWakeLock != 0) {
//			Log.i("Kinoma", "Pausing -- had wakelock type " + mHasWakeLock + " - remember that");
			mWakeLock.release();
			mHadWakeLock = mHasWakeLock;
			mHasWakeLock = 0;
		}
	}

	void resumeWakeLock() {
//		Log.i("Kinoma", "Resuming -- mHasWakeLock: " + mHasWakeLock + " - mHadWakeLock:" + mHadWakeLock + " - recreate that");
		if (0 != mHadWakeLock) {
//			Log.i("Kinoma", "Resuming -- had wakelock type " + mHadWakeLock + " - recreate that");
			if (mHadWakeLock == 1) {
				mWakeLock = mPowerManager.newWakeLock(
						PowerManager.SCREEN_BRIGHT_WAKE_LOCK
						| PowerManager.ACQUIRE_CAUSES_WAKEUP
						| PowerManager.ON_AFTER_RELEASE, "KinomaFullWake");
				mWakeLock.setReferenceCounted(false);
				mHasWakeLock = 1;
				mHadWakeLock = 0;
			}
			else if (mHadWakeLock == 2) {
				mWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "KinomaPartialWake");
				mWakeLock.setReferenceCounted(false);
				mHasWakeLock = 2;
				mHadWakeLock = 0;
			}
		}
	}

    /*******************************************************/
	// when activity loses focus
	@Override
	protected void onPause() {
		Log.i("Kinoma", " - onPause()");
		if (mInitPhase > 4) {
			unregisterReceiver(batteryReceiver);
		}

		if (mFrameCallback != null) {
			mChoreographer.removeFrameCallback(mFrameCallback);
		}

		if (gInitialized) {
			callFsk(kJNIExtendAudioBuffer, "");

			callFsk(kJNIWindowDeactivated, "");
//			Log.i("Kinoma", "Pausing --- so free up some memory for system");
		//	callFsk(kJNILowMemory, "");

			pauseWakeLock();

			if (0 != mViewingWeb) {
//				Log.i("Kinoma", "onPause - was viewing web, shut down web part");
				killWebView();
			}

   			mWasFullscreen = mIsFullscreen;
   			setFullscreen(false);

			if (0 != doIsIMEEnabled(0))
				doIMEEnable(0);

//			if (0 != callFskInt(kGetOpenGL, 0))
//				doPause();

		}
		mPaused = true;
//#ifdefined C2D_MESSAGE
		active = false;
//#endif

		super.onPause();

		CookieSyncManager.getInstance().stopSync();
	}

    /*******************************************************/


@Override
	protected void onResume() {
		Log.i("Kinoma", " - onResume()");
		if (gInitialized) {
//			Log.i("Kinoma", "onResume - invalWindow");

//			if (0 != callFskInt(kGetOpenGL, 0))
//				doResume();

			callFsk(kJNIReduceAudioBuffer, "");

			if (mInitPhase > 4) {
				registerReceiver(batteryReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
			}

			xdoIMEEnable(0);

//			Log.i("Kinoma", "onResume - activate window");
			callFsk(kJNIWindowActivated, "");
			callFsk(kJNIFskInvalWindow, "");

//			resumeWakeLock();

			Log.i("Kinoma", "onResume: Informing Kinoma that screen is " + (mScreenOn ? "on" : "off"));
			fskPhoneStateChanged(kFskBacklightOn, mScreenOn ? 1 : 0);

			mLastVol = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
			Log.i("Kinoma", "onResume: get volume returns " + mLastVol);
			callFskInt(kSetVolume, mLastVol);
		}

//		Log.i("Kinoma", "onResume: - about to wake main thread");
		mainRunloop.sendEmptyMessageDelayed(kResumeMessage, 100);

		if (mFrameCallback != null) {
			mChoreographer.postFrameCallback(mFrameCallback);
		}

//#ifdefined ACCESS_FINE_LOCATION
		if (mCheckGPSOnResume) {
//			Log.i("Kinoma", "onResume - try to install GPS");
			installGPS();
		}
//#endif

		mPaused = false;
//#ifdefined C2D_MESSAGE
		active = true;
//#endif
//		Log.i("Kinoma", "onResume - about to super.onResume");
		super.onResume();

//		Log.i("Kinoma", "onResume - about to registerRemoteControl");
		registerRemoteControl();

//#ifdefined C2D_MESSAGE
		Log.i("Kinoma", "onResume - about to registerRemoteNotification");
		registerRemoteNotification();
//#endif

        CookieSyncManager.getInstance().startSync();
	}

    /*******************************************************/

	@Override
	protected void onDestroy() {
	    Log.i("Kinoma", "Android system calling 'onDestroy'. Prepare to die.");
     	if (gInitialized) {
			callFsk(kJNIStopFsk, "");
			for (int i = 0; i < 100; i++) {
//				Log.i("Kinoma", "onDestroy - idle Fsk a little longer");
				callFsk(kJNIIdleFsk, " ");
			}
     	}

     	// delete wifi lock?

//     	mAudioManager.unregisterMediaButtonEventReceiver(mRemoteControlResponder);
		unregisterRemoteControl();

		stopService(gSvc);

	    Log.i("Kinoma", "super.onDestroy");
	    super.onDestroy();

	}


    /*******************************************************/


	private Handler mainRunloop = new Handler() {
		public void handleMessage(Message msg) {
			if (endInitPhase > mInitPhase) {
				 doInit(mInitPhase++);
				return;
			}

			int nextCallMS = 0;
			int iter;

			if (msg.what == kIdleMessage)
				mIdlePending = false;

			if (msg.what == kAirplaneModeOff) {
				String op = null;
				if (ConnectivityManager.isNetworkTypeValid(ConnectivityManager.TYPE_MOBILE)) {
//#ifdefined ACCESS_NETWORK_STATE
					op = mTelephonyManager.getNetworkOperatorName();
//					Log.i("Kinoma", "airplane mode off - trying to set operator to " + op);
					fskPhoneOperatorChanged(op);
//#endif
				}
				if (op == null || op.length() == 0) {
					if (mAirplaneModeOffCheck++ < 30)
						mainRunloop.sendEmptyMessageDelayed(kAirplaneModeOff, 2000);
				}
				return;
			}

			if (msg.what == kDismissKeyboard && gPendingIMEClose) {
//				Log.i("Kinoma", "delayed dismissal of keyboard");
				gPendingIMEClose = false;
				doIMEEnable(0);
				return;
			}

			if (msg.what == kResumeMessage) {
				callFsk(kJNIWindowActivated, "");
//				mFskView.mNeedsActivation = true;
//callFsk(kJNIWindowActivated, "");
//callFsk(kJNIFskInvalWindow, "");

				resumeWakeLock();
			}

			if (msg.what == kFullscreenMessage) {
				setFullscreen(true);
			}

//#ifdefined ACCESS_FINE_LOCATION
			if (msg.what == kGPSOffMessage ) {
                if  (null != locationManager) {
                	if (null != listenerFine)
                		locationManager.removeUpdates(listenerFine);
                	listenerFine = null;
                	if (null != listenerCoarse)
                		locationManager.removeUpdates(listenerCoarse);
                	listenerCoarse = null;
                	locationManager = null;
                }
				mGpsPendingDispose = false;
				mGpsUpdaterInstalled = false;
				locationAvailable = false;
				return;
			}
//#endif

			iter = 0;
			while (0 == nextCallMS) {
				nextCallMS = callFsk(kJNIIdleFsk, " ");
				if (-1 == nextCallMS) {
					//   	   stopFsk();
					System.runFinalization();
					System.exit(0);
				}
//				if (iter++ > 5) {
//					Log.i("Kinoma", "runloop " + iter + "  iterations");
//				}
			}

			if (!mIdlePending && (nextCallMS > 0)) {
				mainRunloop.removeMessages(kTimerMessage);
				mainRunloop.sendEmptyMessageDelayed(kTimerMessage, nextCallMS);
			}
		}
	};

	/*******************************************************/
	void wakeIfNecessary() {
		if (!mIdlePending) {
			mIdlePending = true;
			mainRunloop.sendEmptyMessage(kIdleMessage);
		}
	}

	boolean mWebViewActivated;

	/*******************************************************/
//	@Override
	public boolean dispatchTouchEvent(MotionEvent event)
	{
		if (isWebviewTouchEvent(event)) {
			mWebViewActivated = true;
			return super.dispatchTouchEvent(event);
		}
		mWebViewActivated = false;

		int j = 0, max;
		int action = event.getAction();

		if (!gInitialized || (0 != mViewingWeb))
			return super.dispatchTouchEvent(event);

		final int yOffset = mIsFullscreen ? 0 : mVerticalOffset;
		final int pointerCount = event.getPointerCount();

//		Log.i("Kinoma", "dispatchTouchEvent - " + pointerCount + " fingers");

		if (MotionEvent.ACTION_MOVE == action) {
			int[] ptAndTicks = new int[4 * pointerCount];

			if (0 == 1) {	// do we need the point history? Not currently used at this time, so just toss
				int historySize = event.getHistorySize();
				max = (historySize + 1) * pointerCount;
//				Log.i("Kinoma", "ACTION_MOVE - max: " + max + " pointerCount:" + pointerCount + " getHistorySize:" + historySize);
				//int[] ptAndTicks = new int[max*4];
				int evTime;

				for (int h=0; h < historySize; h++) {
					evTime = (int)(event.getHistoricalEventTime(h) - mStartTime);
					for (int p = 0; p < pointerCount; p++) {
						int x = (int)event.getHistoricalX(p, h);
						int y = (int)event.getHistoricalY(p, h);
						ptAndTicks[j++] = (int)event.getPointerId(p);
						ptAndTicks[j++] = x;
						ptAndTicks[j++] = y - yOffset;
						ptAndTicks[j++] = evTime;
//						Log.i("Kinoma", "ACTION_MOVE - j: " + j + " zOMG[" + p + "] t:" + ptAndTicks[j-1] + " x:" + ptAndTicks[j-3] + " y:" + ptAndTicks[j-2]);

					}
				}
			}	// end if (0)

			for (int p = 0; p < pointerCount; p++) {
				ptAndTicks[j++] = (int)event.getPointerId(p);
				ptAndTicks[j++] = (int)event.getX(p);
				ptAndTicks[j++] = (int)event.getY(p) - yOffset;
				ptAndTicks[j++] = (int)(event.getEventTime() - mStartTime);
//				Log.i("Kinoma", "ACTION_MOVE doFskMotionMultipleTouch - j: " + j + " zOMG2[" + p + "] t:" + ptAndTicks[j-1] + " x:" + ptAndTicks[j-3] + " y:" + ptAndTicks[j-2]);
//								Log.i("Kinoma", "  scaled    - j: " + j + " zOMG2[" + p + "] t:" + ptAndTicks[j-1] + " x:" +  ((ptAndTicks[j-3] * 2) /3) + " y:" +  ((ptAndTicks[j-2] * 2) /3) );
			}

			doFskMotionMultipleTouch(ptAndTicks);
			wakeIfNecessary();
			return true;
		}

		int pointer = 0;
		max = pointerCount;

		if (((action & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_POINTER_DOWN)
			|| ((action & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_POINTER_UP))
			{
			pointer = ((action & MotionEvent.ACTION_POINTER_ID_MASK) >> MotionEvent.ACTION_POINTER_ID_SHIFT);
		}
		if (pointer != ((action & MotionEvent.ACTION_POINTER_ID_MASK) >> MotionEvent.ACTION_POINTER_ID_SHIFT)) {
			Log.i("Kinoma ***", " pointer is different.");
		}

		mThereWasSomeMouseAction = true;
		doFskMotionTouch(event.getAction(), (int)event.getPointerId(pointer), (int)event.getX(pointer), (int)event.getY(pointer) - yOffset, (int)(event.getEventTime() - mStartTime));

		wakeIfNecessary();
		return true;
	}


	/*******************************************************/
	@Override
	public boolean dispatchKeyEvent(KeyEvent event)
	{
		int code;

		if (!gInitialized)
			return super.dispatchKeyEvent(event);

		if (mWebViewActivated) {
			Log.i("Kinoma", "WebView mode text handling");
			return super.dispatchKeyEvent(event);
		}

		if (0 != mViewingWeb) {
			if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
				killWebView();
				return true;
			}
			return super.dispatchKeyEvent(event);
		}

		code = event.getKeyCode();

		// Don't capture volume up/down key events
		if (KeyEvent.KEYCODE_VOLUME_UP == code || KeyEvent.KEYCODE_VOLUME_DOWN == code) {
			return super.dispatchKeyEvent(event);
		}

//		Log.i("Kinoma", "key event: keyCode " + event.getKeyCode());
		if (false != mInputMethodShown) {
			int action;


			switch (code) {
				case KeyEvent.KEYCODE_BACK:
				case KeyEvent.KEYCODE_DPAD_UP:
				case KeyEvent.KEYCODE_DPAD_DOWN:
				case KeyEvent.KEYCODE_DPAD_CENTER:
				case KeyEvent.KEYCODE_DPAD_RIGHT:
				case KeyEvent.KEYCODE_DPAD_LEFT:
					break;
				default:
					if (super.dispatchKeyEvent(event)) {
						if (code != KeyEvent.KEYCODE_ENTER) {
//							Log.i("Kinoma", "#### IME ate the Enter, but we need to pass it to Play for handling");
							return true;
						}
					}
					else
						if (code == KeyEvent.KEYCODE_ENTER) {
//							Log.i("Kinoma", "###### IME didn't eat ENTER, pass it to Play");
						}
				}

				int inputType;
				inputType = mFskEditText[0].getInputType();
				if ((inputType & InputType.TYPE_TEXT_FLAG_MULTI_LINE) != 0) {
//					Log.i("Kinoma", "## Multiline input - should we eat the enter? (yes)");
					return true;
				}

				action = event.getAction();

				if (mSoftKeyboardType == kAndroidSoftKeyboardTypeHTC) {
					// incredible
					switch (code) {
						case KeyEvent.KEYCODE_ENTER:
							Log.i("Kinoma", " pulsing unhandled keycode " + code);
							doFskKeyEvent(code, event.getMetaState(), KeyEvent.ACTION_DOWN, event.getUnicodeChar(event.getMetaState()), event.getRepeatCount());
							return doFskKeyEvent(code, event.getMetaState(), KeyEvent.ACTION_UP, event.getUnicodeChar(event.getMetaState()), event.getRepeatCount());
						case KeyEvent.KEYCODE_DPAD_UP:
						case KeyEvent.KEYCODE_DPAD_DOWN:
						case KeyEvent.KEYCODE_DPAD_CENTER:
						case KeyEvent.KEYCODE_DPAD_LEFT:
						case KeyEvent.KEYCODE_DPAD_RIGHT:
							return doFskKeyEvent(code, event.getMetaState(), action, event.getUnicodeChar(event.getMetaState()), event.getRepeatCount());
					}
					return false;
				}
				else {
					// nexus one
					switch (code) {
						case KeyEvent.KEYCODE_DPAD_UP:
						case KeyEvent.KEYCODE_DPAD_DOWN:
						case KeyEvent.KEYCODE_DPAD_CENTER:
						case KeyEvent.KEYCODE_ENTER:
						case KeyEvent.KEYCODE_DPAD_LEFT:
						case KeyEvent.KEYCODE_DPAD_RIGHT:
							return doFskKeyEvent(code, event.getMetaState(), action, event.getUnicodeChar(event.getMetaState()), event.getRepeatCount());
					}
					return false;
				}
		}

		if (doFskKeyEvent(code, event.getMetaState(), event.getAction(), event.getUnicodeChar(event.getMetaState()), event.getRepeatCount())) {
			wakeIfNecessary();
			return true;
		}

//		Log.i("Kinoma", "didn't handle keycode: " + event.getKeyCode());
		return super.dispatchKeyEvent(event);
	}

	/*******************************************************/
	int lastKbdType = -1;
	int lastOrientation = -1;

	@TargetApi(8)
	@Override public void onConfigurationChanged(Configuration newConfig) {
		int kbdType = 0;
		super.onConfigurationChanged(newConfig);
		if (!gInitialized)
			return;
//		Log.i("Kinoma", "KinomaPlay new configuration");
		if (newConfig.hardKeyboardHidden == Configuration.HARDKEYBOARDHIDDEN_NO) {
			if (newConfig.keyboard == Configuration.KEYBOARD_12KEY)
				kbdType = kFskKeyboardTypePhone12Keys;
			else if (newConfig.keyboard == Configuration.KEYBOARD_QWERTY)
				kbdType = kFskKeyboardTypeAlphanumeric;
			else
				kbdType = kFskKeyboardTypeVirtual;
		}
		else
			kbdType = kFskKeyboardTypeVirtual;

		if (mHasGetRotation) {
			mOrientation = mDisplay.getRotation();
//			Log.i("Kinoma", "Orientation - " + mOrientation);
		}
		else {
			switch (newConfig.orientation ) {
			case Configuration.ORIENTATION_LANDSCAPE:
				mOrientation = 1;
				break;
			case Configuration.ORIENTATION_PORTRAIT:
				mOrientation = 0;
				break;
			case Configuration.ORIENTATION_SQUARE:
				mOrientation = 0;
				break;
			case Configuration.ORIENTATION_UNDEFINED:
				break;
			}
		}
		if (mOrientation != lastOrientation) {
			setDeviceOrientation(mOrientation);
			lastOrientation = mOrientation;
			if (mInputMethodShown)
				xdoIMEEnable(0);
		}
		if (kbdType != lastKbdType) {
			setFskKeyboardType(kbdType);
			lastKbdType = kbdType;
		}

		if (false == newConfig.locale.getLanguage().equals(gLastLanguage)) {
			gLastLanguage = newConfig.locale.getLanguage();
			setAndroidLanguage(gLastLanguage);
		}

	}


	/*******************************************************/
	public int fskJNIFetch(int selector, int value) {
		int ret = 0;

//		Log.i("Kinoma", "fskJNIFetch - sel: " + selector + " value: " + value);
		switch (selector) {
		case 1:	// JNIFETCH_SD_MOUNTED
			if (android.os.Environment.getExternalStorageState().equals(android.os.Environment.MEDIA_MOUNTED))
				ret = 1;
			break;
		case 2:	// JNIFETCH_IME_ENABLED
			// not used yet
			break;
		case 3: // JNIFETCH_PHONE_LOG_START
			ret = doStartCallLog();
			break;
		case 4:	// JNIFETCH_PHONE_LOG_STOP
//			doStopCallLog();
			break;
		case 5:	// JNIFETCH_PHONE_LOG_NEXT
//			ret = doNextCallLog();
			break;
		case 6:
//			mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
			mLastVol = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
//			Log.i("Kinoma", " get volume returns " + mLastVol);
			ret = mLastVol;
			break;
		case 7:
			return trampolineInstalled() ? 1 : 0;
		default:
			break;
 		}

		wakeIfNecessary();
		return ret;
	}


 	int wifiLock = 0;

	int mHasWakeLock = 0;
	int mHadWakeLock = 0;

	/*******************************************************/
	public int androidIntent(int what, String action, String uri) {
		int ret = 0;

		Log.i("Kinoma", "androidIntent - what:" + what + " action:" + action + " uri:" + uri);
		try {
			Intent intent = null;
			if (uri.equals(""))
				intent = new Intent(action);
			else
				intent = new Intent(action, Uri.parse(uri));

			switch (what) {
			case 0: // canDo
				ComponentName theActivity;
				theActivity = intent.resolveActivity(mPackageManager);
				if (theActivity.toShortString().length() != 0)
					ret = 1;
				break;
			case 1: // start
				startActivity(intent);
				break;
			}
		}
	    catch(Exception e)
	    {
	       Log.e("Kinoma", "androidIntent fail", e);
	       ret = -1;
	    }

		return ret;
	}

	/*******************************************************/
	public int androidIntentClass(int what, String action, String packageName, String className) {
		int ret = 0;

		Log.i("Kinoma", "androidIntentClass - what:" + what + " packageName:" + packageName + " className:" + className);
		try {
//			Intent intent = new Intent(action);
			Intent intent = new Intent("android.settings.WIFI_SETTINGS");

//			intent.setClassName(packageName, className);

			switch (what) {
			case 2: // canDo
				ComponentName theActivity;
				theActivity = intent.resolveActivity(mPackageManager);
				if (theActivity.toShortString().length() != 0)
					ret = 1;
				break;
			case 3: // start
				startActivity(intent);
				break;
			}
		}
	    catch(Exception e)
	    {
	       Log.e("Kinoma", "androidIntentClass fail", e);
	       ret = -1;
	    }

		return ret;
	}

	/*******************************************************/
	@Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);

        if (Intent.ACTION_MAIN.equals(intent.getAction())) {
            // This gets called when the user hits the Home button from outside of Kinoma Play
            // Close the menu
            getWindow().closeAllPanels();
			doFskKeyEvent(KeyEvent.KEYCODE_HOME, 0, KeyEvent.ACTION_UP, 0, 0);
        }
        else if ("KinomaPlay.installAndGo".equals(intent.getAction())) {
        	Bundle extras = intent.getExtras();
        	if (extras != null) {
            String[] appicationList = extras.getStringArray( "apps" );
            String launchStartApplication = extras.getString( "launch.app" );
            String launchStartService = extras.getString( "launch.service" );

            doInstallAndGo( appicationList, launchStartApplication, launchStartService );
        	}
        }
        else if ("KinomaPlay.launch".equals(intent.getAction())) {
        	Bundle extras = intent.getExtras();
        	if (extras != null) {
            String launchStartApplication = extras.getString( "launch.app" );
            String LaunchStartService = extras.getString( "launch.service" );

            doLaunch( launchStartApplication, LaunchStartService );
        	}
        }
    }

    void doInstallAndGo(String[] installAppList, String launchApp, String launchService )
    {
       if( installAppList != null )
       {
          for( int j = 0; j < installAppList.length; j++ )
          {
             if( j < (installAppList.length - 1) )
                callFsk( kJNIFskReceivedSMS, "x-id://debug.kinoma.com/install?path=" + URLEncoder.encode( installAppList[j] ) );
             else
                callFsk( kJNIFskReceivedSMS, "x-id://debug.kinoma.com/install?path=" + URLEncoder.encode( installAppList[j] ) +
                                                 "&launch=x-id://" + launchApp + (launchService != null ? launchService : "/main") );
          }
       }
    }

    void doLaunch( String launchApp, String launchService ) {
		callFsk( kJNIFskReceivedSMS, "x-id://debug.kinoma.com/launch?uri=x-id://" + launchApp + (launchService != null ? launchService : "/main") );
    }

	int mLastVol = -1;

	/*******************************************************/
	public void fskJNIControl(int what, int value) {
//	      Log.i("Kinoma", "JNIControl - what is "  + what + " - value is " + value);

		switch (what) {
		case 1:	// SLEEP
//			Log.i("Kinoma", "JNIControl - what is "  + what + " (sleep) - value is " + value);
//			if (mPaused) {
//				Log.i("Kinoma", " -CURRENTLY PAUSED- had " + mHadWakeLock +
//						" wanted wakelock type " + value + " - remember that");
//				mHadWakeLock = value;		// just set it up
//			}
//			else {
				switch (value) {
				case 0:				// release
					if (0 != mHasWakeLock) {
//						Log.i("Kinoma", "hasWakeLock - release");
						mWakeLock.release();
						mHasWakeLock = 0;
					}
					mHadWakeLock = 0;
					break;
				case 1:				// don't dim
//					Log.i("Kinoma", " - change to don't dim");
					if (mHasWakeLock == 1) {
//						Log.i("Kinoma", "hasWakeLock - do nothing, already have type 1");
					}
					else if (mHasWakeLock == 2) {
//						Log.i("Kinoma", "hasWakeLock - release wakelock type 2");
						mWakeLock.release();
					}
					else if (mHasWakeLock == 3) {
//						Log.i("Kinoma", "hasWakeLock - release wakelock type 3");
						mWakeLock.release();
					}

					if (mHasWakeLock == 3 || mHasWakeLock == 2 || mHasWakeLock == 0) {
//						Log.i("Kinoma", "hasWakeLock - create wakelock type 1 (full)");
						mWakeLock = mPowerManager.newWakeLock(
								PowerManager.SCREEN_BRIGHT_WAKE_LOCK
								| PowerManager.ACQUIRE_CAUSES_WAKEUP
								| PowerManager.ON_AFTER_RELEASE, "KinomaFullWake");
						mWakeLock.setReferenceCounted(false);
					}
					mHasWakeLock = 1;
					mWakeLock.acquire();
					break;
				case 2:				// screen off ok, dont sleep
//					Log.i("Kinoma", " - change to dim ok, don't sleep");
					if (mHasWakeLock == 1) {
//						Log.i("Kinoma", "hasWakeLock - release wakelock type 1");
						mWakeLock.release();
					}
					else if (mHasWakeLock == 2) {
//						Log.i("Kinoma", "hasWakeLock - do nothing, already have type 2");
					}
					else if (mHasWakeLock == 3) {
//						Log.i("Kinoma", "hasWakeLock - release wakelock type 3");
						mWakeLock.release();
					}

					if (mHasWakeLock == 3 || mHasWakeLock == 1 || mHasWakeLock == 0) {
//						Log.i("Kinoma", "hasWakeLock - create wakelock type 2 (partial wake)");
						mWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "KinomaPartialWake");
						mWakeLock.setReferenceCounted(false);
					}
					mHasWakeLock = 2;
					mWakeLock.acquire();

					break;
				case 3:				// dim ok, dont sleep
//					Log.i("Kinoma", " - change to dim ok, don't sleep");
					if (mHasWakeLock == 1) {
//						Log.i("Kinoma", "hasWakeLock - release wakelock type 1");
						mWakeLock.release();
					}
					else if (mHasWakeLock == 2) {
//						Log.i("Kinoma", "hasWakeLock - release wakelock type 2");
						mWakeLock.release();
					}
					else if (mHasWakeLock == 3) {
//						Log.i("Kinoma", "hasWakeLock - do nothing, already have type 3");
					}

					if (mHasWakeLock == 2 || mHasWakeLock == 1 || mHasWakeLock == 0) {
//						Log.i("Kinoma", "hasWakeLock - create wakelock type 2 (partial wake)");
						mWakeLock = mPowerManager.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, "KinomaDimWake");
						mWakeLock.setReferenceCounted(false);
					}
					mHasWakeLock = 3;
					mWakeLock.acquire();

					break;
				}
//			}
			break;
//#ifdefined ACCESS_FINE_LOCATION
		case 3: // turn on GPS
			if (false == mGpsUpdaterInstalled) {
//				Log.i("Kinoma", "Install GPS listener");
				installGPS();
			}
			break;
		case 4: // turn off GPS
			if (mGpsUpdaterInstalled) {
//				mGpsUpdaterInstalled = false;
//				Log.i("Kinoma", "Remove GPS listener");
				mGpsPendingDispose = true;
				mainRunloop.sendEmptyMessage(kGPSOffMessage);
		 	}
			break;
//#endif
		case 5: // turn off splash screen
			setContentView(mMain);
			callFsk(kJNIFskInvalWindow, "  ");
			mFskView.invalidate();
			break;
		case 6:	// wake main thread
//			Log.i("Kinoma", "wakeIfNecessary ");
			wakeIfNecessary();
			break;
		case 7: // JNICONTROL_KEEP_WIFI_ALIVE
// 			Log.i("Kinoma", "keepWifiAlive? " + value);
			if (value == 0) {
				wifiLock--;
				mWifiLock.release();
				if (wifiLock != 0) {
//					Log.i("Kinoma", " - wifiLock still held :" + wifiLock);
				}
				else {
//					Log.i("Kinoma", " - releasing wifiLock");
				}
			}
			else {
//				Log.i("Kinoma", " - acquiring wifiLock :" + wifiLock + " -> " + (wifiLock + 1));
				wifiLock++;
				mWifiLock.acquire();
			}
			break;
		case 8:	// JNICONTROL_SHOW_SYSTEM_BAR
// 			Log.i("Kinoma", "ShowSystemBar? " + value);
// 			if (mViewingWeb == 1)
//				Log.i("Kinoma", "ShowSystemBar but we're doing the web?");

 			if (value == 0)
 				setFullscreen(true);
 			else
 				setFullscreen(false);
 			break;
		case 9:
			mFskView.setInitialized(true);
			gInitialized = true;
			break;
		case 10:
			if (mLastVol != value) {
//				Log.i("Kinoma", " set volume to " + value);
//				mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
				mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, value, 0);
				mLastVol = value;
			}
			break;
		case 11:	// JNICONTROL_RESET_ANDROID_HOME
//			String appSig = getApplication().getPackageName();
//			mPackageManager.clearPackagePreferredActivities(appSig);

			Uri packageURI = Uri.parse("package:com.kinoma.trampoline");
			Intent uninstallIntent = new Intent(Intent.ACTION_DELETE, packageURI);
			startActivity(uninstallIntent);

//			final Intent intent = new Intent("com.kinoma.trampoline/.UninstallerActivity");
//	       intent.setPackage("com.kinoma.trampoline");
//			startActivity(intent);

			break;
		case 98:
			setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
			break;
		case 99:	//
	        //---change to landscape mode---
	        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
	        break;
		}
	}

	/*******************************************************/
	boolean isHoneycomb() {
		if (android.os.Build.VERSION.SDK_INT >= 11 /* android.os.Build.VERSION_CODES.HONEYCOMB */
			&& android.os.Build.VERSION.SDK_INT < 14)
			return true;
		else
			return false;
	}

	/*******************************************************/
	boolean isHoneycombOrLater() {
		if (android.os.Build.VERSION.SDK_INT >= 11) /* android.os.Build.VERSION_CODES.HONEYCOMB */
			return true;
		else
			return false;
	}

	void setFullscreen(Boolean fullscreen) {
		if (fullscreen)
			mainRunloop.removeMessages(kFullscreenMessage);

		if (fullscreen == mIsFullscreen) {
//			Log.i("Kinoma", "fullscreen is correct, don't change");
			return;
		}
		if (fullscreen) {
//			Log.i("Kinoma", "set fullscreen");
			mIsFullscreen = true;
			if (isHoneycomb()) {
				mFskView.setSystemUiVisibility(View.STATUS_BAR_HIDDEN);
			}
			else {
				getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
				getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
			}
		}
		else {
//			Log.i("Kinoma", "clear fullscreen");
			mIsFullscreen = false;
			if (isHoneycomb()) {
				mFskView.setSystemUiVisibility(View.STATUS_BAR_VISIBLE);
			}
			else {
				getWindow().addFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
				getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
			}
		}
 		mFskView.requestLayout();
	}

	/*******************************************************/
	protected void doSetKeyboardSelection(int selBegin, int selEnd) {
		CharSequence curText = mFskEditText[mFskEditTextCur].getText();
		int len = curText.length();
		int alt = mFskEditTextCur == 0 ? 1 : 0;

//		Log.i("Kinoma", "doSetKeyboardSelection alt: " + alt + " selBegin: " + selBegin + " - selEnd: " + selEnd + "len: " + len);
		if (selBegin > len) selBegin = len;
		if (selEnd > len) selEnd = len;

		if ((selBegin == selEnd) && mThereWasSomeMouseAction) {
//			Log.i("Kinoma", "	selBegin == selEnd " + selBegin + " -- I think it's a tap - remove suggestions");
//			Log.i("Kinoma", "   setting alternate field's text to '" + curText + "' and selection to " + selBegin);
			mFskEditTextIgnoreChanges = true;
			mFskEditText[alt].selectAll();
			mFskEditText[alt].setText(curText);
			mFskEditText[alt].requestFocus();
			mFskEditText[alt].setEnabled(true);
			mFskEditText[alt].setSelection(selBegin);
		mInputMethodMgr.restartInput(mFskEditText[alt]);
			mFskEditText[mFskEditTextCur].setEnabled(false);
			mFskEditTextCur = alt;
			mFskEditTextIgnoreChanges = false;
		}
		else {
			if (selBegin == 0 && selEnd == len) {
//				Log.i("Kinoma", "   setting selection ALL on current textfield '" + curText + "' from " + selBegin + " to " + selEnd);
				mFskEditText[mFskEditTextCur].selectAll();
			}
			else {
//				Log.i("Kinoma", "   setting selection on current textfield '" + curText + "' from " + selBegin + " to " + selEnd);
				mFskEditText[mFskEditTextCur].setSelection(selBegin, selEnd);
			}
		}
		mThereWasSomeMouseAction = false;

	}

	/*******************************************************/
	int fskKbdTypeToAndroidInputType(int kbdType) {
		int androidInputType;
		switch (kbdType) {
		case 0:		// normal text
			androidInputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_AUTO_CORRECT;	break;
		case 1:		// multiline text
			androidInputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_AUTO_CORRECT | InputType.TYPE_TEXT_FLAG_MULTI_LINE;	break;
		case 2:		// numeric
			androidInputType = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED;		break;

		case 3:		// password
			androidInputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD;	break;
		case 4:		// email
			androidInputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS; break;
		case 5:		// uri
			androidInputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_URI;		break;
		case 6:		// phone
			androidInputType = InputType.TYPE_CLASS_PHONE;	break;
		default:
			androidInputType = InputType.TYPE_CLASS_TEXT;	break;
		}
//		Log.i("Kinoma", "kbdType: " + kbdType + " converted to android type: " + androidInputType);
		return androidInputType;
	}

	protected void doSetKeyboardHints(String hintText) {
//		Log.i("Kinoma", "setKeyboardHints: + hintText");
		mFskEditText[0].setHint(hintText);
		mFskEditText[1].setHint(hintText);
	}

	protected void doSetTERect(int x, int y, int w, int h) {
		mFskEditText[0].setLayoutParams(new AbsoluteLayout.LayoutParams(w, h, x, y));
		mFskEditText[1].setLayoutParams(new AbsoluteLayout.LayoutParams(w, h, x, y));

	}

	/*******************************************************/
	protected void doSetKeyboardStuff(int kbdType, String theContent) {
		int androidInputType;

//		Log.i("Kinoma", "doSetKeyboardStuff " + kbdType + " - theContent " + theContent);
		mFskEditTextIgnoreChanges = true;
		mFskEditText[0].setText(theContent);
		mFskEditText[1].setText(theContent);
		mFskEditTextIgnoreChanges = false;
		mFskEditText[0].selectAll();
		mFskEditText[1].selectAll();

		androidInputType = fskKbdTypeToAndroidInputType(kbdType);

		mFskEditText[0].setInputType(androidInputType);
		mFskEditText[1].setInputType(androidInputType);
	}


	/*******************************************************/
	int doIsIMEEnabled(int ignore) {
		return mInputMethodShown ? 1 : 0;
	}

	final int kEnableKeyboardNothing = 0;
	final int kEnableKeyboardStart = 1;
	final int kEnableKeyboardShowStatus = 1;
	final int kEnableKeyboardWaitForStatus = 2;
	final int kEnableKeyboardEnableKeyboard = 3;
	final int kEnableKeyboardWaitForKeyboard = 4;
	final int kEnableKeyboardIsEnabled = 10;
	final int kEnableKeyboardRequestClose = 11;
	final int kEnableKeyboardRestoreFullscreen = 12;
	final int kEnableKeyboardFullscreenRestored = 13;

	int mKeyboardEnableStage = kEnableKeyboardNothing;
	Boolean mWasFullscreen = false;

	Boolean wantsKeyboard() {

//		Log.i("Kinoma", "wantsKeyboard - mKeyboardEnableStage: " + mKeyboardEnableStage);
		switch (mKeyboardEnableStage) {
		case kEnableKeyboardNothing:
//	xx		if (!mInputMethodShown && mKeyboardRect.height() != 0) {
//				Log.i("Kinoma", "dont want Keyboard - but kbd height is: " + mKeyboardRect.height());
//				xdoIMEEnable(0);
//			}
			return mInputMethodShown;
		case kEnableKeyboardShowStatus:
//			Log.i("Kinoma", "wantsKeyboard ShowStatus- mIsFullscreen: " + mIsFullscreen + " mWasFullscreen:" + mWasFullscreen);
 			mWasFullscreen = mIsFullscreen;
   			if (mWasFullscreen == true) {
 //  				Log.i("Kinoma", "wantsKeyboard - setting Fullscreen (false)");
   				setFullscreen(false);
   				mKeyboardEnableStage = kEnableKeyboardWaitForStatus;
   	   			return true;
   			}
//   			else
//				Log.i("Kinoma", "wantsKeyboard - already showing status bar");
			// fall through
		case kEnableKeyboardWaitForStatus:
//			if (isHoneycomb()) {
			if (isHoneycombOrLater()) {
				// honeycomb doesn't change vertical ofset
			}
			else {
				if (mVerticalOffset == 0) {
//				Log.i("Kinoma", "wantsKeyboard - WaitForStatus - verticalOffset: " + mVerticalOffset);
					return true;
				}
			}
//			Log.i("Kinoma", "wantsKeyboard - WaitForStatus - now showing status bar");
			// fall through
		case kEnableKeyboardEnableKeyboard:
			mKeyboardEnableStage = kEnableKeyboardWaitForKeyboard;
			if (!mInputMethodShown) {
//   				Log.i("Kinoma", "wantsKeyboard - Keyboard not shown - requesting it");
				xdoIMEEnable(1);
				return true;
			}
			// fall through
		case kEnableKeyboardWaitForKeyboard:
//			Log.i("Kinoma", "wantsKeyboard - wait for Keyboard w: " + (mKeyboardRect.right - mKeyboardRect.left)
//					+ " h: " + (mKeyboardRect.bottom - mKeyboardRect.top));
			if ((mKeyboardRect.bottom - mKeyboardRect.top) == 0)
				return true;

			mKeyboardEnableStage = kEnableKeyboardIsEnabled;
			// fall through
		case kEnableKeyboardIsEnabled:
//			Log.i("Kinoma", "wantsKeyboard - kEnableKeyboardIsEnabled-Do nothing   w: " + (mKeyboardRect.right - mKeyboardRect.left)
//					+ " h: " + (mKeyboardRect.bottom - mKeyboardRect.top));
			break;

		case kEnableKeyboardRequestClose:
			if (mInputMethodShown) {
//  				Log.i("Kinoma", "wantsKeyboard RequestClose - Keyboard shown - shutting it down");
   				mKeyboardEnableStage = kEnableKeyboardRestoreFullscreen;
				xdoIMEEnable(0);
//	don't			return false;
			}
			// fall through
		case kEnableKeyboardRestoreFullscreen:
//			Log.i("Kinoma", "wantsKeyboard RestoreFullscreen - mIsFullscreen: " + mIsFullscreen + " mWasFullscreen:" + mWasFullscreen);
			if (mIsFullscreen != mWasFullscreen) {
//   				Log.i("Kinoma", "wantsKeyboard RestoreFullscreen to " + mWasFullscreen);
				mKeyboardEnableStage = kEnableKeyboardFullscreenRestored;
//	don't			setFullscreen(mWasFullscreen);
				mainRunloop.sendEmptyMessageDelayed(kFullscreenMessage, 1000);

			}
			else {
				mKeyboardEnableStage = kEnableKeyboardNothing;
			}
			return false;
		case kEnableKeyboardFullscreenRestored:
			if (mIsFullscreen) {
				if (isHoneycombOrLater()) {
					// Honeycomb doesn't change vertical offset
					mKeyboardEnableStage = kEnableKeyboardNothing;
					return false;
				}
				else {
					if (mVerticalOffset == 0) {
//	   					Log.i("Kinoma", "wantsKeyboard RestoreFullscreen - got fullscreen!");
						mKeyboardEnableStage = kEnableKeyboardNothing;
						return false;
					}
				}
//   				Log.i("Kinoma", "wantsKeyboard RestoreFullscreen - still waiting for fullscreen - vert offset is " + mVerticalOffset);
	                mainRunloop.sendEmptyMessageDelayed(kFullscreenMessage, 1000);

			}
			else {
				if (isHoneycomb()) {
					// Honeycomb doesn't change vertical offset
					mKeyboardEnableStage = kEnableKeyboardNothing;
					return false;
				}
				else {
					if (mVerticalOffset != 0) {
//	   					Log.i("Kinoma", "wantsKeyboard RestoreFullscreen - got status bar: " + mVerticalOffset);
						mKeyboardEnableStage = kEnableKeyboardNothing;
						return false;
					}
				}
 //  				Log.i("Kinoma", "wantsKeyboard RestoreFullscreen - still waiting for status bar - vert offset is " + mVerticalOffset);
                mainRunloop.sendEmptyMessageDelayed(kFullscreenMessage, 1000);

			}
		}
		return mInputMethodShown;
	}


	void doIMEEnable(int enable) {
		Log.i("Kinoma", "doIMEEnable(" + enable + ")");
		if (0 == enable)
			mKeyboardEnableStage = kEnableKeyboardRequestClose;
		else
			mKeyboardEnableStage = kEnableKeyboardStart;
		wantsKeyboard();
	}

	void xdoIMEEnable(int enable) {
//		Log.i("Kinoma", "xdoIMEEnable is " + enable);
		setIMEEnabled(enable);
		mThereWasSomeMouseAction = false;
		callFskInt(kSetKeyboard, enable);
	    if (0 == enable) {
	    	if (mInputMethodShown) {
	    		CharSequence curText = mFskEditText[mFskEditTextCur].getText();
	    		int len = curText.length();
	    		int alt = mFskEditTextCur == 0 ? 1 : 0;
	    		mFskEditTextIgnoreChanges = true;
	    		mFskEditText[alt].setText(curText);
	    		mFskEditTextIgnoreChanges = false;

	    		mFskEditTextCur = 0;
	    		mInputMethodShown = false;
	    		mNeedsKeyboardHeight = true;
	    		mFskEditText[0].setVisibility(View.INVISIBLE);
	    		mFskEditText[1].setVisibility(View.INVISIBLE);
	    		mInputMethodMgr.hideSoftInputFromWindow(mMain.getWindowToken(), 0);
	    	}
	    }
	   	else {
	   		if (!mInputMethodShown) {
	   			mFskEditTextCur = 0;
	   			mFskEditText[0].setVisibility(View.VISIBLE);
	   			mFskEditText[0].requestFocus();
	   			mFskEditText[0].setEnabled(true);
	   			mFskEditText[1].setVisibility(View.VISIBLE);
	   			mFskEditText[1].setEnabled(true);
	   			mInputMethodMgr.showSoftInput(mFskEditText[0], InputMethodManager.SHOW_FORCED);
	   			mInputMethodShown = true;
	   			mNeedsKeyboardHeight = true;
	   		}
		}
	}


	/*******************************************************/
       ProgressBar progress = null;
	   LinearLayout progressParent = null;

	   void killWebView() {
		   mViewingWeb = 0;
		   setContentView(mMain);
		   mWebView.stopLoading();
		   mWebView.clearCache(true);
		   if (mWasInputMethodShowing) {
			   doIMEEnable(1);
			   mWasInputMethodShowing = false;
		   }
		   setFullscreen(mWasFullscreen);
		   progress = null;
		   mWebView.destroy();
		   mWebView = null;
		   mWebMainView = null;
		   callFsk(kJNIWindowActivated, "");
		   callFsk(kJNIFskInvalWindow, "");
		   wakeIfNecessary();
	   }

    private class FskWebChromeClient extends android.webkit.WebChromeClient {

        @Override
        public void onProgressChanged(WebView view, int newProgress) {
 //       	Log.i("Kinoma", "progress " + newProgress);
        	if (progress != null) {
        		progress.setProgress(newProgress);
        	}
        }
    }

	public class FskWebViewClient extends WebViewClient {
		public String mRedirectTo;

		@Override
		public boolean shouldOverrideUrlLoading(WebView view, String url) {
			view.loadUrl(url);
			return true;
		}

		@Override
		public void onLoadResource(WebView view, String url)
		{
//			Log.i("Kinoma","onLoadResource " + url);
			super.onLoadResource(view, url);
		}

		@Override
		public void onPageStarted(WebView view, String url, Bitmap favicon) {
//			Log.i("Kinoma", "onPageStarted " + url);
			super.onPageStarted(view, url, favicon);

			if (null != mRedirectTo) {
				view.loadUrl(mRedirectTo);
				mRedirectTo = null;
			}
			if (null != progress)
				progress.setVisibility(View.VISIBLE);
		}

		@Override
		public void onPageFinished(WebView view, String url) {
//			Log.i("Kinoma", "onPageFinished " + url);
			super.onPageFinished(view, url);

			if (null != mRedirectTo) {
				view.loadUrl(mRedirectTo);
				mRedirectTo = null;
			}
			if (null != progress)
				progress.setVisibility(View.INVISIBLE);

			CookieSyncManager.getInstance().sync();
		}

		@Override
		public void onReceivedError(WebView view, int errorCode, String description, String failingUrl) {
//			Log.i("Kinoma", "onReceivedError on: " + failingUrl + " - description: " + description);
			super.onReceivedError(view, errorCode, description, failingUrl);
		}

		@Override
		public void onReceivedHttpAuthRequest (WebView view, HttpAuthHandler handler, String host, String realm) {
//			Log.i("Kinoma", "onReceivedHttpAuthRequest for host: " + host + " realm: " + realm);
			super.onReceivedHttpAuthRequest(view, handler, host, realm);
		}
		@SuppressLint("NewApi")
		@Override
		public void onReceivedSslError(WebView view, SslErrorHandler handler, SslError error) {
			Log.i("Kinoma", "onReceivedSslError: " + error);

//MDK - this is for kInst - need to handle it better.
			handler.proceed();
//			super.onReceivedSslError(view, handler, error);
		}

		public void setRedirectTo(String where) {
			mRedirectTo = where;
		}
	}

	/*******************************************************/
	public void launchDoc(int what, String url) {
		Intent intent = new Intent();
//		Log.i("Kinoma", "Callback into launchDoc with (" + what + ") URL:" + url);

		switch (what) {
			case 1:
                callFsk(kJNIWindowDeactivated, "");
				mViewingWeb = 1;

				mWasInputMethodShowing = mInputMethodShown;
				if (mWasInputMethodShowing)
					doIMEEnable(0);

	   			mWasFullscreen = mIsFullscreen;
	   			setFullscreen(false);

	   		    Button button;

	   			mWebLayout = LayoutInflater.from(this).inflate(R.layout.web, null);
	   			progress = (ProgressBar)mWebLayout.findViewById(R.id.web_progress);
	   			mWebView = (WebView)mWebLayout.findViewById(R.id.web_view);

	   			mWebView.setLayoutParams(new LinearLayout.LayoutParams
	   	        		   (LinearLayout.LayoutParams.FILL_PARENT, 0, 1.0f));
	   			WebSettings webSettings = mWebView.getSettings();
	   			webSettings.setJavaScriptEnabled(true);
	   			mWebView.setWebViewClient(new FskWebViewClient());
	   			mWebView.setWebChromeClient(new FskWebChromeClient());
	   			webSettings.setJavaScriptCanOpenWindowsAutomatically(true);
	   			webSettings.setCacheMode(WebSettings.LOAD_NO_CACHE);
	   			mWebView.setFocusable(true);
	   			mWebView.setFocusableInTouchMode(true);
	   			mWebView.setClickable(true);
	   			mWebView.setEnabled(true);
	   			mWebView.setLongClickable(true);

	   			mWebMainView = (LinearLayout)mWebLayout.findViewById(R.id.web_mainlayout);
	   	        mWebMainView.setEnabled(true);
	   	        mWebMainView.setClickable(true);

	   	     	setContentView(mWebMainView);

				mWebView.setClickable(true);
				mWebView.requestFocus();
				mWebView.loadUrl(url);
				return;

			case 100:	// not using this at the moment
				Intent webIntent = null;
				webIntent = new Intent("android.intent.action.VIEW", Uri.parse(url));
		        startActivity(webIntent);
		        break;

			case 0:
			case 2:
				if ((url.lastIndexOf(".apk") + 4) == url.length()) {
					intent.setAction(android.content.Intent.ACTION_VIEW);
					intent.setDataAndType(Uri.parse("file://" + url), "application/vnd.android.package-archive");
					startActivity(intent);
				}
				else {
					Intent launchApp;
					if (0 == url.compareTo("com.android.contacts")) {
						launchApp = new Intent(Intent.ACTION_VIEW, ContactsContract.Contacts.CONTENT_URI);
					}
					else {
						launchApp = mPackageManager.getLaunchIntentForPackage(url);
					}
					if (null != launchApp) {
						try {
							startActivity(launchApp);
						}
						catch (Exception e) {
							Log.i("Kinoma", "can't open " + url);
							Intent doit = Intent.createChooser(launchApp, "Choose app to launch");
							if (null != doit) {
								startActivity(doit);
							}
						}
					}
				}
				break;
//			case 3:
//				intent.setAction(Intent.ACTION_MAIN);
//				intent.setComponent(new ComponentName(JIL_CALL_RUNTIME_PACKAGE, JIL_CALL_RUNTIME_CLASS));
//                intent.putExtra("rootdirectory", url);
//                intent.putExtra("arguments", new String[0]);
//                startActivityForResult(intent, 0);
 //               break;

		}
	}


	/*******************************************************/

	String mLaunchables = "";
	SparseArray mIconBuffers = new SparseArray(10);
	HashMap mIconBufferMap = new HashMap();


	byte[] mIconBuffer = {};

	private int doPreloadAppIcon(String appPackageName, String activityName, int iconID, int iconNum) {
//		Log.i("Kinoma", "doPreloadAppIcon" + appPackageName);

		Resources res;
		try {
			res = mPackageManager.getResourcesForApplication(appPackageName);
			InputStream is = res.openRawResource(iconID);
			int amt = is.available();
			mIconBuffer = new byte[amt];
			is.read(mIconBuffer);
			is.close();
			mIconBuffers.append(iconNum, mIconBuffer);
			String key = appPackageName + activityName + "#" + iconID;
			mIconBufferMap.put(key, mIconBuffer);
			mIconBuffer = null;
			return 0;
		} catch (NameNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (Resources.NotFoundException e) {
			return -1;
		}

		return -1;
	}

	/*******************************************************/
	private int doFetchAppIcon(String appPackageName, int iconID) {
		String key = appPackageName + "#" + iconID;
//	int iconNum = (int) mIconBufferMap.get(key);
//		mIconBuffer = (byte[]) mIconBuffers.valueAt(iconNum);
		mIconBuffer = (byte[]) mIconBufferMap.get(key);
		if (null == mIconBuffer)
			return -1;
		return 0;
	}

	/*******************************************************/
	private void doFetchApps(int x) {
		mPreloadApps.lock();
		if (0 == mApplistLoaded)
			preloadApps(1);
		mPreloadApps.unlock();
		return;		//   mLaunchables is already set by preloadApps
	}

	/*******************************************************/
	private void doLaunchApp(String pkgName, String actName) {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
        intent.setComponent(new ComponentName(pkgName, actName));

        startActivity(intent);
	}

	/*******************************************************/
	private void preloadApps(int x) {
		if (x != 1)
			mPreloadApps.lock();
		int iconNum = 0;
		mLaunchables = "";

		Intent mainIntent = new Intent(Intent.ACTION_MAIN, null);
		mainIntent.addCategory(Intent.CATEGORY_LAUNCHER);

		List<ResolveInfo> currentApps = mPackageManager.queryIntentActivities(mainIntent, 0);

		for (int i=0; i < currentApps.size(); i++) {
			ResolveInfo app = currentApps.get(i);
			CharSequence name = app.loadLabel(mPackageManager);
			String packageName = app.activityInfo.applicationInfo.packageName;
			String activityName = app.activityInfo.name;
			int iconRes = app.activityInfo.icon;
			if (iconRes == 0)
				iconRes = app.activityInfo.applicationInfo.icon;

	       	mLaunchables += name + "|*|" + iconRes + "|*|" + packageName + "|*|" + activityName + "|*|";
	        doPreloadAppIcon(packageName, activityName, iconRes, iconNum);
	        iconNum++;
		}

		mApplistLoaded = 1;

		if (x != 1)
			mPreloadApps.unlock();
	}



	// /*******************************************************/

	class WebViewJavaScriptInterface {
		KinomaPlay mPlay;
		String mScript;
		String mResult;

		public WebViewJavaScriptInterface(KinomaPlay play) {
			mPlay = play;
		}

		public void setScript(String script) {
			mScript = script;
			mResult = null;
		}

	    @JavascriptInterface
		public String script() {
			return mScript;
		}

		public String result() {
			return mResult;
		}

	    @JavascriptInterface
		public void setResult(String result) {
			if (mScript != null) {
				mResult = result;
				Log.i("Kinoma", String.format("result = %s", result));
				mPlay.webviewHandleEvaluationResult(result);
				mScript = null;
			}
		}

		public void handleError(ConsoleMessage consoleMessage) {
			if (mScript != null) {
				mPlay.webviewHandleEvaluationResult(null);
				mScript = null;
			}
		}
	};

	HashMap<Integer, WebView> mWebViewMap = new HashMap<Integer, WebView>();
	HashMap<WebView, Integer> mWebViewReverseMap = new HashMap<WebView, Integer>();
	HashMap<Integer, WebViewJavaScriptInterface> mWebViewJSInterfaceMap = new HashMap<Integer, WebViewJavaScriptInterface>();

	private void webviewCreate(final int webviewId) {
		WebView webView = new WebView(this);

		WebSettings webSettings = webView.getSettings();
		webSettings.setJavaScriptEnabled(true);

		webView.setWebViewClient(new WebViewClient() {
			@Override
			public boolean shouldOverrideUrlLoading(WebView view, String url) {
				Integer webviewId = fetchWebViewId(view);
				if (webviewId != null) {
					Log.i("Kinoma", String.format("shouldOverrideUrlLoading(%d) ", webviewId) + url);
					if (!webviewShouldHandleUrl(webviewId, url)) return true;
				}
				return false;
			}

			@Override
			public void onPageStarted(WebView view, String url, Bitmap favicon) {
				Integer webviewId = fetchWebViewId(view);
				if (webviewId != null) {
					Log.i("Kinoma", String.format("onPageStarted(%d) ", webviewId));
					webviewHandleLoading(webviewId);
				}
			}

			@Override
			public void onPageFinished(WebView view, String url) {
				Integer webviewId = fetchWebViewId(view);
				if (webviewId != null) {
					Log.i("Kinoma", String.format("onPageFinished(%d) ", webviewId));
					webviewHandleLoaded(webviewId);
				}
			}

			@Override
			public void onReceivedError(WebView view, int errorCode, String description, String failingUrl) {
				Log.e("Kinoma", String.format("onReceivedError(%h) %d - %s ", view, errorCode, description));
				Log.i("Kinoma", failingUrl);
			}
		});

		webView.setWebChromeClient(new WebChromeClient() {
			@Override
			public boolean onConsoleMessage(ConsoleMessage consoleMessage) {
				Log.i("Kinoma", String.format("onConsoleMessage(%d) - %s - Line %d: %s", webviewId, consoleMessage.messageLevel(), consoleMessage.lineNumber(), consoleMessage.message()));

				WebViewJavaScriptInterface jsInterface = (WebViewJavaScriptInterface)mWebViewJSInterfaceMap.get(webviewId);
				MessageLevel level = consoleMessage.messageLevel();
				if (jsInterface != null && level == MessageLevel.ERROR) {
					jsInterface.handleError(consoleMessage);
				}
				return true;
			}
		});

		WebViewJavaScriptInterface jsInterface = new WebViewJavaScriptInterface(this);
		webView.addJavascriptInterface(jsInterface, "_kinoma");

		webSettings.setCacheMode(WebSettings.LOAD_NO_CACHE);
		webView.setFocusable(true);
		webView.setFocusableInTouchMode(true);
		webView.setClickable(true);
		webView.setEnabled(true);
		webView.setLongClickable(true);

		webView.requestFocus();

		mWebViewMap.put(webviewId, webView);
		mWebViewReverseMap.put(webView, webviewId);
		mWebViewJSInterfaceMap.put(webviewId, jsInterface);

		Log.i("Kinoma", String.format("webviewCreate() => %d, %h, %h", webviewId, webView, webView.getParent()));
	}

	private void webviewDispose(int webviewId) {
		Log.i("Kinoma", String.format("webviewDispose(%d)", webviewId));
		WebView webView = fetchWebView(webviewId);
		if (webView != null) {
			mWebViewMap.remove(webviewId);
			mWebViewReverseMap.remove(webView);
			mWebViewJSInterfaceMap.remove(webviewId);

			webviewDetach(webviewId);
		}
	}

	private void webviewActivated(int webviewId, int activeIt) {
		Log.i("Kinoma", String.format("webviewActivated(%d, %d)", webviewId, activeIt));
		WebView webView = fetchWebView(webviewId);
		if (webView != null) {
			// show hide webview
		}
	}

	private void webviewSetFrame(int webviewId, int x, int y, int width, int height) {
		Log.i("Kinoma", String.format("webviewSetFrame(%d, %d, %d, %d, %d)", webviewId, x, y, width, height));
		WebView webView = fetchWebView(webviewId);
		if (webView != null) {
			AbsoluteLayout.LayoutParams params = new AbsoluteLayout.LayoutParams(width, height, x, y);
			webView.setLayoutParams(params);
		}
	}

	private void webviewAttach(int webviewId) {
		Log.i("Kinoma", String.format("webviewAttach(%d)", webviewId));
		WebView webView = fetchWebView(webviewId);
		if (webView != null) {
			if (webView.getParent() == null) {
				((ViewGroup)mMain).addView(webView);
			}
		}
	}

	private void webviewDetach(int webviewId) {
		Log.i("Kinoma", String.format("webviewDetach(%d)", webviewId));
		WebView webView = fetchWebView(webviewId);
		if (webView != null) {
			if (webView.getParent() != null) {
				((ViewGroup)mMain).removeView(webView);
			}
		}
	}

	private String webviewGetURL(int webviewId) {
		Log.i("Kinoma", String.format("webviewGetURL(%d)", webviewId));
		WebView webView = fetchWebView(webviewId);
		if (webView != null) {
			return webView.getUrl();
		}
		return null;
	}

	private void webviewSetURL(int webviewId, String url) {
		Log.i("Kinoma", String.format("webviewSetURL(%d, %s)", webviewId, url));
		WebView webView = fetchWebView(webviewId);
		if (webView != null) {
			webView.loadUrl(url);
			Log.i("Kinoma", "loadUrl:done");
		}
	}

	private int webviewEvaluateScript(int webviewId, String script) {
		WebView webView = fetchWebView(webviewId);
		WebViewJavaScriptInterface jsInterface = (WebViewJavaScriptInterface)mWebViewJSInterfaceMap.get(webviewId);
		if (webView != null && jsInterface != null) {
			jsInterface.setScript(script);

			String url = "javascript:" + "_kinoma.setResult(eval(_kinoma.script()))";
			webView.loadUrl(url);

			Log.i("Kinoma", String.format("webviewEvaluateScript(%d, %s)", webviewId, script));
			return 1;
		}

		return 0;
	}

	private void webviewReload(int webviewId) {
		Log.i("Kinoma", String.format("webviewReload(%d)", webviewId));
		WebView webView = fetchWebView(webviewId);
		if (webView != null) {
			webView.reload();
		}
	}

	private void webviewBack(int webviewId) {
		Log.i("Kinoma", String.format("webviewBack(%d)", webviewId));
		WebView webView = fetchWebView(webviewId);
		if (webView != null) {
			webView.goBack();
		}
	}

	private void webviewForward(int webviewId) {
		Log.i("Kinoma", String.format("webviewForward(%d)", webviewId));
		WebView webView = fetchWebView(webviewId);
		if (webView != null) {
			webView.goForward();
		}
	}

	private int webviewCanBack(int webviewId) {
		Log.i("Kinoma", String.format("webviewCanBack(%d)", webviewId));
		WebView webView = fetchWebView(webviewId);
		if (webView != null) {
			return webView.canGoBack() ? 1 : 0;
		}
		return 0;
	}

	private int webviewCanForward(int webviewId) {
		Log.i("Kinoma", String.format("webviewCanForward(%d)", webviewId));
		WebView webView = fetchWebView(webviewId);
		if (webView != null) {
			return webView.canGoForward() ? 1 : 0;
		}
		return 0;
	}

	protected WebView fetchWebView(int webviewId) {
		return (WebView)mWebViewMap.get(webviewId);
	}

	protected Integer fetchWebViewId(WebView webview) {
		return (Integer)mWebViewReverseMap.get(webview);
	}

	protected boolean isWebviewTouchEvent(MotionEvent event) {
		final int yOffset = mIsFullscreen ? 0 : mVerticalOffset;

		int x = (int) event.getX();
		int y = (int) event.getY() - yOffset;

		for (Map.Entry<Integer, WebView> entry : mWebViewMap.entrySet()) {
			WebView webView = (WebView)entry.getValue();

			int left = webView.getLeft();
			int top = webView.getTop();
			int right = webView.getRight();
			int bottom = webView.getBottom();

			if (x >= left && x <= right && y >= top && y <= bottom) return true;
		}

		return false;
	}

    protected native void webviewHandleLoading(int webviewId);
    protected native void webviewHandleLoaded(int webviewId);
    protected native boolean webviewShouldHandleUrl(int webviewId, String url);
    protected native void webviewHandleEvaluationResult(String result);

	/*******************************************************/
	Cursor mCallLogCursor;
	String	mCallLogString = null;
	int mCallLogCursorIDColumn;
	int mCallLogCursorNumberColumn;
	int mCallLogCursorDateColumn;
	int mCallLogCursorTypeColumn;
	int mCallLogCursorDurationColumn;
	int mCallLogCursorNameColumn;
	int mCallLogCursorNumberTypeColumn;

	public int doStartCallLog() {
		mCallLogCursor = managedQuery(
                android.provider.CallLog.Calls.CONTENT_URI,
                null, null, null,
                android.provider.CallLog.Calls.DEFAULT_SORT_ORDER);
//                android.provider.CallLog.Calls.DATE + " DESC");
		mCallLogCursorIDColumn = mCallLogCursor.getColumnIndex(android.provider.CallLog.Calls._ID);
		mCallLogCursorNumberColumn = mCallLogCursor.getColumnIndex(android.provider.CallLog.Calls.NUMBER);
		mCallLogCursorDateColumn = mCallLogCursor.getColumnIndex(android.provider.CallLog.Calls.DATE);
		mCallLogCursorTypeColumn = mCallLogCursor.getColumnIndex(android.provider.CallLog.Calls.TYPE);
		mCallLogCursorDurationColumn = mCallLogCursor.getColumnIndex(android.provider.CallLog.Calls.DURATION);
		mCallLogCursorNameColumn = mCallLogCursor.getColumnIndex(android.provider.CallLog.Calls.CACHED_NAME);
		mCallLogCursorNumberTypeColumn = mCallLogCursor.getColumnIndex(android.provider.CallLog.Calls.CACHED_NUMBER_TYPE);

		mCallLogString = "";
		mCallLogCursor.moveToFirst();

		if (0 < mCallLogCursor.getCount()) {
			do {
				String ID = mCallLogCursor.getString(mCallLogCursorIDColumn);
				String callerNumber = mCallLogCursor.getString(mCallLogCursorNumberColumn);
				long callDate = mCallLogCursor.getLong(mCallLogCursorDateColumn) / 1000;
//	long cd = callDate * 1000;
				int callType = mCallLogCursor.getInt(mCallLogCursorTypeColumn);
				int duration = mCallLogCursor.getInt(mCallLogCursorDurationColumn);
				String callerName = mCallLogCursor.getString(mCallLogCursorNameColumn);
				int numberType = mCallLogCursor.getInt(mCallLogCursorNumberTypeColumn);

//				java.sql.Time tm = new java.sql.Time(cd);
//	Log.i("Kinoma", "time " + cd + " " + " converted time: " + tm.toString());

				if (callerNumber.compareTo("-1") == 0)
					callerNumber = "Restricted";

				if (null == callerName)
			        mCallLogString += ID + ", " + callerNumber + ", , " + callDate + ", "
		        	+ duration + ", " + callType + ", " + numberType + "\n";
				else
					mCallLogString += ID + ", " + callerNumber + ", " + callerName + ", " + callDate + ", "
		        	+ duration + ", " + callType + ", " + numberType + "\n";

			} while (mCallLogCursor.moveToNext());
		}

		if (null != mCallLogCursor) {
			mCallLogCursor.close();
			mCallLogCursor = null;
		}

		if (mCallLogString != "")
			return 1;

		return 0;

	}

//#ifdefined READ_CONTACTS

	// GetContacts
	//  kFskMailContactAddressTypeAny = 0, kFskMailContactAddressTypeEmail = 1, kFskMailContactAddressTypePhone = 2
	// SetContactPicture (imageBuffer)
	//	 People.setPhotoData()
	//  People.loadContactPhoto()
	// GetAccounts
	//  kFskMailTypeAny = 0, kFskMailAccountTypeEmail = 1, kFskMailAccountTypeSMS = 2, kFskMailAccountTypeMMS = 3
	// SendMessage
	//
	/*******************************************************/


	// Form an array specifying which columns to return.
	String[] projection = new String[] {
			People._ID,
			People.NAME,
			People.DISPLAY_NAME,
			People.NUMBER
	};

	// Get the base URI for the People table in the Contacts content provider.
	Uri contactsURI =  People.CONTENT_URI;
	Uri methodsURI = Contacts.ContactMethods.CONTENT_EMAIL_URI;

	String mContactSet = null;
	Cursor mContactCursor = null;
	int mContactNum;
	int mContactIdColumn;
	int mContactNameColumn;
	int mContactPhoneColumn;


	public int doStartContacts(int i) {
		if (i == 0)
			mContactCursor = managedQuery(contactsURI,
					projection, // Which columns to return
					null,       // Which rows to return (all rows)
					null,       // Selection arguments (none)
					// Put  the results in ascending order by name
					People.NAME + " ASC");
		else
			mContactCursor = managedQuery(contactsURI,
				projection, // Which columns to return
				null,       // Which rows to return (all rows)
				null,       // Selection arguments (none)
					// Put  the results in ascending order by name
				People.NAME + " DESC");

		mContactCursor.moveToFirst();
		mContactIdColumn = mContactCursor.getColumnIndex(People._ID);
		mContactNameColumn = mContactCursor.getColumnIndex(People.DISPLAY_NAME);
		mContactPhoneColumn = mContactCursor.getColumnIndex(People.NUMBER);


		mContactNum = mContactCursor.getCount();
		if (0 == mContactNum)
			return 0;

		String name;
		mContactNum = 0;
		do {
			name = mContactCursor.getString(mContactNameColumn);
			if (name == null)
				continue;
			mContactNum++;
		} while(mContactCursor.moveToNext());
		mContactCursor.moveToFirst();

		return mContactNum;
	}

	public void doStopContacts() {
		mContactCursor.close();
		mContactCursor = null;
	}

	public void doNextContact() {
		String name, phones, mail, id;

		if (0 == mContactNum)
			return;

		// get the next, stash it into mContactSet as
		// Name, HomePhone, BusinessPhone, MobilePhone, Email
		do {
			name = mContactCursor.getString(mContactNameColumn);
			if (name == null)
				continue;
			id = mContactCursor.getString(mContactIdColumn);
			//        phone = mContactCursor.getString(mContactPhoneColumn);

			mail = GetPreferredMail(id);
			phones = GetPhoneNumbers(id);
			mContactSet = name + ", " + phones + ", " + mail;

			Uri uri = ContentUris.withAppendedId(People.CONTENT_URI, mContactCursor.getLong(mContactIdColumn));
//			Bitmap b = Contacts.People.loadContactPhoto(this, uri, 0, null);
//			if (null != b) {
//				ByteArrayOutputStream outStream = new ByteArrayOutputStream(128);
//				b.compress(Bitmap.CompressFormat.JPEG, 100, outStream);
//				Log.i("compress contact image", " size = " + outStream.size());

//				mIconBuffer = outStream.toByteArray();
//			}
//			else
			{
				mIconBuffer = null;
			}
			break;
		} while (true);

		mContactCursor.moveToNext();	// prepare for next iteration
	}

	public void doIdxContact(int idx) {
		if (0 == mContactNum)
			return;

		// get the idx'th contact, stash it into mContactSet as
		//
		mContactCursor.moveToPosition(idx);
		doNextContact();
	}


	void doFetchContacts(int kind) {
		Cursor cur = mResolver.query( ContactsContract.Contacts.CONTENT_URI, null, null, null, null );
		mContactSet = "";
		if (cur.getCount() > 0) {
			while (cur.moveToNext()) {
				String id = cur.getString(cur.getColumnIndex(ContactsContract.Contacts._ID));
				String name = cur.getString(cur.getColumnIndex(ContactsContract.Contacts.DISPLAY_NAME));
				if (name == null)
					continue;

				if (kind == 1)	{		// kFskMailContactAddressTypeEmail
						Cursor emailCur = mResolver.query( ContactsContract.CommonDataKinds.Email.CONTENT_URI,
							null, ContactsContract.CommonDataKinds.Email.CONTACT_ID + " = ?", new String[]{id}, null);
						if (emailCur.getCount() > 0) {
							mContactSet += id + ", " + name;
							while (emailCur.moveToNext()) {
								String email = emailCur.getString(
										emailCur.getColumnIndex(ContactsContract.CommonDataKinds.Email.DATA));
								String emailType = emailCur.getString(
										emailCur.getColumnIndex(ContactsContract.CommonDataKinds.Email.TYPE));
								mContactSet += ", " +  email;
							}
							mContactSet += "\n";
						}
					 	emailCur.close();
				}
				else if (kind == 2)	{ // kFskMailContactAddressTypePhone
					if (Integer.parseInt(cur.getString(cur.getColumnIndex(ContactsContract.Contacts.HAS_PHONE_NUMBER))) > 0) {
						Cursor pCur = mResolver.query(ContactsContract.CommonDataKinds.Phone.CONTENT_URI,
							null, ContactsContract.CommonDataKinds.Phone.CONTACT_ID +" = ?", new String[]{id}, null);
						mContactSet += id + ", " + name;
						while (pCur.moveToNext()) {
							String phone = pCur.getString(pCur.getColumnIndex(ContactsContract.CommonDataKinds.Phone.NUMBER));
							mContactSet += ", " + phone;
						}
						pCur.close();
						mContactSet += "\n";
					}
				}
				else {
//					Log.i("Kinoma", "unknown contact type desired - " + kind);
				}
			}
		}


	}


	String[] EMAIL_PROJECTION = new String[] {
			Contacts.ContactMethods.PERSON_ID,
			Contacts.ContactMethods.DISPLAY_NAME,
			Contacts.ContactMethods.KIND,
			Contacts.ContactMethods.TYPE,
			//    		Contacts.ContactMethods.AUX_DATA,
			Contacts.ContactMethods.DATA
	};

	Uri contactMethodsURI = Contacts.ContactMethods.CONTENT_URI;

	public String GetPreferredMail(String personID)
	{
		String ret = "";

		try {
			Cursor cur = managedQuery(contactMethodsURI, EMAIL_PROJECTION,
					Contacts.ContactMethods.PERSON_ID + "=" + personID + " AND " +
					Contacts.ContactMethods.KIND + "=1",						// 1 == EMAIL
					null, Contacts.ContactMethods.ISPRIMARY + " DESC");			// primary email first
			if (cur.moveToFirst()) {
				int nRow = cur.getCount();//get rows of phones database
//				Log.d("Kinoma", "ContactMethodsCount" + String.valueOf(nRow));
				do {
					int nColumnData = cur.getColumnIndex(Contacts.ContactMethods.DATA);
					String strData = cur.getString(nColumnData);
//					if (strData != null)
//						Log.d("data", strData);
					int nColumnType = cur.getColumnIndex(Contacts.ContactMethods.TYPE);
					String strType = cur.getString(nColumnType);
//					if (strType != null)
//						Log.d("type", strType);

					ret += strData.toString();	// .getBytes("UTF-8");
					break;
				} while (cur.moveToNext());
			}
			cur.close();
		} catch (Exception e) {
			Log.i("Kinoma", "ParseContactMethods Exception" + e.toString());
			e.printStackTrace();
		}
		return ret;
	}

	String[] PHONES_PROJECTION = new String[] {
			Contacts.Phones.PERSON_ID,
			Contacts.Phones.NUMBER,
			Contacts.Phones.NUMBER_KEY,
			Contacts.Phones.TYPE
	};

	Uri contactPhonesURI = Contacts.Phones.CONTENT_URI;

	public String GetPhoneNumbers(String personID)
	{
		String ret = "";
		try {
			String homePhone = "", workPhone = "", mobPhone = "";

			Cursor cur = managedQuery(contactPhonesURI, PHONES_PROJECTION,
					Contacts.Phones.PERSON_ID + "=" + personID, null, Contacts.Phones.ISPRIMARY + " DESC");
			if (cur.moveToFirst()) {
				int nRow = cur.getCount();//get rows of phones database
//				Log.d("Kinoma", "ContactPhonesCount" + String.valueOf(nRow));

				do {
					int nColumnType = cur.getColumnIndex(Contacts.Phones.TYPE);
					String strType = cur.getString(nColumnType);
//					if (strType != null)
//						Log.d("Kinoma", "type" + strType);
					int nColumnPhoneNumber = cur.getColumnIndex(Contacts.Phones.NUMBER);
					String strPhoneNumber = cur.getString(nColumnPhoneNumber);
//					if (strPhoneNumber != null)
//						Log.d("Kinoma", "Phone Number" + strPhoneNumber);
					if( strType.equals(String.valueOf(Contacts.Phones.TYPE_HOME)))
						homePhone = strPhoneNumber;
					else if (strType.equals(String.valueOf(Contacts.Phones.TYPE_WORK)))
						workPhone = strPhoneNumber;
					else if (strType.equals(String.valueOf(Contacts.Phones.TYPE_MOBILE)))
						mobPhone = strPhoneNumber;
				} while (cur.moveToNext());
			}
			cur.close();
			ret = homePhone + ", " + workPhone + ", " + mobPhone;
		} catch (Exception e) {
			Log.i("Kinoma", "ParseContactMethods Exception" + e.toString());
			e.printStackTrace();
		}
		return ret;
	}
//#endif


	/*******************************************************/
	protected void doSendAttach(String emailTo, String emailSubject, String emailBody, String filePaths[], int numFiles) {
		Intent sendIntent = null;

//		Log.i("Kinoma", "doSendAttach: " + emailTo + " - sub: " + emailSubject + "bod: " + emailBody + " numFiles: " + numFiles);

		// Create a new Intent to send messages
		if (numFiles == 1)
			sendIntent = new Intent(Intent.ACTION_SEND);
		else
			sendIntent = new Intent(Intent.ACTION_SEND_MULTIPLE);
		// Add attributes to the intent

		if (-1 == emailTo.indexOf(',')) {
			String[] addrs = { emailTo };
			sendIntent.putExtra(Intent.EXTRA_EMAIL, addrs);
		}
		else {
			String [] addrs = null;
			addrs = emailTo.split(",");
			sendIntent.putExtra(Intent.EXTRA_EMAIL, addrs);
		}

		sendIntent.putExtra(Intent.EXTRA_SUBJECT, emailSubject);
		sendIntent.putExtra(Intent.EXTRA_TEXT, emailBody);
//		sendIntent.setType("*/*");
		sendIntent.setType("message/rfc822");
//		sendIntent.setType("image/*");

		if (numFiles == 1) {
			sendIntent.putExtra(Intent.EXTRA_STREAM, Uri.parse("file://" + filePaths[0]));
		}
		else {
			ArrayList<Uri> uris = new ArrayList<Uri>();
			for (int i = 0; i< numFiles; i++) {
				File fileIn = new File(filePaths[i]);
				Uri u = Uri.fromFile(fileIn);
				uris.add(u);
			}
			sendIntent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, uris);
		}
		startActivity(Intent.createChooser(sendIntent, "Send mail (with attachment)..."));
	}

	/*******************************************************/
	protected void doSendMail(String emailTo, String emailSubject, String emailBody) {
//		Log.i("Kinoma", "doSendMail: " + emailTo + " - sub: " + emailSubject + "bod: " + emailBody);

	// Setup the recipient in a String array
		String[] mailto = { emailTo };
		// Create a new Intent to send messages
		Intent sendIntent = new Intent(Intent.ACTION_SEND);
		// Add attributes to the intent
		if (null != emailTo && emailTo.length() != 0)
			sendIntent.putExtra(Intent.EXTRA_EMAIL, mailto);
//			sendIntent.putExtra(Intent.EXTRA_EMAIL, emailTo);
		if (null != emailSubject && emailSubject.length() != 0)
			sendIntent.putExtra(Intent.EXTRA_SUBJECT, emailSubject);
		if (null != emailBody && emailBody.length() != 0)
			sendIntent.putExtra(Intent.EXTRA_TEXT, emailBody);
		sendIntent.setType("*/*");
		startActivity(Intent.createChooser(sendIntent, "Send mail..."));

	}

//#ifdefined SEND_SMS
	/*******************************************************/
	protected void doSendSMS(String number, String message)
	{
		SmsManager mng = SmsManager.getDefault();

		try{
			mng.sendTextMessage(number, null, message, null, null);
		}catch(Exception e){
			Log.e("Kinoma", "SmsIntent SendException", e );
		}
	}
//#endif

//#ifdefined CALL_PHONE
	protected void doDial(String thePhoneNumber) {
//		Log.i("Kinoma", "doDial: " + thePhoneNumber);

		if (thePhoneNumber.length() == 0) {
//#ifdefined ACCESS_NETWORK_STATE
			thePhoneNumber = mTelephonyManager.getVoiceMailNumber();
//			Log.i("Kinoma", "doDial: (wasVoiceMail) now: " + thePhoneNumber);
//#endif
		}

		try {
			startActivity(new Intent(Intent.ACTION_CALL, Uri.parse("tel:" + thePhoneNumber)));
		}
		catch (Exception e) {
			e.printStackTrace();
		}
	}
//#endif




/*******************************************************/
private static void initializeRemoteControlRegistrationMethods() {
   try {
      if (mRegisterMediaButtonEventReceiver == null) {
         mRegisterMediaButtonEventReceiver = AudioManager.class.getMethod(
               "registerMediaButtonEventReceiver",
               new Class[] { ComponentName.class } );
      }
      if (mUnregisterMediaButtonEventReceiver == null) {
         mUnregisterMediaButtonEventReceiver = AudioManager.class.getMethod(
               "unregisterMediaButtonEventReceiver",
               new Class[] { ComponentName.class } );
      }
      /* success, this device will take advantage of better remote */
      /* control event handling                                    */
   } catch (NoSuchMethodException nsme) {
      /* failure, still using the legacy behavior, but this app    */
      /* is future-proof!                                          */
   }
}

private void registerRemoteControl() {
	startPlayback();
    try {
        if (mRegisterMediaButtonEventReceiver == null) {
            return;
        }
        mRegisterMediaButtonEventReceiver.invoke(mAudioManager, mRemoteControlResponder);
    } catch (InvocationTargetException ite) {
        /* unpack original exception when possible */
        Throwable cause = ite.getCause();
        if (cause instanceof RuntimeException) {
            throw (RuntimeException) cause;
        } else if (cause instanceof Error) {
            throw (Error) cause;
        } else {
            /* unexpected checked exception; wrap and re-throw */
            throw new RuntimeException(ite);
        }
    } catch (IllegalAccessException ie) {
        System.err.println("unexpected " + ie);
    }
}

private void unregisterRemoteControl() {
	stopPlayback();
    try {
        if (mUnregisterMediaButtonEventReceiver == null) {
            return;
        }
        mUnregisterMediaButtonEventReceiver.invoke(mAudioManager,
                mRemoteControlResponder);
    } catch (InvocationTargetException ite) {
        /* unpack original exception when possible */
        Throwable cause = ite.getCause();
        if (cause instanceof RuntimeException) {
            throw (RuntimeException) cause;
        } else if (cause instanceof Error) {
            throw (Error) cause;
        } else {
            /* unexpected checked exception; wrap and re-throw */
            throw new RuntimeException(ite);
        }
    } catch (IllegalAccessException ie) {
        System.err.println("unexpected " + ie);
    }
}

//#ifdefined C2D_MESSAGE
	private SharedPreferences getPreferences() {
		return getSharedPreferences(kConfigName, Context.MODE_PRIVATE);
	}

	private void notifyIfSaved() {
		SharedPreferences prefs = getPreferences();
		String message = prefs.getString(kRemoteNotificationMessageKey, null);
		if (message != null) {
			Log.i("kinoma", "kRemoteNotificationMessage=" + message);
			doFskOnRemoteNotification(message);
			Editor editor = prefs.edit();
			editor.remove(kRemoteNotificationMessageKey);
			editor.commit();
		}
	}

	private void registerRemoteNotification() {
		Log.i("kinoma", "registerRemoteNotification");

		if ( !gInitialized || mIsEmulator || (getRemoteNotificationType() == 0) || (mBuildRemoteNotificationID == null) ) {
			Log.i("kinoma", "no need to register");
			return;
		}

		if (mRemoteNotificationRegistered) {
			notifyIfSaved();
			return;
		}

		int resultCode = GooglePlayServicesUtil.isGooglePlayServicesAvailable(this);

		if (resultCode != ConnectionResult.SUCCESS) {
			Log.i("kinoma", "GCM is not available");
			if (GooglePlayServicesUtil.isUserRecoverableError(resultCode)) {
				GooglePlayServicesUtil.getErrorDialog(resultCode, this,
													  PLAY_SERVICES_RESOLUTION_REQUEST).show();
			} else {
				doFskOnRemoteNotificationRegistered(null);
				setRemoteNotificationType(0);
				mRemoteNotificationRegistered = true;
			}
			return;
		}

		Log.i("kinoma", "start registering");
		final Context context = this.getApplicationContext();
		new AsyncTask<Void, Void, Void>() {
			@Override
				protected Void doInBackground(Void... params) {
				try {
					GoogleCloudMessaging gcm = GoogleCloudMessaging.getInstance(context);
					if (gcm != null) {
						String regid = gcm.register(mBuildRemoteNotificationID);
						Log.i("kinoma", "registerRemoteNotification ID=" + regid);
						doFskOnRemoteNotificationRegistered(regid);
						mRemoteNotificationRegistered = true;

						notifyIfSaved();
					}
				} catch (IOException e) {
					e.printStackTrace();
				}
				return null;
			}
		}.execute(null, null, null);
    }
//#endif

public class NoisyAudioStreamReceiver extends BroadcastReceiver {

	public void onReceive(Context context, Intent intent) {
    	String intentAction = intent.getAction();

    	if (AudioManager.ACTION_AUDIO_BECOMING_NOISY.equals(intentAction)) {
            // Pause the playback
 //       	Log.i("Kinoma", "NoisyAudioStreamReceiver");

//        	if (mAudioManager.isBluetoothA2dpOn()) {
        	    // Adjust output for Bluetooth.
//       		Log.i("kinoma", "Bluetooth is on");
//        	} else if (mAudioManager.isSpeakerphoneOn()) {
        	    // Adjust output for Speakerphone.
//        		Log.i("kinoma", "speakerphone is on");
//        	} else if (mAudioManager.isWiredHeadsetOn()) {
        	    // Adjust output for headsets
//        		Log.i("kinoma", "wired headset on");
//        	} else {
        	    // If audio plays and noone can hear it, is it still playing?
//        	}

        	// send Fsk's Pause key
        	com.kinoma.kinomaplay.KinomaPlay.doFskKeyEvent(KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE, 0, KeyEvent.ACTION_UP, 0, 0);

        	return;
    	}
    	abortBroadcast();
	}
}

private void startPlayback() {
    registerReceiver(mNoisyAudioStreamResponder, mNoisyAudioIntent);
}

private void stopPlayback() {
    unregisterReceiver(mNoisyAudioStreamResponder);
}

	/* Media Library */

	// ----------------------------------------------
	// MEDIA -abstract-

	static final String MediaFieldSeparator = "\t";
	static final String MediaRecordSeparator = "\n";

	public abstract class Media {
		public long id;
		public String initial;
		public String sort;

		@Override
		public String toString() {
			return "" + id;
		}

		abstract public String[] fieldValues();

		public String fieldValuesString() {
			String[] fields = fieldValues();
			if (fields.length == 0) return "";

			StringBuffer buffer = new StringBuffer(256);

			for (int i = 0; i < fields.length - 1; i++) {
				buffer.append(escapedFieldValue(fields[i]));
				buffer.append(MediaFieldSeparator);
			}

			buffer.append(escapedFieldValue(fields[fields.length - 1]));

			return buffer.toString();
		}

		public String escapedFieldValue(String value) {
			if (value == null) return "";
			return value.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n");
		}
	}

	// ----------------------------------------------
	// ALBUM

	public class Album extends Media {
		public String album;
		public String artist;
		public int numberOfSongs;

		@Override
		public String toString() {
			return album + "<" + artist + ">";
		}

		public String[] fieldValues() {
			return new String[] {
				"" + id,
				album,
				artist,
				"" + numberOfSongs,
				initial
			};
		}
	}

	static final String[] albumProjection = new String[]{
		MediaStore.Audio.Albums._ID,
		MediaStore.Audio.Albums.ALBUM,
		MediaStore.Audio.Albums.ARTIST,
		MediaStore.Audio.Albums.NUMBER_OF_SONGS
	};

	static final String[] albumProjectionForArtist = new String[]{
		MediaStore.Audio.Albums._ID,
		MediaStore.Audio.Albums.ALBUM,
		MediaStore.Audio.Albums.ARTIST,
		MediaStore.Audio.Albums.NUMBER_OF_SONGS_FOR_ARTIST
	};

	public ArrayList<Media> albumCursorToList(Cursor cursor, boolean forArtist) {
		final ArrayList<Media> list = new ArrayList<Media>();

		if (cursor.moveToFirst()) {
			int idIndex = cursor.getColumnIndex(MediaStore.Audio.Albums._ID);
			int albumIndex = cursor.getColumnIndex(MediaStore.Audio.Albums.ALBUM);
			int artistIndex = cursor.getColumnIndex(MediaStore.Audio.Albums.ARTIST);
			int numIndex = cursor.getColumnIndex(forArtist ? MediaStore.Audio.Albums.NUMBER_OF_SONGS_FOR_ARTIST : MediaStore.Audio.Albums.NUMBER_OF_SONGS);
			do {
				Album album = new Album();
				album.id = cursor.getLong(idIndex);
				album.album = cursor.getString(albumIndex);
				album.artist = cursor.getString(artistIndex);
				album.numberOfSongs = cursor.getInt(numIndex);

				album.sort = album.album;

				list.add(album);
			} while(cursor.moveToNext());
		}

		cursor.close();
		return list;
	}

	public String getAlbumArtwork(long albumId) {
		Cursor cursor = getContentResolver().query(
				MediaStore.Audio.Albums.EXTERNAL_CONTENT_URI,
				new String[] { MediaStore.Audio.Albums.ALBUM_ART },
				MediaStore.Audio.Albums._ID + "=?",
				new String[] {String.valueOf(albumId)},
				null);

		String path = null;
		if (cursor.moveToFirst()) {
			int index = cursor.getColumnIndex(MediaStore.Audio.Albums.ALBUM_ART);
			path = cursor.getString(index);
		}
		cursor.close();

		return path;
	}

	public Bitmap getAlbumThumnail(long albumId) {
		String path = getAlbumArtwork(albumId);
		return BitmapFactory.decodeFile(path);
	}

	// ----------------------------------------------
	// ARTIST

	public class Artist extends Media {
		public String artist;
		public int numberOfAlbums;
		public int numberOfSongs;
		public String album;
		public long album_id;

		@Override
		public String toString() {
			return artist;
		}

		public String[] fieldValues() {
			return new String[] {
				"" + id,
				artist,
				album,
				"" + numberOfAlbums,
				"" + numberOfSongs,
				initial,
				"" + album_id
			};
		}
	}
	static final String[] artistProjection = new String[]{
		MediaStore.Audio.Artists._ID,
		MediaStore.Audio.Artists.ARTIST,
		MediaStore.Audio.Artists.NUMBER_OF_ALBUMS
	};

	public ArrayList<Media> artistCursorToList(Cursor cursor) {
		final ArrayList<Media> list = new ArrayList<Media>();

		if (cursor.moveToFirst()) {
			int idIndex = cursor.getColumnIndex(MediaStore.Audio.Artists._ID);
			int artistIndex = cursor.getColumnIndex(MediaStore.Audio.Artists.ARTIST);
			int numIndex = cursor.getColumnIndex(MediaStore.Audio.Artists.NUMBER_OF_ALBUMS);
			do {
				Artist artist= new Artist();
				artist.id = cursor.getLong(idIndex);
				artist.artist = cursor.getString(artistIndex);
				artist.numberOfAlbums = cursor.getInt(numIndex);
				artist.numberOfSongs = 0;
				artist.album_id = 0;

				artist.sort = artist.artist;

				for (Media media : getArtistAlbums(artist.id)) {
					Album album = (Album)media;
					if (artist.album == null && album.album != null) {
						artist.album = album.album;
					}

					if (artist.album_id == 0) {
						artist.album_id = album.id;
					}

					artist.numberOfSongs += album.numberOfSongs;
				}

				list.add(artist);
			} while(cursor.moveToNext());
		}

		cursor.close();
		return list;
	}

	// ----------------------------------------------
	// GENRE

	public class Genre extends Media {
		public String name;
		public int numberOfSongs;
		public long album_id;

		@Override
		public String toString() {
			return id + ": " + name + " (" + numberOfSongs + " songs)";
		}

		public String[] fieldValues() {
			return new String[] {
				"" + id,
				name,
				"" + numberOfSongs,
				initial,
				"" + album_id
			};
		}
	}

	static final String[] genreProjection = new String[]{
		MediaStore.Audio.Genres._ID,
		MediaStore.Audio.Genres.NAME
	};

	public ArrayList<Media> genreCursorToList(Cursor cursor) {
		final ArrayList<Media> list = new ArrayList<Media>();

		if (cursor.moveToFirst()) {
			int idIndex = cursor.getColumnIndex(MediaStore.Audio.Genres._ID);
			int nameIndex = cursor.getColumnIndex(MediaStore.Audio.Genres.NAME);
			do {
				Genre genre = new Genre();
				genre.id = cursor.getLong(idIndex);
				genre.name = cursor.getString(nameIndex);

				genre.sort = genre.name;

				genre.numberOfSongs = getGenreSongCount(genre.id);

				Song song = getGenreSong(genre.id);
				if (song != null) {
					genre.album_id = song.album_id;
				}

				if (genre.numberOfSongs > 0) {
					list.add(genre);
				}
			} while(cursor.moveToNext());
		}

		cursor.close();
		return list;
	}

	public Cursor allGenresCursor() {
		return getContentResolver().query(
			MediaStore.Audio.Genres.EXTERNAL_CONTENT_URI,
			genreProjection,
			MediaStore.Audio.Genres.NAME + " != ''",
			null,
			MediaStore.Audio.Genres.NAME);
	}

	public int getGenreSongCount(long genreId) {
		Cursor cursor = getContentResolver().query(
			genreContentUriWithGenreId(genreId),
			new String[] { MediaStore.Audio.Genres._ID },
			MediaStore.Audio.Media.IS_MUSIC + " != 0",
			null, null);
		int count = cursor.getCount();
		cursor.close();
		return count;
	}

	public long getGenreId(String genre) {
		Cursor cursor = getContentResolver().query(
			MediaStore.Audio.Genres.EXTERNAL_CONTENT_URI,
			new String[] { MediaStore.Audio.Genres._ID },
			MediaStore.Audio.Genres.NAME + " = ?",
			new String[] { genre },
			null);
		if (!cursor.moveToFirst()) return 0;

		long genreId = cursor.getLong(cursor.getColumnIndex(MediaStore.Audio.Genres._ID));
		cursor.close();
		return genreId;
	}

	public Uri genreContentUriWithGenreId(long genreId) {
		return MediaStore.Audio.Genres.Members.getContentUri("external", genreId);
	}

	// ----------------------------------------------
	// SONG

	// { "INSERT INTO audio ( mime, duration, artist_id, album_id, track, genre_id, id ) VALUES ( ?, ?, ?, ?, ?, ?, ? )", "tdrrirr", NULL, NULL},
	// { "SELECT id, date, mime, size, path, title, duration, time, artist_name, album_name, track, genre_name FROM audio_view ORDER by title", NULL, "rdtIttddttit", NULL},
	public class Song extends Media {
		public double date;
		public long size;
		public String mime;
		public String title;
		public double duration;
		public double time;
		public String artist;
		public String album;
		public int track;
		public String path;
		public long album_id;

		@Override
		public String toString() {
			return title + " <" + album + "> by " + artist;
		}

		public String[] fieldValues() {
			return new String[] {
				"" + id,
				"" + date,
				"" + size,
				mime,
				title,
				"" + duration,
				"" + time,
				artist,
				album,
				"" + track,
				path,
				initial,
				"" + album_id
			};
		}
	}

	static final String[] songProjection = new String[]{
		MediaStore.Audio.Media._ID,
		MediaStore.Audio.Media.DATE_ADDED,
		MediaStore.Audio.Media.SIZE,
		MediaStore.Audio.Media.MIME_TYPE,
		MediaStore.Audio.Media.TITLE,
		MediaStore.Audio.Media.DURATION,

		MediaStore.Audio.Media.ARTIST,
		MediaStore.Audio.Media.ALBUM,
		MediaStore.Audio.Media.ALBUM_ID,
		MediaStore.Audio.Media.TRACK,
		MediaStore.Audio.Media.DATA
	};

	public ArrayList<Media> songCursorToList(Cursor cursor) {
		final ArrayList<Media> list = new ArrayList<Media>();

		if (cursor.moveToFirst()) {
			int idIndex = cursor.getColumnIndex(MediaStore.Audio.Media._ID);
			int dateAddedIndex = cursor.getColumnIndex(MediaStore.Audio.Media.DATE_ADDED);
			int sizeIndex = cursor.getColumnIndex(MediaStore.Audio.Media.SIZE);
			int mimeTypeIndex = cursor.getColumnIndex(MediaStore.Audio.Media.MIME_TYPE);
			int titleIndex = cursor.getColumnIndex(MediaStore.Audio.Media.TITLE);
			int durationIndex = cursor.getColumnIndex(MediaStore.Audio.Media.DURATION);
			int artistIndex = cursor.getColumnIndex(MediaStore.Audio.Media.ARTIST);
			int albumIndex = cursor.getColumnIndex(MediaStore.Audio.Media.ALBUM);
			int albumIdIndex = cursor.getColumnIndex(MediaStore.Audio.Media.ALBUM_ID);
			int trackIndex = cursor.getColumnIndex(MediaStore.Audio.Media.TRACK);
			int dataIndex = cursor.getColumnIndex(MediaStore.Audio.Media.DATA);
			do {
				Song music = new Song();
				music.id = cursor.getLong(idIndex);
				music.date = cursor.getDouble(dateAddedIndex);
				music.size = cursor.getLong(sizeIndex);
				music.mime = cursor.getString(mimeTypeIndex);
				music.title = cursor.getString(titleIndex);
				music.duration = cursor.getDouble(durationIndex);
				music.time = 0;
				music.artist = cursor.getString(artistIndex);
				music.album = cursor.getString(albumIndex);
				music.track = cursor.getInt(trackIndex);
				music.path = cursor.getString(dataIndex);
				music.album_id = cursor.getLong(albumIdIndex);

				music.sort = music.title;

				if (music.track > 1000) {
					music.track = music.track % 1000;
				}

				list.add(music);
			} while(cursor.moveToNext());
		}

		cursor.close();
		return list;
	}

	public ArrayList<Media> getSongs() {
		Cursor cursor = getContentResolver().query(
				MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
				songProjection,
				MediaStore.Audio.Media.IS_MUSIC + " != 0",
				null,
				MediaStore.Audio.Media.TITLE);

		return songCursorToList(cursor);
	}

	public ArrayList<Media> getGenreSongs(String genre) {
		Uri uri = genreContentUriWithGenreId(getGenreId(genre));

		Cursor cursor = getContentResolver().query(
				uri,
				songProjection,
				MediaStore.Audio.Media.IS_MUSIC + " != 0",
				null,
				MediaStore.Audio.Media.TITLE);

		return songCursorToList(cursor);
	}

	public Song getGenreSong(long genre_id) {
		Uri uri = genreContentUriWithGenreId(genre_id);

		Cursor cursor = getContentResolver().query(
				uri,
				songProjection,
				MediaStore.Audio.Media.IS_MUSIC + " != 0",
				null,
				MediaStore.Audio.Media.TITLE + " limit 1");

		return getFirstSong(songCursorToList(cursor));
	}

	public ArrayList<Media> getAlbumSongs(String album) {
		Cursor cursor = getContentResolver().query(
				MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
				songProjection,
				MediaStore.Audio.Media.IS_MUSIC + " != 0 AND " + MediaStore.Audio.Media.ALBUM + " = ?",
				new String[] {album},
				MediaStore.Audio.Media.TRACK);

		return songCursorToList(cursor);
	}

	public ArrayList<Media> getArtistSongs(String artist) {
		Cursor cursor = getContentResolver().query(
				MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
				songProjection,
				MediaStore.Audio.Media.IS_MUSIC + " != 0 AND " + MediaStore.Audio.Media.ARTIST + " = ?",
				new String[] {artist},
				MediaStore.Audio.Media.TITLE);

		return songCursorToList(cursor);
	}

	public Song getArtistSong(long artist_id) {
		Cursor cursor = getContentResolver().query(
				MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
				songProjection,
				MediaStore.Audio.Media.IS_MUSIC + " != 0 AND " + MediaStore.Audio.Media.ARTIST_ID + " = ?",
				new String[] {"" + artist_id},
				MediaStore.Audio.Media.TITLE + " limit 1");

		return getFirstSong(songCursorToList(cursor));
	}

	public String getAlbumSongContentPath(int album_id, int index) {
		Cursor cursor = getContentResolver().query(
				MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
				new String[] { MediaStore.Audio.Media._ID },
				MediaStore.Audio.Media.IS_MUSIC + " != 0 AND " + MediaStore.Audio.Media.ALBUM_ID + " = ?",
				new String[] {"" + album_id},
				MediaStore.Audio.Media.TRACK + " limit " + index + ",1");

		long song_id = 0;
		if (cursor.moveToFirst()) {
			song_id = cursor.getLong(cursor.getColumnIndex(MediaStore.Audio.Media._ID));
		}
		cursor.close();
		if (song_id == 0) return null;

		cursor = getContentResolver().query(
				MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
				new String[] { MediaStore.Audio.Media.DATA },
				MediaStore.Audio.Media._ID + " = ?",
				new String[] { "" + song_id },
				null);

		String path = null;
		if (cursor.moveToFirst()) {
			path = cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.DATA));
		}
		cursor.close();

		return path;
	}

	private Song getFirstSong(ArrayList<Media> songs) {
		if (songs == null || songs.isEmpty()) return null;
		return (Song)songs.get(0);
	}

	// ----------------------------------------------
	// IMAGE

	public class Image extends Media {
		public double date;
		public String mime;
		public long size;
		public String title;
		public int width;
		public int height;
		public int rotation;
		public double taken;
		public String path;

		public String[] fieldValues() {
			return new String[] {
				"" + id,
				"" + date,
				mime,
				"" + size,
				title,
				"" + width,
				"" + height,
				"" + rotation,
				"" + taken,
				path
			};
		}
	}

	static final String[] imageProjection = new String[]{
		MediaStore.Images.Media._ID,
		MediaStore.Images.Media.DATE_ADDED,
		MediaStore.Images.Media.MIME_TYPE,
		MediaStore.Images.Media.SIZE,
		MediaStore.Images.Media.TITLE,
		MediaStore.Images.Media.ORIENTATION,
		MediaStore.Images.Media.DATE_TAKEN,
		MediaStore.Images.Media.DATA
	};

	static final String[] imageProjectionWithProjection = new String[]{
		MediaStore.Images.Media._ID,
		MediaStore.Images.Media.DATE_ADDED,
		MediaStore.Images.Media.MIME_TYPE,
		MediaStore.Images.Media.SIZE,
		MediaStore.Images.Media.TITLE,
		MediaStore.Images.Media.WIDTH,
		MediaStore.Images.Media.HEIGHT,
		MediaStore.Images.Media.ORIENTATION,
		MediaStore.Images.Media.DATE_TAKEN,
		MediaStore.Images.Media.DATA
	};

	public ArrayList<Media> imageCursorToList(Cursor cursor, boolean withDimention) {
		final ArrayList<Media> list = new ArrayList<Media>();

		if (cursor.moveToFirst()) {
			int idIndex = cursor.getColumnIndex(MediaStore.Images.Media._ID);
			int dateAddedIndex = cursor.getColumnIndex(MediaStore.Images.Media.DATE_ADDED);
			int mimeTypeIndex = cursor.getColumnIndex(MediaStore.Images.Media.MIME_TYPE);
			int sizeIndex = cursor.getColumnIndex(MediaStore.Images.Media.SIZE);
			int titleIndex = cursor.getColumnIndex(MediaStore.Images.Media.TITLE);
			int orientationIndex = cursor.getColumnIndex(MediaStore.Images.Media.ORIENTATION);
			int dateTakenIndex = cursor.getColumnIndex(MediaStore.Images.Media.DATE_TAKEN);
			int dataIndex = cursor.getColumnIndex(MediaStore.Images.Media.DATA);
			int widthIndex, heightIndex;

			if (withDimention) {
				widthIndex = cursor.getColumnIndex(MediaStore.Images.Media.WIDTH);
				heightIndex = cursor.getColumnIndex(MediaStore.Images.Media.HEIGHT);
			} else {
				widthIndex = heightIndex = -1;
			}

			do {
				Image image = new Image();
				image.id = cursor.getLong(idIndex);
				image.date = cursor.getDouble(dateAddedIndex);
				image.mime = cursor.getString(mimeTypeIndex);
				image.size = cursor.getLong(sizeIndex);
				image.title = cursor.getString(titleIndex);
				image.rotation = cursor.getInt(orientationIndex);
				image.taken = cursor.getDouble(dateTakenIndex);
				image.path = cursor.getString(dataIndex);

				if (withDimention) {
					image.width = cursor.getInt(widthIndex);
					image.height = cursor.getInt(heightIndex);
				}

				list.add(image);
			} while(cursor.moveToNext());
		}

		cursor.close();
		return list;
	}

	public Bitmap getImageThumnail(long id, boolean micro) {
		return MediaStore.Images.Thumbnails.getThumbnail(
			getContentResolver(),
			id,
			(micro
				? MediaStore.Images.Thumbnails.MICRO_KIND
				: MediaStore.Images.Thumbnails.MINI_KIND),
			null);
	}

	public void saveImage(byte[] data) {
		ContentResolver cr = getContentResolver();
		ContentValues values = new ContentValues();;
		values.put(Images.Media.MIME_TYPE, "image/jpeg");

		// The default android.provider.MediaStore.Images.Media#insertImage() save image at the end of the gallery
		// Add the date meta data to ensure the image is added at the front of the gallery
		values.put(Images.Media.DATE_ADDED, System.currentTimeMillis());
		values.put(Images.Media.DATE_TAKEN, System.currentTimeMillis());

		try {
			Uri url = cr.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values);
			OutputStream imageOut = cr.openOutputStream(url);
			imageOut.write(data);
			imageOut.close();
		} catch (Exception error) {
			Log.d("KinomaPlay", "Image not saved: " + error.getMessage());
		}
	}

	// ----------------------------------------------
	// VIDEO

	public class Video extends Media {
		public double date;
		public String mime;
		public long size;
		public String title;
		public int width;
		public int height;
		public double duration;
		public double time;
		public String path;

		public String[] fieldValues() {
			return new String[] {
				"" + id,
				"" + date,
				mime,
				"" + size,
				title,
				"" + width,
				"" + height,
				"" + duration,
				"" + time,
				path
			};
		}
	}

	static final String[] videoProjection = new String[]{
		MediaStore.Video.Media._ID,
		MediaStore.Video.Media.DATE_ADDED,
		MediaStore.Video.Media.MIME_TYPE,
		MediaStore.Video.Media.SIZE,
		MediaStore.Video.Media.TITLE,
		// MediaStore.Video.Media.WIDTH,
		// MediaStore.Video.Media.HEIGHT,
		MediaStore.Video.Media.DURATION,
		MediaStore.Video.Media.DATA
	};

	public ArrayList<Media> videoCursorToList(Cursor cursor) {
		final ArrayList<Media> list = new ArrayList<Media>();

		if (cursor.moveToFirst()) {
			int idIndex = cursor.getColumnIndex(MediaStore.Video.Media._ID);
			int dateAddedIndex = cursor.getColumnIndex(MediaStore.Video.Media.DATE_ADDED);
			int mimeTypeIndex = cursor.getColumnIndex(MediaStore.Video.Media.MIME_TYPE);
			int sizeIndex = cursor.getColumnIndex(MediaStore.Video.Media.SIZE);
			int titleIndex = cursor.getColumnIndex(MediaStore.Video.Media.TITLE);
			// int widthIndex = cursor.getColumnIndex(MediaStore.Video.Media.WIDTH);
			// int heightIndex = cursor.getColumnIndex(MediaStore.Video.Media.HEIGHT);
			int durationIndex = cursor.getColumnIndex(MediaStore.Video.Media.DURATION);
			int dataIndex = cursor.getColumnIndex(MediaStore.Video.Media.DATA);
			do {
				Video video = new Video();
				video.id = cursor.getLong(idIndex);
				video.date = cursor.getDouble(dateAddedIndex);
				video.mime = cursor.getString(mimeTypeIndex);
				video.size = cursor.getLong(sizeIndex);
				video.title = cursor.getString(titleIndex);
				// video.width = cursor.getInt(widthIndex);
				// video.height = cursor.getInt(heightIndex);
				video.duration = cursor.getDouble(durationIndex);
				video.time = 0;
				video.path = cursor.getString(dataIndex);

				list.add(video);
			} while(cursor.moveToNext());
		}

		cursor.close();
		return list;
	}

	public Bitmap getVideoThumnail(long id, boolean micro) {
		return MediaStore.Video.Thumbnails.getThumbnail(
			getContentResolver(),
			id,
			(micro
				? MediaStore.Video.Thumbnails.MICRO_KIND
				: MediaStore.Video.Thumbnails.MINI_KIND),
			null);
	}

	// ---------------------------

	private byte[] bitmapToJpegBytes(Bitmap bitmap) {
		ByteArrayOutputStream outStream = new ByteArrayOutputStream(128);
		bitmap.compress(Bitmap.CompressFormat.JPEG, 75, outStream);
		return outStream.toByteArray();
	}

	public String getGenre(long audioId) {
		Cursor cursor = getContentResolver().query(
				MediaStore.Audio.Genres.EXTERNAL_CONTENT_URI,
				new String[] { MediaStore.Audio.Genres.NAME },
				MediaStore.Audio.Media._ID + " = ?",
				new String[] { "" + audioId },
				null);
		if (!cursor.moveToFirst()) {
			Log.i("Basuke", "no genre found for song:" + audioId);
			cursor.close();
			return null;
		}

		int index = cursor.getColumnIndex(MediaStore.Audio.Genres.NAME);
		String genre = cursor.getString(index);
		Log.i("Basuke", "Genre for song:" + audioId + " is " + genre);
		cursor.close();
		return genre;
	}

	// ---------------------------

	public ArrayList<Media> getAlbums() {
		Cursor cursor = getContentResolver().query(
				MediaStore.Audio.Albums.EXTERNAL_CONTENT_URI,
				albumProjection,
				null,
				null,
				MediaStore.Audio.Media.ALBUM + " ASC");

		return albumCursorToList(cursor, false);
	}

	public ArrayList<Media> getArtistAlbums(long artistId) {
		Uri uri = MediaStore.Audio.Artists.Albums.getContentUri("external", artistId);

		Cursor cursor = getContentResolver().query(
				uri,
				albumProjectionForArtist,
				null,
				null,
				MediaStore.Audio.Media.ALBUM + " ASC");

		return albumCursorToList(cursor, true);
	}

	public ArrayList<Media> getArtists() {
		Cursor cursor = getContentResolver().query(
				MediaStore.Audio.Artists.EXTERNAL_CONTENT_URI,
				artistProjection,
				null,
				null,
				MediaStore.Audio.Media.ARTIST + " ASC");

		return artistCursorToList(cursor);
	}

	public ArrayList<Media> getImages(String sort) {
		boolean withDimention = (mSDKVersion >= 16);
		String sqlSort = null;
		if (sort.equals("title"))
			sqlSort = MediaStore.Images.Media.TITLE + " ASC";
		else if (sort.equals("date"))
			sqlSort = MediaStore.Images.Media.DATE_TAKEN + " DESC";
		else if (sort.equals("size"))
			sqlSort = MediaStore.Images.Media.SIZE + " ASC";

		Cursor cursor = getContentResolver().query(
				MediaStore.Images.Media.EXTERNAL_CONTENT_URI,
				(withDimention ? imageProjectionWithProjection : imageProjection),
				null,
				null,
				sqlSort);

		return imageCursorToList(cursor, withDimention);
	}

	public ArrayList<Media> getVideos(String sort) {
		Cursor cursor = getContentResolver().query(
				MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
				videoProjection,
				null,
				null,
				null);

		return videoCursorToList(cursor);
	}

	private String removePrecedingArticles(String sort) {
		sort = sort.toLowerCase();
		if (sort.startsWith("(")) {
			sort = sort.substring(1).trim();
		}
		if (sort.startsWith("the ")) {
			sort = sort.substring(4).trim();
		}
		if (sort.startsWith("an ")) {
			sort = sort.substring(3).trim();
		}
		if (sort.startsWith("a ")) {
			sort = sort.substring(2).trim();
		}
		return sort;
	}

	private String alphabetInitial(Collator collator, String sort) {
		if (collator.compare(sort, "n") < 0)
			if (collator.compare(sort, "g") < 0)
				if (collator.compare(sort, "d") < 0)
					if (collator.compare(sort, "b") < 0)
						if (collator.compare(sort, "a") < 0)
							return "#";
						else
							return "A";
					else
						if (collator.compare(sort, "c") < 0)
							return "B";
						else
							return "C";
				else // >="D"
					if (collator.compare(sort, "f") < 0)
						if (collator.compare(sort, "e") < 0)
							return "D";
						else // >="E"
							return "E";
					else // >="F"
						return "F";
			else // >="G"
				if (collator.compare(sort, "j") < 0)
					if (collator.compare(sort, "i") < 0)
						if (collator.compare(sort, "h") < 0)
							return "G";
						else
							return "H";
					else // >="I"
						return "I";
				else // >="J"
					if (collator.compare(sort, "l") < 0)
						if (collator.compare(sort, "k") < 0)
							return "J";
						else
							return "K";
					else // >="L"
						if (collator.compare(sort, "m") < 0)
							return "L";
						else
							return "M";
		else // >="N"
			if (collator.compare(sort, "u") < 0)
				if (collator.compare(sort, "r") < 0)
					if (collator.compare(sort, "p") < 0)
						if (collator.compare(sort, "o") < 0)
							return "N";
						else
							return "O";
					else
						if (collator.compare(sort, "q") < 0)
							return "P";
						else
							return "Q";
				else // >="R"
					if (collator.compare(sort, "t") < 0)
						if (collator.compare(sort, "s") < 0)
							return "R";
						else
							return "S";
					else
						return "T";
			else // >="U"
				if (collator.compare(sort, "x") < 0)
					if (collator.compare(sort, "w") < 0)
						if (collator.compare(sort, "v") < 0)
							return "U";
						else
							return "V";
					else
						return "W";
				else // >="X"
					if (collator.compare(sort, "z") < 0)
						if (collator.compare(sort, "y") < 0)
							return "X";
						else
							return "Y";
					else // >="Z"
						if (sort.substring(0, 1).equals("z"))
							return "Z";
						else
							return "#";
	}

	private void alphabetSort(ArrayList<Media> list) {
		final Collator collator = Collator.getInstance(Locale.US);
		collator.setDecomposition(Collator.CANONICAL_DECOMPOSITION);

		for (Media media : list) {
			String sort = media.sort;

			media.sort = removePrecedingArticles(media.sort);
			media.initial = alphabetInitial(collator, media.sort);
		}

		Collections.sort(list, new Comparator<Media>() {
			@Override
			public int compare(Media obj1, Media obj2) {
				boolean isAlpha1 = !obj1.initial.equals("#");
				boolean isAlpha2 = !obj2.initial.equals("#");

				if ((isAlpha1 && isAlpha2) || (!isAlpha1 && !isAlpha2)) {
					return collator.compare(obj1.sort, obj2.sort);
				}

				if (isAlpha1) return -1;
				return 1;
			}
		});
	}

	public Bitmap libraryThumbnail(String kind, int id, boolean micro) {
		Bitmap bitmap;

		if (kind.equalsIgnoreCase("image")) {
			bitmap = getImageThumnail(id, micro);
		} else if (kind.equalsIgnoreCase("video")) {
			bitmap = getVideoThumnail(id, micro);
		} else if (kind.equalsIgnoreCase("album")) {
			bitmap = getAlbumThumnail(id);
		} else {
			return null;
		}
		if (bitmap == null) return null;

		return bitmap;
	}

	public String libraryFetch(String kind, String option, String optionValue) {
		ArrayList<Media> result;

		if (kind.equalsIgnoreCase("album")) {
			if (option != null) {
				if (option.equalsIgnoreCase("artist")) {
					result = getArtistAlbums(Long.parseLong(optionValue));
				} else {
					return null;
				}
			} else {
				result = getAlbums();
			}
			alphabetSort(result);
		} else if (kind.equalsIgnoreCase("artist")) {
			if (option != null) {
				return null;
			} else {
				result = getArtists();
			}
			alphabetSort(result);
		} else if (kind.equalsIgnoreCase("genre")) {
			if (option != null) {
				return null;
			} else {
				result = genreCursorToList(allGenresCursor());
			}
			alphabetSort(result);
		} else if (kind.equalsIgnoreCase("song")) {
			boolean sort = true;

			if (option != null) {
				if (option.equalsIgnoreCase("album")) {
					result = getAlbumSongs(optionValue);
					sort = false;
				} else if (option.equalsIgnoreCase("artist")) {
					result = getArtistSongs(optionValue); // ??
				} else if (option.equalsIgnoreCase("genre")) {
					result = getGenreSongs(optionValue);
				} else {
					return null;
				}
			} else {
				result = getSongs();
			}
			
			if (sort) alphabetSort(result);
		} else if (kind.equalsIgnoreCase("image")) {
			if (option != null) {
				result = getImages(option);
			} else {
				result = getImages("title");
			}
		} else if (kind.equalsIgnoreCase("video")) {
			if (option != null) {
				result = getVideos(option);
			} else {
				result = getVideos("title");
			}
		} else {
			return null;
		}

		StringBuffer buffer = new StringBuffer(1024);

		for (Media media : result) {
			buffer.append(media.fieldValuesString());
			buffer.append(MediaRecordSeparator);
		}

		return buffer.toString();
	}
}

// end of KinomaPlay activity

