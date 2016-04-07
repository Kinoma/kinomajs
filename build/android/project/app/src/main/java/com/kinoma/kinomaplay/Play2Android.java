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
import android.os.Handler;
import android.os.Bundle;
import android.os.Message;

public class Play2Android {
	static Handler h = null;
	static Context context = null;
	
	public Play2Android() {}
	public Play2Android(Handler h) { this.h = h; }
	public Play2Android(Context context, Handler h) { this.context = context; this.h = h; }

	public static void callback() {
		Bundle b = new Bundle();
		b.putString("test", "what");
		Message m = Message.obtain();
		m.setData(b);
		m.setTarget(h);
		m.sendToTarget();
	}

	public static Class getClass(String name) {
		Class cls = null;
		try {
			if (name.indexOf('.') == -1)
				name = "com.kinoma.kinomaplay." + name;
			cls = Class.forName(name);
		}
		catch (Throwable t) {
			
		}
		return cls;
	}

	public static Context getContext() {
		return context;
	}

};

