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
import {ListeningSocket, Socket} from "socket";

var conf = [
	{	// tftpd
		port: 6969,
		protocol: "udp",
		mode: System.config.tftp ? System.connection.STA | System.connection.UAP : 0,
		fqst: "_tftp._tcp",
		exec: function(sock) {
			if (this._instance) {	// only one session at once
				console.log("tftpd: rejecting a new session");
				sock.recv(sock.bytesAvailable);
				sock.send((new Uint8Array([0, 5, 3, 0, 0])).buffer, sock.peer);	// ENOSPACE
				return;
			}
			var tftpd = require.weak("tftpd");
			let instance = new tftpd();
			if (!instance.connect(sock)) {
				instance.close();
				instance = undefined;
			}
			return instance;
		},
		discovery: function() {
			let mdns = require.weak("mdns");
			mdns.add(this.fqst, System.hostname, this.port);
		},
		close: function() {
			let mdns = require.weak("mdns");
			mdns.remove(this.fqst);
		},
	},
	{	// telnetd
		port: 2323,
		protocol: "tcp",
		mode: System.config.telnet ? System.connection.STA | System.connection.UAP : 0,
		fqst: "_telnet._tcp",
		exec: function(sock) {
			if (this._instance) {	// only one connection at once
				console.log("telnetd: rejecting a connection");
				(sock.accept()).close();
				return;
			}
			var telnetd = require.weak("telnetd");
			return new telnetd(sock);
		},
		discovery: function() {
			let mdns = require.weak("mdns");
			mdns.add(this.fqst, System.hostname, this.port);
		},
		close: function() {
			let mdns = require.weak("mdns");
			mdns.remove(this.fqst);
		},
	},
	{	// Kinoma Studio
		port: 10000,
		protocol: "tcp",
		mode: System.config.kinomaStudio && System.connection.STA | System.connection.UAP,
		exec: function(sock) {
			if (!this._instance) {
				var StudioHTTPServer = require.weak("StudioHTTPServer");
				var o = new StudioHTTPServer();
			}
			else
				var o = this._instance;
			o.onConnect(sock);
			return o;
		},
		description: {
				DEVICE_TYPE: "shell",
				DEVICE_VERSION: 1,
				DEVICE_SCHEMA: "urn:schemas-kinoma-com",
				HTTP_PORT: 10000,
				LOCATION: "/",
		},
		discovery: function() {
			let ssdp = require.weak("ssdp");
			ssdp.add(this.description);
		},
		close: function() {
			let ssdp = require.weak("ssdp");
			ssdp.remove(this.description);
		},
	},
	{	// setup
		port: 8081,
		protocol: "tcp",
		mode: System.config.setup ? System.connection.UAP | System.connection.STA | System.connection.FWUPDATE : 0,
		fqst: "_kinoma_setup._tcp",
		exec: function(sock) {
			if (!this._instance) {
				var HTTPServer = require.weak("HTTPServer");
				var o = new HTTPServer({tls: false});
				o.onRequest = function(http) {
					var url = http.decomposeUrl(http.url);
					if (url.path.length == 0) {
						http.errorResponse(404, "Not Found");
					}
					else {
						try {
							let Launcher = require.weak("launcher");
							Launcher._launch("setup/" + url.path[0], http, url);
						} catch(e) {
							http.errorResponse(500, "Internal Server Error");
						}
					}
				}
			}
			else
				var o = this._instance;
			o.onConnect(sock);
			return o;
		},
		discovery: function(mode) {
			if ((mode & System.connection.UAP) && !this.dhcpd) {
				var DHCPServer = require.weak("dhcpd");
				this.dhcpd = new DHCPServer();
			}
			let uuid = require.weak("uuid");
			let mdns = require.weak("mdns");
			mdns.add(this.fqst, System.hostname, this.port, {uuid: uuid.get()}, 60);	// TTL=50
		},
		close: function() {
			if (this.dhcpd) {
				this.dhcpd.stop();
				this.dhcpd = undefined;
			}
			let mdns = require.weak("mdns");
			mdns.remove(this.fqst);
		},
	},
	{	// WAC
		port: 8082,
		protocol: "tcp",
		mode: System.config.mfiPins ? System.connection.UAP | System.connection.WAC : 0,
		seed: 0,
		exec: function(sock, mode) {
			if (mode & System.connection.UAP) {
				if (!this._instance) {
					var WACServer = require.weak("wac");
					var o = new WACServer();
				}
				else
					var o = this._instance;
				o.onConnect(sock);
				return o;
			}
			else {
				// wait for the /configured message to finish the process
				var HTTPServer = require.weak("HTTPServer");
				var o = new HTTPServer();
				o.onRequest = http => {
					// this.url has to be "/configured"
					http.setHeader("Connection", "close");	// make sure the connection is closed so the http instance will be gone
					http.response();	// just respond as it's successfuly started up
					this.close();		// and stop the mDNS.
				}
				o.onConnect(sock);
				return o;
			}
		},
		discovery: function(mode) {
			var WACServer = require.weak("wac");
			this.discoveryService = WACServer.startDiscoveryService(mode, this.port, this.seed++);
		},
		close: function() {
			var WACServer = require.weak("wac");
			WACServer.stopDiscoveryService(this.discoveryService);
		},
	},
];

