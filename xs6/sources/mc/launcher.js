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

/*
	Unused 

function verify(app)
{
	let Files = require.weak("files");
	var Crypt = require.weak("crypt");
	var base = Files.applicationDirectory + "/" + app;
	// first check if the signature file exists
	var sig = Files.read(base + ".sig");
	if (!sig) {
		console.log(base + ".sig" + " not found");
		return false;
	}
	var f = new File(base + ".xsb");
	var buf = new ArrayBuffer(128);
	var dgst = new Crypt.SHA1();
	while (f._read(128, buf)) {
		dgst.update(buf);
	}
	f.close();
	var h = dgst.close();
	var pk = new Crypt.PKCS1_5(Crypt.X509.decodeSPKI(Files.read("xssig.der")));
	return pk.verify(h, sig);
}
*/

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
		if (app) {
			/*
			if (!verify(app)) {
				console.log(app + ": verification failed");
				return;
			}
			*/
			this.launch(app);
		}
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
	launch(path, ...params) {
		this.quit();
		let module = require.weak(path);
		this.state.module = module;
		if (module && module.onLaunch) {
			try {
				return module.onLaunch(...params);
			} catch(e) {
				console.log("launcher: onLaunch: caught an exception");
			}
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
			if (o.cb) {
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
};

export default Launcher;
