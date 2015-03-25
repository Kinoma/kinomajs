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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.view.KeyEvent;

public class RemoteControlReceiver extends BroadcastReceiver {
	public native boolean doFskKeyEvent(int keyCode, int modifiers, int action, int param, int repeat);

    public void onReceive(Context context, Intent intent) {
 //   	Log.i("Kinoma", "RemoteControlReceiver");

    	String intentAction = intent.getAction();
        if (!Intent.ACTION_MEDIA_BUTTON.equals(intentAction)) {
        	return;
        }
            /* handle media button intent here by reading contents */
            /* of EXTRA_KEY_EVENT to know which key was pressed    */
        KeyEvent event = (KeyEvent)intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
        if (event == null) {
        	return;
        }
//    	Log.i("Kinoma", "RemoteControlReceiver - got a key event");
        int action = event.getAction();
        if (action == KeyEvent.ACTION_DOWN || action == KeyEvent.ACTION_UP) {
        	// do something
 //       	Log.i("Kinoma", "RemoteControlReceiver - dispatch it.");
			com.kinoma.kinomaplay.KinomaPlay.doFskKeyEvent(event.getKeyCode(), event.getMetaState(), event.getAction(), event.getUnicodeChar(event.getMetaState()), event.getRepeatCount());
			return;
        }
        abortBroadcast();
    }
}
    