var inetd = {
	_mode: 0,
	_exec: function(cp, sock, mode) {
		try {
			let instance = cp.exec(sock, mode);
			if (instance) {
				cp._instance = instance;
				cp._instance.onClose = function() {
					console.log("inetd: closing instance for " + cp.port);
					cp._instance.close();
					cp._instance = undefined;
				};
			}
		} catch (e) {
			console.log("inetd: exec failed " + cp.port);
			cp._sock.close();
			cp._sock = undefined;
			return;
		}
	},
	_runService(cp, mode) {
		if (cp._sock) {
			if ("discovery" in cp)
				cp.discovery(mode);	// try to (re)run the discovery service only
			return;
		}
		cp._instance = undefined;
		if (!(mode & cp.mode))
			return;
		console.log("inetd: starting " + cp.protocol + ":" + cp.port);
		cp._sock = new ListeningSocket({port: cp.port, proto: cp.protocol});
		switch (cp.protocol) {
		case "tcp":
			cp._sock.onConnect = () => inetd._exec(cp, cp._sock, mode);
			break;
		case "udp":
			cp._sock.onMessage = () => inetd._exec(cp, cp._sock, mode);
			break;
		}
		cp._sock.onClose = () => {
			console.log("inetd: closing socket for " + cp.port);
			if (cp._instance && "close" in cp._instance)
				cp._instance.close();
			cp._sock.close();	// close the listening socket which has been opened by inetd
			cp._sock = undefined;
			cp._instance = undefined;
		};
		cp._sock.onError = () => {
			console.log("inetd: socket error on " + cp.port);
			cp._sock.close();
			cp._sock = undefined;
			cp._instance = undefined;
		};
		if ("discovery" in cp)
			cp.discovery(mode);
	},
	_start: function(mode) {
		let mdns = require.weak("mdns");
		mdns.start();
		conf.forEach(cp => {
			try {
				this._runService(cp, mode);
			} catch(e) {
				console.log("inetd: failed to start " + cp.port);
			}
		});
	},
	_stop: function(mode) {
		conf.forEach(cp => {
			if (!(this._mode & cp.mode) || (mode & cp.mode))
				return;
			if ("close" in cp) {
				try {
					cp.close();
				} catch(e) {
					console.log("inetd: failed to close for " + cp.port);
				}
			}
			if (cp._instance && "close" in cp._instance)
				cp._instance.close();
			if (cp._sock) {
				cp._sock.close();
				console.log("inetd: stopped " + cp.port);
			}
			cp._sock = cp._instance = undefined;
		});
		let mdns = require.weak("mdns");
		mdns.stop();
	},
	start: function(mode) {
		if (mode & System.connection.SINGLEUSER)
			return;
		console.log("inetd: starting... mode = " + mode);
		this._start(mode);
		console.log("inetd: running");
		this._mode = mode;
	},
	stop: function(mode) {
		if (mode & System.connection.SINGLEUSER)
			return;
		console.log("inetd: stopping...");
		this._stop(0);
	},
	restart: function() {
		this._start(this._mode);
	},
};

export default inetd;
