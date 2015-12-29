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
import System from "system";
import Files from "files";
import Environment from "env";

var env = new Environment();

function verify(app)
{
	var Crypt = require.weak("crypt");
	var base = Files.applicationDirectory + "/" + app;
	// first check if the signature file exists
	var sig = Files.readChunk(base + ".sig");
	if (!sig) {
		console.log(base + ".sig" + " not found");
		return false;
	}
	var f = new Files(base + ".xsb");
	var buf = new ArrayBuffer(128);
	var dgst = new Crypt.SHA1();
	while (f._read(128, buf)) {
		dgst.update(buf);
	}
	var h = dgst.close();
	var pk = new Crypt.PKCS1_5(Crypt.X509.decodeSPKI(Files.readChunk("xssig.der")));
	return pk.verify(h, sig);
}

var Launcher = {
	module: undefined,
	instances: [],
	start(mode) {
		var app = env.get("APPLICATION");
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
		var n = this.instances.length;
		while (--n >= 0) {
			if (this.instances[n].instance == instance)
				return;
		}
		this.instances.push({instance, cb});
	},
	remove(instance) {
		var n = this.instances.length;
		while (--n >= 0) {
			if (this.instances[n].instance == instance) {
				this.instances.splice(n, 1);
				break;
			}
		}
		System.gc();
	},
	launch(path, ...params) {
		this.quit();
		this.module = require.weak(path);
		if (this.module && this.module.onLaunch) {
			try {
				return this.module.onLaunch(...params);
			} catch(e) {
				console.log("launcher: onLaunch: caught an exception");
			}
		}
	},
	quit() {
		if (this.module) {
			if (this.module.onQuit) {
				try {
					this.module.onQuit();
				} catch(e) {
					console.log("launcher: onQuit: caught an exception");
				}
			}
			delete this.module;
			System.gc();
		}
		var n = this.instances.length;
		while (--n >= 0) {
			var o = this.instances[n];
			if (o.cb) {
				try {
					o.cb(o.instance);
				} catch(e) {
					console.log("launcher: quit: caught an exception");
				}
			}
		}
		this.instances = [];
		System.gc();
	},
	run(mod, args) {
		this.quit();
		this.module = mod;
		if (this.module.onLaunch) {
			try {
				return this.module.onLaunch.apply(this.module, args);
			} catch(e) {
				console.log("launcher: onLaunch: caught an exception");
			}
		}
	},
};

export default Launcher;
