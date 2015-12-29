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
import Connection from "wifi";
import System from "system";
import Debug from "debug";
import Environment from "env";
import Files from "files";
import Launcher from "launcher";

var env = new Environment();

var digits = "0123456789e+-.".split('');

var cd = null;	// root

var PROMPT = env.get("PROMPT")
if (!PROMPT)
	PROMPT = "[" + System.hostname + "]$ ";

function log(...args) {
	var sts = console.enable;
	console.enable = false;
	console.log(...args);
	console.enable = sts;
}

function split(line) {
	line = line.trim();
	if (!line) return;

	var args = [];
	while (line) {
		var end;
		var q = line.charAt(0);
		if (q == '"' || q == '\'') {
			var index = 1;

			while (true) {
				var c = line.charAt(index);
				if (c == q) {
					end = index + 1;
					break;
				}

				if (c == '') {
					line += q;
					end = index + 1;
					break;
				}

				if (c == '\\') {
					index += 1;
				}

				index += 1;
			}
		} else {
			end = line.indexOf(' ');
			if (end < 0) end = line.length;
		}

		args.push(line.substring(0, end));
		line = line.substring(end).trim();
	}

	return args;
}

function parse2native(args) {
	return args.map(arg => {
		switch (arg) {
			case 'undefined': return undefined;
			case 'null': return null;
			case 'true': return true;
			case 'false': return false;
		}

		var c = arg.charAt(0);
		if (c == '"' || c == '\'') return arg.substring(1, arg.length - 1);

		if (arg.split('').every(c => digits.indexOf(c) >= 0)) {
			return Number(arg);
		}

		return arg;
	});
}

function nameToFullPath(name, type, exists) {
	var path;

	if ((type == undefined) || (type == null))
		type = 3;

	if (name.charAt(0) == '/') {
		path = name;
	} else {
		if (cd == null)
			path = "/" + name;
		else
			path = "/" + cd + "/" + name;
	}
	var comp = path.split('/');
	comp.shift();
	while (comp.length > 0) {
		if (comp[comp.length - 1] != "") {
			break;
		}
		if ((type & 2) == 0)
			return null;
		comp.pop();
	}
	path = "/" + comp.join("/");
	if (comp.length == 0) {
		// root
	} else if (comp.length == 1) {
		// directory
		if ((type & 2) == 0) {
			return null;
		}
		if (exists && (Files.getVolumeInfo(path) == null)) {
			return null;
		}
	} else if (comp.length == 2) {
		// file
		if ((type & 1) == 0) {
			return null;
		}
		if (exists && (Files.getInfo(path) == null)) {
			return null;
		}
	} else {
		return null;
	}
	return path;
}

