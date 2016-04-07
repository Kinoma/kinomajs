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
//#ifdefined C2D_MESSAGE
package com.kinoma.kinomaplay;

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import org.json.JSONObject;

import com.google.android.gms.gcm.GoogleCloudMessaging;

public class GcmIntentService extends IntentService {
	private static final String GCM_TITLE_KEY = "GCM Title";
	private static final String GCM_MESSAGE_KEY = "GCM Message";
    private NotificationManager mNotificationManager;
    NotificationCompat.Builder builder;

    public GcmIntentService() {
        super("GcmIntentService");
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        Bundle extras = intent.getExtras();
        GoogleCloudMessaging gcm = GoogleCloudMessaging.getInstance(this);
		String messageType = gcm.getMessageType(intent);

        if ((gcm != null) && !extras.isEmpty()) {
			try {
				if (messageType.equals(GoogleCloudMessaging.MESSAGE_TYPE_SEND_ERROR)) {
				} else if (messageType.equals(GoogleCloudMessaging.MESSAGE_TYPE_DELETED)) {
				} else {
					String message = extras.getString("message");
					//Log.i("kinoma", "message:" + message);
					JSONObject json = new JSONObject(message);
					String jsonString = json.toString();
					Log.i("kinoma", "json:" + jsonString);

					if (KinomaPlay.getLaunched() && KinomaPlay.active) {
						KinomaPlay.doFskOnRemoteNotification(jsonString);
					} else {
						saveMessage(jsonString);
						sendNotification(json);
					}
				}
			} catch (Exception e) {
				Log.i("kinoma", "exception");
			}
        }
        GcmBroadcastReceiver.completeWakefulIntent(intent);
    }

	private SharedPreferences getPreferences() {
		return getSharedPreferences(KinomaPlay.kConfigName, Context.MODE_PRIVATE);
	}

	private void saveMessage(String message) {
		SharedPreferences prefs = getPreferences();
		Editor editor = prefs.edit();
		editor.putString(KinomaPlay.kRemoteNotificationMessageKey, message);
		editor.commit();
		Log.i("kinoma", "saved in Preferences" + message);
	}

    private void sendNotification(JSONObject json) {
		if (mNotificationManager == null) {
			mNotificationManager = (NotificationManager)this.getSystemService(Context.NOTIFICATION_SERVICE);
		}

        PendingIntent contentIntent = PendingIntent.getActivity(this, 0,
                new Intent(this, KinomaPlay.class), 0);

		String title, message;
		try {
			title = json.getString(GCM_TITLE_KEY);
		} catch (Exception e) {
			title = "Kinoma Play";
		}
		try {
			message = json.getString(GCM_MESSAGE_KEY);
		} catch (Exception e) {
			message = "GCM received";
		}

        NotificationCompat.Builder mBuilder =
                new NotificationCompat.Builder(this)
			.setSmallIcon(R.drawable.icon)
			.setTicker(message)
			.setContentTitle(title)
			.setStyle(new NotificationCompat.BigTextStyle()
					  .bigText(message))
			.setContentText(message);

		SharedPreferences prefs = getPreferences();
		int notifType =  prefs.getInt(KinomaPlay.kRemoteNotificationTypeKey, 0);
		Log.i("Kinoma", "RemoteNotificationType: " + notifType);

		if ((notifType & 2) != 0) {
			Log.i("Kinoma", "Notification with sound");
			mBuilder.setDefaults(Notification.DEFAULT_SOUND);
		}

		mBuilder.setContentIntent(contentIntent);
		mBuilder.setAutoCancel(true);
        mNotificationManager.notify(KinomaPlay.kNotificationIDRemote, mBuilder.build());
    }
}
//#endif
