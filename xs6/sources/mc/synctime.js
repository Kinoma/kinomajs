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
import System from "system";

let application;

var synctime = {
	start: function(mode, i) {
		application = i;
		if ((mode & (System.connection.SINGLEUSER | System.connection.MODEMASK)) == System.connection.STA)
			this.run();
	},
	run: function() {
		let req;
		let url = System.get("TIME_SERVER");
		if (url === null)
			url = System.config.timeSyncURL;
		if (url === "")
			return;	// don't do anything
		let HTTPClient = require.weak("HTTPClient");
		req = new HTTPClient(url);
		let t1, t2;
		req.onHeaders = function() {
			t2 = System.time;
			let date = this.getHeader("Date");
			if (date) {
				let t = Date.parse(date);
				t += 500;		// + 0.5 sec
				t += (t2 - t1) / 2;	// + the roundtrip time / 2
				System.time = t;
				System.init_rng(t);
				console.log("-stderr", "setting RTC: " + Date());	// going to the log file
			}
			application.remove(synctime);
			synctime._req = undefined;
		};
		req.start();
		t1 = System.time;
		synctime._req = req;
	},
	stop: function(mode) {
		this._req = undefined;
		System.gc();
	},
};

export default synctime;
