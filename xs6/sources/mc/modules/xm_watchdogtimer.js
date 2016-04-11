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

let wdt = require("system").wdt;

export default {
	__proto__: {
		start(index, autostrobe, shutdown) @ "xs_wdt_start",
		strobe() @ "xs_wdt_strobe",
		stop() @ "xs_wdt_stop",
	},
	sec: 0,
	start(sec, autostrobe = true, shutdown = false) {
		let index = 0xf, s = 90 / 2;
		for (; s >= 1 && s >= sec; s /= 2, --index)
			;
		if (autostrobe)
			wdt.enable(s * 0.8 * 1000);		// strobe every 80% of the timeout
		super.start(index, autostrobe, shutdown);
		wdt.sec = sec;
		this.strobe();
	},
	stop() {
		wdt.enable();
		super.stop();
	},
	resume() {
		if (wdt.sec)
			this.start(wdt.sec);
	},
	shutdown(sec) {
		// somehow we need to re-start the wdt in the reset mode first, then re-re-start with the interrupt mode
		this.start(sec, false, false);
		super.stop();
		this.start(sec, false, true);
	},
};