export default {
	connect(ssid, security, password, hidden, save) {
		Connection.connect({ssid: ssid, security: security, password: password, hidden: hidden, save: save});
	},
	reconnect(state) {
		Connection.connect(state);
	},
	netstat() {
		Connection.stat();
	},
	hostname(name) {
		if (name)
			System.hostname = name;
		return System.hostname;
	},
	ip() {
		return Connection.ip;
	},
	mac() {
		return Connection.mac;
	},
	getenv(name) {
		return env.get(name);
	},
	setenv(name, val) {
		env.set(name, val);
		return true;
	},
	saveenv() {
		return env.save();
	},
	unsetenv(name) {
		return env.unset(name);
	},
	printenv(name, encryption) {
		var tenv = name ? new Environment(name, false, encryption) : env;
		var iter = tenv.getIterator();
		var e;
		while (e = iter.next())
			log(e + "=" + tenv.get(e));
	},
	dumpenv(flags) {
		env.dump(flags);
	},
	gc() {
		System.gc();
		return true;
	},
	report() {
		Debug.report();
	},
	xsbug(h){
		if(!h) h = env.get("XSBUG_HOST");
		if(h){
			trace("XSBUG_HOST: " + h + "\n");
			Debug.login(h);
		}
	},
	load(module) {
		require.weak(module);
	},
	unlink(module) {
		(module);
	},
	ls(name) {
		var dir = cd;
		if ((name != undefined) && (name != null)) {
			var path = nameToFullPath(name, 3, 1);
			if (path == null) {


				log(name + ": No such file or directory");
				return;
			}
			var file = nameToFullPath(name, 1, 1);
			if (file != null) {
				log(file);
				return;
			}
			dir = (path == "/") ? null : path;
		}
		var iter = (dir == null) ? Files.VolumeIterator() : Files.Iterator(dir);
		var i = 0;
		for (var item of iter) {
			log(item.name);
		}
	},
	cd(dir) {
		if ((dir == undefined) || (dir == null) || (dir == "/")) {
			cd = null;
		} else {
			var path = nameToFullPath(dir, 2, 1);
			if (path == null) {
				if (nameToFullPath(dir, 1, 1) != null) {
					log(dir + ": Not a directory");
					return;
				}
				log(dir + ": No such file or directory");
				return;
			}
			cd = path.substring(1);
		}
	},
	pwd() {
		if (cd == null) {
			log("/");
		} else {
			log("/" + cd);
		}
	},
	hexdump(file) {
		var path = nameToFullPath(file, 1, 1);
		if (path == null) {
			if (nameToFullPath(file, 2, 1) != null)
				log(file + ": Is a directory");
			else
				log(file + ": No such file or directory");
			return;
		}
		var f = new Files(path), i = 0, c, line = "";
		while ((c = f.readChar()) !== undefined) {
			var x = c.toString(16).toLowerCase();
			line += (x.length == 1 ? "0" + x : x) + " ";
			if ((++i % 16) == 0) {
				log(line);
				line = "";
			}
		}
		if (line != "")
			log(line);
		f.close();
	},
	cat(file) {
		var path = nameToFullPath(file, 1, 1);
		if (path == null) {
			if (nameToFullPath(file, 2, 1) != null)
				log(file + ": Is a directory");
			else
				log(file + ": No such file or directory");
			return;
		}
		var f = new Files(path), i = 0, c, line = "";
		while ((c = f.readChar()) !== undefined) {
			var cc = String.fromCharCode(c);
			if (c == 0)
				line += "\\0";
			else if (cc == '\\')
				line += "\\\\";
			else if (cc == '\r' || cc == '\t')
				line += cc;
			else if (cc == '\n') {
				log(line);
				line = "";
			}
			else if (c < 20 || c >= 0x7f)
				line += "\\" + c.toString(16).toLowerCase();
			else
				line += cc;
		}
		if (line != "")
			log(line);
		f.close();
	},
	rm(file) {
		var path = nameToFullPath(file, 1, 1);
		if (path == null) {
			if (nameToFullPath(file, 2, 1) != null)
				log(file + ": Is a directory");
			else
				log(file + ": No such file or directory");
			return;
		}
		return Files.deleteFile(path);
	},
	date() {
		return Date();
	},
	launch(module) {
		Launcher.launch(module);
	},
	quit() {
		Launcher.quit();
	},
	reboot(force) {
		if (force === undefined)
			force = true;
		System.reboot(force);
	},
	shutdown(force) {
		System.shutdown(force);
	},
	update(target, noupdate) {
		var nullhttp = {
			response() {
			},
			errorResponse(code, status) {
				log(code, status);
			},
			responseWithChunk() {
			},
			putChunk(c) {
				log(c);
			},
			terminateChunk(o) {
				log(o);
			},
		};
		var args = {
			query: {
				target: target || "ELEMENT_FIRMWARE_SMOKE",
				test: noupdate ? "true" : "false",
			},
		};
		Launcher.launch("setup/download", nullhttp, args);
	},
	evaluate(line) {
		line = line.trim();
		if (!line) {
			this.prompt();
			return;
		}

		var args = split(line);

		var func = args.shift();
		var args = parse2native(args);

		if (func != 'evaluate' && this[func]) {
			var result = this[func].apply(this, args);
			if (result !== undefined) log(result);
		} else {
			var parts = func.split('.');
			try {
				var result = require.weak(parts.shift());
				var context = null;
				while (parts.length > 0) {
					context = result;
					result = context[parts.shift()];
				}

				if (result !== undefined) {
					if (typeof result == 'function')
						result = result.apply(context, args);
					else if ("onLaunch" in result)
						result = Launcher.run(result, args);
					if (result !== undefined) log(result);
				}
			}
			catch (e) {
				log("Exception: ", e);
			}
		}
		this.prompt();
	},
	register(obj) {
		var that = this;
		obj.onExecute = function(line) {
			that.evaluate(line);
		}
	},
	eval(s) {
		eval(s);
	},
	prompt() {
		log("-n", PROMPT);
	},
};
