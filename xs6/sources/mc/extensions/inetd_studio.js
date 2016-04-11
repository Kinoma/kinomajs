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
import TimeInterval from "timeinterval";		//@@ only used when active connection. unloadable - or doesnt matter?

const connectionTimeout = 20000;	// 20 sec

function xs() {
	return require.weak("debug");
}

let services = {
	app_check: function(http, args) {
		http.response("application/json", JSON.stringify({
			"success": false
		}));
	},

	break: function(http, args) {
		xs().xsdebugger();
		http.response();	
	},

	connect: function(http, args) {		// xsDebug
		if (args && 'host' in args) {
			let host = args.host;
			if ('port' in args)
				host += ":" + args.port;
			this.connected = xs().login(host);
			http.response("application/json", JSON.stringify({
				"root": "//",
				"status": "ready",
				"compile": true,
				"hwpdir": (System.device == "host" ? "simulator" : "device")
			}));
			if (this.connected) {
				let wdt = require.weak("watchdogtimer");
				wdt.stop();
			}
		}
		else
			http.errorResponse();
	},
	
	info: function(http, args) {
		http.response("application/json", JSON.stringify({
			"root": "//",
		}));
	},

	description: function(http, args) {
		var name = System.hostname;
		http.response('application/json', JSON.stringify({
			"firmware": System.get("FW_VER"),
			"version": "7.1.41",	// version should be at least 7.1 (XS6)
			"name": name || "Kinoma Element",
			"debugShell": "true",
			"id": "com.marvell.kinoma.launcher.element",
			"studio": {
			   "version": "1.3.49",
			   "locked": false,
			   "compile": "always",
			   "install": "always",
			   "profile": false
			},
			"xsedit": true
		}));
	},

	description_icon: function(http, args) {	//	/description/icon
		let Files = require.weak("files");
		let data = Files.read('icon.png');
		if (data)
			http.response("", data);
		else
			http.errorResponse(400, "Bad Request");
	},

	disconnect: function(http, args) {
		if (this.connected) {
			xs().logout();
			this.connected = false;
			let wdt = require.weak("watchdogtimer");
			wdt.resume();
		}

		http.response(undefined, undefined, !("terminate" in args && !args.terminate));

		let Launcher = require.weak("launcher");
		Launcher.quit();	//stop app from running
	},
	
	install: function(http, args) {
		http.response();
	},

	launch: function(http, args) {
		let dir, file;
		{
		let Files = require.weak("files");
		dir = Files.applicationDirectory;
		file = Files.getInfo(dir + "/main.jsb") ? "main.jsb" : "main.js";
		}
		if ('file' in args && args.file)
			file = args.file;
		if (http.content) {
			let content = JSON.parse(String.fromArrayBuffer(http.content));
			if ('breakpoints' in content)
				content.breakpoints.forEach(item => xs().setBreakpoint(item.file, item.line));
		}
		try {
			let Launcher = require.weak("launcher");
			Launcher.launch(file, args);
			http.response();
		} catch(error) {
			http.errorResponse(505, "Internal Server Error");
		}
	},

	manifest: function(http, args) {
		if ('put' in args) {
			http.response();
		}
		else {
			http.response('application/json', "{}");
		}
	},

	ping: function(http, args) {
		http.response();
	},
	
	uninstall: function(http, args) {
		http.response();
	},
	
	upload: function(http, args) {
		try {
			if (!('path' in args)) {
				http.response(400, "Bad Request");
				return;
			}
			var parts = args.path.split("/");
			var f = parts[parts.length - 1];	// file name
			let Files = require.weak("files");
			var dir = Files.applicationDirectory;
			if (http.content) {
				Files.write(dir + "/" + f, http.content);
			}
			http.response();
		} catch(error) {
			http.errorResponse(505, "Internal Server Error");
		}
	},
};

var StudioHttpService = function(server) {
	this.server = server;
};
StudioHttpService.prototype = {
	connected: false,

	onQuery: function(http) {		//interface exposed
		try {
			trace('from: ' + http.sock.peer + '\n');
			var url = http.url;
			var query = this.__getQuery(url);
			if (query.startsWith("_")) query = query.substring(1);
			if (query && query in services) {
				trace("StudioHttpService: calling " + query + "\n");
				services[query].call(this, http, this.__getArgs(url));
			}
			else if (url.indexOf("/dial/apps") == 0) {
				let Dial = require.weak("dial");
				Dial.onQuery(http, this.__getArgs(url));
			}
			else if (url == "/")
				http.response();
			else {
				trace("StudioHttpService: Unknown Request: " + url + "\n");
				http.errorResponse(404, "Not Found");						
			}
		} catch(error) {
			http.errorResponse(505, "Internal Server Error");
		}
	},

//@@ some form of parseURL seems needed.... this code is going to be duplicated elsewhere...
	__getQuery: function(url) {	//get query name and remove slashes
		var idx = url.indexOf("?");
		try {
			var root = url;
			if (idx != -1)
				root = url.slice(0, idx);
			return root.replace(/\//g, "_");
		} catch(error) {
			trace("Wrong query format\n");
			return "";
		}
	},

	__getArgs: function(url) {
		var args = {};
		var idx = url.indexOf("?");
		try {
			if (idx != -1){
				var parts = url.slice(idx+1).split('&');
				for (var i = 0, l = parts.length; i < l; i++ ) {
					var query = parts[i].split('=');
					args[query[0]] = decodeURIComponent(query[1]);
				}
			}	
		} catch(error) {
			trace("Wrong query format\n");
		}
		return args;
	},
};

export default function StudioHTTPServer() {
	let HTTPServer = require.weak("HTTPServer");

	this.service = undefined;
	this.http = new HTTPServer();
	this.timer = undefined;
	this.http.onRequest = http => {
		trace("StudioHTTPServer: onRequest: " + http.url + "\n");
		if (!this.service)
			this.service = new StudioHttpService(this.http);
		this.service.onQuery(http);
		if (this.http) {	// it's possible the http server has been already closed even before reaching here
			if (!this.timer) {
				this.timer = new TimeInterval(() => {
					if (this.service && this.service.connected)
						return;
					this.timer.close();
					this.timer = undefined;
					this.onClose();
				}, connectionTimeout);
				this.timer.start();
			}
			else
				this.timer.reschedule(connectionTimeout);
		}
	};
	this.http.onClose = () => {
		let service = this.service;
		if (this.timer) {
			this.timer.close();
			delete this.timer;
		}
		this.onClose();
	};
};
StudioHTTPServer.prototype = {
	onConnect: function(s) {
		this.http.onConnect(s);
	},
	onClose: function() {	/* will be overrode */
		this.close();
	},
	close: function() {
		if (this.timer) {
			this.timer.close();
			delete this.timer;
		}
		this.http.close();
		this.http = undefined;
	},
};
