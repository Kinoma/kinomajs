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

import java.lang.reflect.Method;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class KinomaService extends Service {
    private NotificationManager mNotificationManager;


	/** not using ipc... dont care about this method */
	@Override
	public IBinder onBind(Intent intent) {
	  return null;
	}

	@Override
	public void onCreate() {
	  super.onCreate();

	  mNotificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
	  
	  // init the service here
//	  startForeground(1, new Notification());
	}


    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i("LocalService", "Received start id " + startId + ": " + intent);
        // We want this service to continue running until it is explicitly
        // stopped, so return sticky.
        showNotification();
        return START_NOT_STICKY;
    }

    
	@Override
	public void onDestroy() {
	  super.onDestroy();

	}

    private void showNotification() {
        CharSequence text = getText(R.string.service_running);
        Notification notification = new Notification(R.drawable.ball, text,
                System.currentTimeMillis());

        // The PendingIntent to launch our activity if the user selects this notification
        PendingIntent contentIntent = PendingIntent.getActivity(this, 0,
                new Intent(this, KinomaPlay.class), 0);

        // Set the info for the views that show in the notification panel.
        notification.setLatestEventInfo(this, getText(R.string.main_fsk),
                       text, contentIntent);

        // Send the notification.
        // We use a string id because it is a unique number.  We use it later to cancel.
 //       mNotificationManager.notify(R.string.service_started, notification);
        startForeground(R.string.service_running, notification);
    }



}
