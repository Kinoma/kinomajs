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

var Launcher = {
	s: null,
	get state() {
		let state = this.s;
		if (!state) {
			state = System.launcher;
			if (!state)
				state = this.s = System.launcher = {module: null, instances: []};
		}
		return state;
	},
	start(mode) {
		let app = System.get("APPLICATION");
		if (app)
			this.launch(app);
	},
	stop(mode) {
		this.quit();
	},
	add(instance, cb) {
		let instances = this.state.instances;
		var n = instances.length;
		while (--n >= 0) {
			if (instances[n].instance == instance)
				return;
		}
		instances.push({instance, cb});
	},
	remove(instance) {
		let instances = this.state.instances;
		var n = instances.length;
		while (--n >= 0) {
			if (instances[n].instance == instance) {
				instances.splice(n, 1);
				break;
			}
		}
		System.gc();
	},
	_launch(path, ...params) {
		let module = require.weak(path);
		if (module) {
			module._path = path;
			if (module.onLaunch)
				module.onLaunch(...params);
		}
		return module;
	},
	launch(path, ...params) {
		try {
			this.quit();
			this.state.module = this._launch(path, ...params);
		} catch(e) {
			console.log("launcher: onLaunch: caught an exception");
		}
	},
	quit() {
		let state = this.state;
		let module = state.module;
		if (module) {
			if (module.onQuit) {
				try {
					module.onQuit();
				} catch(e) {
					console.log("launcher: onQuit: caught an exception");
				}
			}
			module = null;
			state.module = null;
			System.gc();
		}
		let instances = state.instances;
		var n = instances.length;
		while (--n >= 0) {
			let o = instances[n];
			if (o && o.cb) {
				try {
					o.cb(o.instance);
				} catch(e) {
					console.log("launcher: quit: caught an exception");
				}
			}
		}
		instances.length = 0;
		System.gc();
	},
	run(module, args) {
		this.quit();
		this.state.module = module;
		if (module.onLaunch) {
			try {
				return module.onLaunch.apply(module, args);
			} catch(e) {
				console.log("launcher: onLaunch: caught an exception");
			}
		}
	},
	verify() {
		if (!this.state.module || !this.state.module._path)
			return false;
		return true;	// until the helper app is signed
		let xssig = require.weak("xssig");
		return xssig.verify(this.state.module._path);
	},
};

export default Launcher;
