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

var Debug = {
	gc(f) @ "xs_dbg_gcEnable",
	stress(f) @ "xs_dbg_stress",
	xstrace(f) @ "xs_dbg_xstrace",
	report(silence) @ "xs_dbg_report",
	_login(h) @ "xs_dbg_login",
	login(h, n) {
		var res = this._login(h, n);
		if (res) {
			let wdt = require.weak("watchdogtimer");
			wdt.stop();
			let mdns = require.weak("mdns");
			System._mdns_services = [];
			mdns.services.forEach(s => {
				if (s.service && s.probed) {
					System._mdns_services.push({service: s.service, txt: s.txt, ttl: s.ttl});	// remember the original TTL and TXT
					// add "busy=1" to TXT
					let txt = {};
					if (s.txt) {
						for (let key in s.txt)
							txt[key] = s.txt[key];
					}
					txt.busy = 1;
					mdns.update(s.service, txt, 24*60*60);
				}
			});
		}
		return res;
	},
	_logout() @ "xs_dbg_logout",
	logout() {
		this._logout();
		this.onDisconnect();
	},
	onDisconnect() {
		if (System._mdns_services) {
			let mdns = require.weak("mdns");
			System._mdns_services.forEach(s => mdns.update(s.service, s.txt === undefined ? null : s.txt, s.ttl));
			delete System._mdns_services;
		}
		let wdt = require.weak("watchdogtimer");
		wdt.resume();
	},
	xsdebugger() @ "xs_dbg_debugger",
	setBreakpoint() @ "xs_dbg_setBreakpoint",
};
export default Debug;
