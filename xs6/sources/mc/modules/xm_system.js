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

import TimeInterval from "timeinterval";

let System = {
	init() @ "xs_system_init",
	fin() @ "xs_system_fin",
	get osVersion() @ "xs_system_get_osVersion",
	get device() @ "xs_system_get_device",
	get platform() @ "xs_system_get_platform",
	get hostname() @ "xs_system_get_hostname",
	set hostname(it) @ "xs_system_set_hostname",
	get time() @ "xs_system_get_time",
	set time(t) @ "xs_system_set_time",
	get timezone() @ "xs_system_get_timezone",
	set timezone(tz) @ "xs_system_set_timezone",
	get timestamp() @ "xs_system_get_timestamp",
	init_rng(seed) @ "xs_system_init_rng",
	rng(n) @ "xs_system_get_rng",
	_init_key(key) @ "xs_system_init_key",
	load(p) @ "xs_system_load",
	sleep(n) @ "xs_system_sleep",
	gc() @ "xs_system_gc",
	_run() @ "xs_system_run",
	addPath(path) @ "xs_system_addPath",
	pm(level, timeout) @ "xs_system_pm",
	reboot(force) @ "xs_system_reboot",
	shutdown(force) @ "xs_system_shutdown",
	onShutdown: function() {},
	run(cb) {
		if (cb)
			this.onShutdown = cb;
		return this._run();
	},
	sched(cb, ...params) {
		if (!System.callbacks)
			System.callbacks = [];
		var t = new TimeInterval(() => {
			t.close();
			cb(...params);
			let i = System.callbacks.indexOf(t);
			if (i >= 0)
				System.callbacks.splice(i, 1);
		}, 0);
		t.start();
		System.callbacks.push(t);
	},
	get(name) {
		let Environment = require.weak("env");
		let env = new Environment();
		return env.get(name);
	},
	wdt: {
		sec: 0,
		timer: undefined,
		enable: function(i) {
			if (undefined != i) {
				this.timer = new TimeInterval(() => {
					let wdt = require.weak("watchdogtimer");
					wdt.strobe();
				}, i);
				this.timer.start();
			}
			else if (this.timer) {
				this.timer.close();
				delete this.timer;
			}
		}
	},
	pins: [],		// pin array for pinmux.js
	cd: null,		// current directory for CLI
	_config: {
		telnet: undefined,
		tftp: undefined,
		kinomaStudio: undefined,
		setup: undefined,
		mfiPins: undefined,
		timeSyncURL: undefined,
		// ledPins, wakeupButtons and powerGroundPinmux will be set in C
	},
	set config(conf) {
		for (let i in conf) {
			if (i in this._config)
				this._config[i] = conf[i];
		}
	},
	get config() {
		return this._config;
	},
	versionCompare(v1, v2) {
		let a1 = v1.split('.');
		let a2 = v2.split('.');
		for (let i = 0; i < a1.length; i++) {
			if (i >= a2.length)
				return 1;
			let d = parseInt(a1[i]) - parseInt(a2[i]);
			if (d != 0)
				return d;
		}
		if (a1.length < a2.length)
			return -1;
		return 0;
	},
};

System.init();
delete System.init;

export default System;
