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
import TimeInterval from "timeinterval";

export default {
	_timeout: 0,
	start(sec) {
		var index = 0xf, off = 30;
		for (var s = 90; s >= 0; s /= 2, --index, off /= 2) {
			if (sec >= s)
				break;
		}
		var that = this;
		this.timer = new TimeInterval(function() {
			that.strobe();
		}, (sec - off) * 1000);
		this.timer.start();
		this._start(index);
		this._timeout = sec;
	},
	stop() {
		if (this.timer) {
			this.timer.close();
			delete this.timer;
		}
		this._stop();
	},
	resume() {
		this.start(this._timeout);
	},
	_start(index) @ "xs_wdt_start",
	strobe() @ "xs_wdt_strobe",
	_stop() @ "xs_wdt_stop",
};
