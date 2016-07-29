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

function log(...args) {
	console.log(...args);
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
	const digits = "0123456789e+-.";

	return args.map(arg => {
		switch (arg) {
			case 'undefined': return undefined;
			case 'null': return null;
			case 'true': return true;
			case 'false': return false;
		}

		var c = arg.charAt(0);
		if (c == '"' || c == '\'') return arg.substring(1, arg.length - 1);

		if (arg.split('').every(c => digits[c] >= 0)) {
			return Number(arg);
		}

		return arg;
	});
}

function nameToFullPath(name, type, exists) {
	var path;

	if ((type == undefined) || (type == null) || (type == 0))
		type = 3;

	if (!name) name = "";

	if (name.charAt(0) == '/') {
		path = name;
	} else {
		if (System.cd == null)
			path = "/" + name;
		else
			path = "/" + System.cd + "/" + name;
	}
	var comp = path.split('/');
	comp.shift();

	for (let i = 0; i < comp.length; i++) {
		if (comp[i] == "..") {
			if (i > 0) {
				comp.splice(i - 1, 2);
				i -= 2;
			} else {
				comp.splice(i, 1);
				i--;
			}
		}
	}

	while (comp.length > 0) {
		if (comp[comp.length - 1] != "") {
			break;
		}
		if ((type & 2) == 0)
			throw name + ": Is a directory";

		comp.pop();
	}
	path = "/" + comp.join("/");
	if (comp.length == 0) {
		// root
	} else if (comp.length == 1) {
		// partition (also directory)
		if ((type & 2) == 0)
			throw name + " is not a file";

		let Files = require.weak("files");
		if (exists && !Files.getVolumeInfo(path))
			throw name + ": No such volume";

	} else if (comp.length >= 2) {
		// file or directory
		let Files = require.weak("files");
		if (exists) {
			let info = Files.getInfo(path);
			if (!info)
				throw name + ": No such file or directory";

			if (!(type & 1) && (info.type == Files.fileType))
				throw name + ": Is a file";

			if (!(type & 2) && (info.type == Files.directoryType))
				throw name + ": Is a directory";
		}
	}
	return path;
}

