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
import Dial from "dial";
import Files from "files";
import System from "system";
import xs from "debug";
import wdt from "watchdogtimer";
import Launcher from "launcher";
import TimeInterval from "timeinterval";
import Connection from "wifi";
import Environment from "env";

var connectionTimeout = 20000;	// 20 sec

var StudioHttpService = function() {};
StudioHttpService.prototype = {
	connectionFlag: false,
	scanFlag: false,

	onQuery: function(http) {		//interface exposed
		try {
			var peer = http.sock.peer;
			trace('from: ' + peer + '\n');	
			var url = http.url;		
			var query = this.__getQuery(url);
			if (query && query in this) {
				trace("StudioHttpService: calling " + query + "\n");
				this[query](http, this.__getArgs(url));
			}
			else if (url.indexOf("/dial/apps") == 0) {
				Dial.onQuery(http, this.__getArgs(url));
			}
			else {
				trace("StudioHttpService: Unknown Request: " + url + "\n");
				http.errorResponse(404, "Not Found");						
			}
		} catch(error) {
			http.errorResponse(505, "Internal Server Error");
		}
	},

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
	
	_app_check: function(http, args) {
		http.response("application/json", JSON.stringify({
			"success": false
		}));
	},

	_break: function(http, args) {
		xs.xsdebugger();
		http.response();	
	},
	
	_close: function(http, args) {
		Launcher.quit();
		http.response();
		wdt.resume();
	},

	_connect: function(http, args) {		// xsDebug
		if (args && 'host' in args) {
			this.connectionFlag = xs.login(args.host);
			http.response("application/json", JSON.stringify({
				"root": "//",
				"status": "ready",
				"compile": true,
				"hwpdir": (System.device == "host" ? "simulator" : "device")
			}));
			wdt.stop();
		}
		else
			http.errorResponse();
	},
	
	_info: function(http, args) {
      http.response("application/json", JSON.stringify({
         "root": "//",
      }));
      wdt.stop();
   },

	connected: function() {
		return this.connectionFlag;
	},

	_description: function(http, args) {
		var name = System.hostname;
		var env = new Environment();
		http.response('application/json', JSON.stringify({
			"firmware": env.get("FW_VER"),
			"version": "7.1",	// version should be at least 7.1 (XS6)
			"name": name || "Kinoma Element",
			"debugShell": "true",
			"id": "com.marvell.kinoma.launcher.element",
			"studio": {
			   "version": "1.3.46",
			   "locked": false,
			   "compile": "always",
			   "install": "always",
			   "profile": false
			}
		}));
	},

	_description_icon: function(http, args) {	//	/description/icon
		var png = Files.readChunk('icon.png');
		if (png)
			http.response("", png);
		else http.errorResponse(400, "Bad Request");
	},

	_disconnect: function(http, args) {
		if (this.connectionFlag) {
			xs.logout();
			this.connectionFlag = false;
		}
		http.response();
		var terminate = 'terminate' in args ? args.terminate : true;	// absense means yes
		if(terminate)
			Launcher.quit();	//stop app from running	
		if ('terminate' in args && args.terminate)
			this.close();
	},
	
	_install: function(http, args) {
		http.response();
	},

	_launch: function(http, args) {
		var dir = Files.applicationDirectory;
		if(Files.getInfo(dir + '/main.jsb'))
			var file = "main.jsb";
		else
			var file = "main.js";
		if ('file' in args && args.file)
			file = args.file;
		if (http.content) {
			var o = JSON.parse(String.fromArrayBuffer(http.content));
			if ('breakpoints' in o) {
				for (var i = 0; i < o.breakpoints.length; i++) {
					var brk = o.breakpoints[i];
					xs.setBreakpoint(brk.file, brk.line);
				}
			}
		}
		try {
			Launcher.launch(file, args);
			http.response();
		} catch(error) {
			http.errorResponse(505, "Internal Server Error");
		}
	},

	_manifest: function(http, args) {
		if ('put' in args) {
			http.response();
		}
		else {
			http.response('application/json', "{}");
		}
	},

	_ping: function(http, args) {
		http.response();
	},
	
	_uninstall: function(http, args) {
		http.response();
	},
	
	_upload: function(http, args) {
		try {
			if (!('path' in args)) {
				http.response(400, "Bad Request");
				return;
			}
			var parts = args.path.split("/");
			var f = parts[parts.length - 1];	// file name
			var dir = Files.applicationDirectory;
			if(http.content)
				Files.writeChunk(dir + "/" + f, http.content);
			http.response();
		} catch(error) {
			http.errorResponse(505, "Internal Server Error");
		}
	},
};

export default function StudioHTTPServer() {
	var HTTPServer = require.weak("HTTPServer");

	this.service = undefined;
	this.http = new HTTPServer();
	this.timer = undefined;
	var that = this;
	this.http.onRequest = function(http) {
		trace("StudioHTTPServer: onRequest: " + http.url + "\n");
		if (!that.service)
			that.service = new StudioHttpService();
		that.service.onQuery(http);
		if (!that.timer) {
			that.timer = new TimeInterval(function() {
				if (that.service && that.service.connected())
					return;
				trace("StudioHTTPServer: closing...\n");
				this.close();
				that.http.close();
				that.onClose();
				that.timer = undefined;
			}, connectionTimeout);
			that.timer.start();
		}
		else
			that.timer.reschedule(connectionTimeout);
	};
	this.http.onClose = function() {
		trace("StudioHTTPServer: onClose: connected = " + (that.service && that.service.connected()) + "\n");
		if (!that.service || !that.service.connected())	// check if we are in between connect and disconnect
			that.onClose();
	};
};
StudioHTTPServer.prototype = {
	onConnect: function(s) {
		this.http.onConnect(s);
	},
	close: function() {
		this.http.close();
	},
};