function services(func, ...args) {
	let result = true;
	let Environment, env, Launcher, Debug, Files, File, Connection, file, dir, path, name, encryption, f, i, c, line;

	switch (func) {
		case "cat":
			file = arguments[1];
			try {
				path = nameToFullPath(file, 1, 1);
			} catch (e) {
				result = e;
				break;
			}
			File = require.weak("file");
			f = new File(path), i = 0, c, line = "";
			while ((c = f.readChar()) !== undefined) {
				let cc = String.fromCharCode(c);
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
			break;
		case "cd":
			dir = arguments[1];
			if ((dir == undefined) || (dir == null) || (dir == "/")) {
				System.cd = null;
			} else {
				try {
					path = nameToFullPath(dir, 2, 1);
				} catch (e) {
					result = e;
					break;
				}
				System.cd = (path == "/") ? null : path.substring(1);
			}
			break;
		case "connect":
			Connection = require.weak("wifi");
			Connection.connect({ssid: arguments[1], security: arguments[2], password: arguments[3], hidden: arguments[4], save: arguments[5], mode: arguments[6]});
			break;
		case "date":
			return Date();
		case "eval":
			try {
				result = eval(arguments[1]);
				if (undefined === result) result = true;	// returning undefined means we didn't handle this request
			}
			catch (e) {
				result = "Error: eval failed, " + e.toString();
			}
			break;
		case "gc":
			System.gc();
			break;
		case "getenv":
			result = System.get(arguments[1]);
			break;
		case "hexdump":
			file = arguments[1];
			try {
				path = nameToFullPath(file, 1, 1);
			} catch (e) {
				result = e;
				break;
			}
			File = require.weak("file");
			f = new File(path), i = 0, c, line = "";
			while ((c = f.readChar()) !== undefined) {
				let x = c.toString(16).toLowerCase();
				line += (x.length == 1 ? "0" + x : x) + " ";
				if ((++i % 16) == 0) {
					log(line);
					line = "";
				}
			}
			if (line != "")
				log(line);
			f.close();
			break;
		case "hostname":
			name = arguments[1];
			if (name)
				System.hostname = name;
			return System.hostname;
		case "ip":
			Connection = require.weak("wifi");
			return Connection.ip;
		case "launch":
			Launcher = require.weak("launcher");
			Launcher.launch(...args);
			break;
		case "load":
			require.weak(arguments[1]);
			break;
		case "ls":
			name = nameToFullPath(arguments[1]);
			Files = require.weak("files");
			if (name != "/") {
				let info;
				if (name.lastIndexOf('/') == 0)
					info = Files.getVolumeInfo(name);
				else {
					// this is redundant because getInfo may call Iterator internally.
					info = Files.getInfo(name);
					if (info && (info.type == Files.fileType)) {
						log(name);
						break;
					}
				}
				if (!info) {
					result = name + ": No such file or directory";
					break;
				}
			}
			let iter = (name == "/") ? Files.VolumeIterator() : Files.Iterator(name, arguments[2] || 0);
			for (let item of iter)
				log(item.name);
			break;
		case "mac":
			Connection = require.weak("wifi");
			return Connection.mac;
		case "modules":
			let startsWith = arguments[1];
			let cache = require.cache;
			let keys = Object.keys(cache);
			if (startsWith)
				keys = keys.filter(key => cache[key].name.toLowerCase().startsWith(startsWith.toLowerCase()));
			keys.sort((a, b) => a.toLowerCase().compare(b.toLowerCase()));
			i = 0;
			keys.forEach(key => console.log(((i++ < 9) ? " " : "") + `${i}: ${cache[key].name}`));
			break;
		case "netstat":
			Connection = require.weak("wifi");
			Connection.stat();
			break;
		case "printenv":
			name = arguments[1], encryption = arguments[2];
			Environment = require.weak("env");
			if (encryption) {
				if (!System.config.printEncryptedEnvironment) {
					result = "Error: cannot print encrypted data";
					break;
				}
			}
			env = name ? new Environment(name, false, encryption, false) : new Environment();
			for (let e of env)
				log(e + "=" + env.get(e));
			break;
		case "pwd":
			result = "/";
			if (System.cd)
				result += System.cd;
			break;
		case "quit":
			Launcher = require.weak("launcher");
			Launcher.quit();
			break;
		case "reboot": {
			let mode = arguments[1];
			let force = false;
			switch (typeof mode) {
				default:
				case "undefined":
					break;
				case "number":
					Environment = require.weak("env");
					env = new Environment();
					env.set("BOOT_MODE", mode.toString());
					env.save();
					break;
				case "boolean":
					force = mode;
					break;
			}
			System.reboot(force);
			}
			break;
		case "reconnect":
			Connection = require.weak("wifi");
			Connection.connect(arguments[1]);
			break;
		case "report":
			Debug = require.weak("debug");
			Debug.report();
			break;
		case "rename":
		case "mv":
			file = arguments[1];
			try {
				path = nameToFullPath(file, 1, 1);
			} catch (e) {
				result = e;
				break;
			}
			Files = require.weak("files");
			result = Files.renameFile(path, arguments[2]);
			if (!result) result = "Error: rename failed";
			break;
		case "rmdir":
			dir = arguments[1];
			try {
				path = nameToFullPath(dir, 2, 1);
			} catch (e) {
				result = e;
				break;
			}
			Files = require.weak("files");
			result = Files.deleteDirectory(path);
			if (!result) result = "Error: rmdir failed";
			break;
		case "rm":
			file = arguments[1];
			try {
				path = nameToFullPath(file, 1, 1);
			} catch (e) {
				result = e;
				break;
			}
			Files = require.weak("files");
			result = Files.deleteFile(path);
			if (!result) result = "Error: rm failed";
			break;
		case "saveenv":
			Environment = require.weak("env");
			env = new Environment();
			result = env.save() ? true : "Error: saveenv failed";
			break;
		case "scan":
			Connection = require.weak("wifi");
			var rescan;
			var printAPs = function(aps) {
				if (aps) {
					log(`${aps.length} networks found`);
					for (let i = 0; i < aps.length; i++) {
						let ap = aps[i];
						log(`  [${i}] "${ap.ssid || "(hidden)"}" ${ap.security} [${ap.bssid}]`);
					}
				}
			};
			if (arguments[1])
				rescan = printAPs;
			let aps = Connection.scan(rescan);
			if (aps)
				printAPs(aps);
			break;
		case "setenv":
			Environment = require.weak("env");
			env = new Environment();
			env.set(arguments[1], arguments[2], arguments[3]);
			break;
		case "shutdown":
			System.shutdown(arguments[1]);
			break;
		case "timestamp":
			return (new Date(System.timestamp * 1000)).toString();
		case "unsetenv":
			Environment = require.weak("env");
			env = new Environment();
			result = env.set(arguments[1]) ? true : "Error: unsetenv failed";
			break;
		case "update": {
			let target = arguments[1], noupdate = arguments[2];
			let nullhttp = {
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
			let args = {
				query: {
					target: target || "ELEMENT_FIRMWARE_RELEASE",
					test: noupdate ? "true" : "false",
				},
			};
			let Launcher = require.weak("launcher");
			Launcher._launch("setup/download", nullhttp, args);
			}
			break;
		case "version":
			return System.get("FW_VER") + " (" + (new Date(System.timestamp * 1000)) + ")";
		case "xsbug":
			let host = arguments[1];
			if (!host)
				host = System.get("XSBUG_HOST");
			if (host) {
				log("XSBUG_HOST: " + host + "\n");
				Debug = require.weak("debug");
				Debug.login(host);
			}
			else
				result = "Error: no xsbug host";
			break;
		default:
			return undefined;
	}

	return result;
}

function run(func, ...args) {
	if (func === undefined) return;

	let parts = func.split('.');

	let context = null, result = null;

	if (parts.length >= 2 && parts[0] == '') {
		parts.shift();
		result = require.weak("launcher").state.module;
	} else {
		result = require.weak(parts.shift());
	}

	while (parts.length > 0) {
		if (!result) throw "cannot find " + func;

		context = result;
		result = context[parts.shift()];
	}

	if (result) {
		if (typeof result == 'function')
			result = result.apply(context, args);
		else if ("onLaunch" in result) {
			let Launcher = require.weak("launcher");
			result = Launcher.run(result, args);

			let module = Launcher.state.module;
			if (module) {
				let keys = Object.keys(module);
				keys.sort();

				for (let key of keys) {
					if (['onLaunch', 'onQuit'].indexOf(key) < 0) {
						if (key.substring(0, 1) != '_') {
							if (typeof module[key] == 'function') {
								console.log('  .' + key);
							}
						}
					}
				}
			}
		}
	}

	return result;
}

export default {
	evaluate(line) {
		line = line.trim();
		if (!line) {
			this.prompt();
			return;
		}

		let args = split(line);
		args = parse2native(args);

		let message = services.apply(this, args);
		if (undefined !== message) {
			if (true !== message)
				log(message);
		} else {
			try {
				message = run(...args);
				if (message) log(message);
			} catch (e) {
				log("Exception: ", e);
			}
		}
		this.prompt();
	},
	prompt() {
		if (!this.PROMPT) {
			this.PROMPT = System.get("PROMPT");
			if (!this.PROMPT)
				this.PROMPT = "[" + (System.hostname || "Kinoma Element") + "]$ ";
		}
		log("-n", this.PROMPT);
	},
};
