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
import Connection from "wifi";
import {ListeningSocket} from "socket";
import Environment from "env";
import Launcher from "launcher";

var env = new Environment();

var conf = [
	{	// tftpd
		port: 6969,
		protocol: "udp",
		mode: Connection.STA | Connection.UAP,
		fqst: "_tftp._tcp.local",
		exec: function(sock) {
			var tftpd = require.weak("tftpd");
			return new tftpd(sock);
		},
		discovery: function() {
			this.mdns = require.weak("mdns");
			this.mdns.start("local", System.hostname);
			this.mdns.newService(this.fqst, this.port, System.hostname);
		},
		close: function() {
			if (this.mdns) {
				this.mdns.removeService(this.fqst);
				this.mdns.stop();
				delete this.mdns;
			}
		},
	},
	{	// telnetd
		port: 2323,
		protocol: "tcp",
		mode: Connection.STA | Connection.UAP,
		fqst: "_telnet._tcp.local",
		exec: function(sock) {
			var telnetd = require.weak("telnetd");
			return new telnetd(sock);
		},
		discovery: function() {
			this.mdns = require.weak("mdns");
			this.mdns.start("local", System.hostname);
			this.mdns.newService(this.fqst, this.port, System.hostname);
		},
		close: function() {
			if (this.mdns) {
				this.mdns.removeService(this.fqst);
				this.mdns.stop();
				delete this.mdns;
			}
		},
	},
	{	// Kinoma Studio
		port: 10000,
		protocol: "tcp",
		mode: env.get("NO_STUDIO") ? 0 : Connection.STA | Connection.UAP,
		ssdp: undefined,
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
		discovery: function() {
			if (env.get("ELEMENT_SHELL")) {
				trace( "%com.kinoma.debug=start\n" );
				return;
			}
			var SSDPDevice = require.weak("SSDPDevice");
			var StudioDiscoveryService = function() {
				SSDPDevice.call(this)
			};
			StudioDiscoveryService.prototype = Object.create(SSDPDevice.prototype, {
				DEVICE_TYPE: {value: "shell"},
				DEVICE_VERSION: {value: 1},
				DEVICE_SCHEMA: {value: "urn:schemas-kinoma-com"},
				HTTP_PORT: {value: this.port},
				LOCATION: {value: "/"},
			});
			this.ssdp = new StudioDiscoveryService();
			this.ssdp.start();
		},
		close: function() {
			if (this.ssdp) {
				this.ssdp.stop()
				this.ssdp = undefined;
			}
		},
	},
	{	// setup
		port: 8081,
		protocol: "tcp",
		mode: Connection.UAP | Connection.STA,
		fqst: "_kinoma_setup._tcp.local",
		exec: function(sock) {
			if (!this._instance) {
				var HTTPServer = require.weak("HTTPServer");
				var o = new HTTPServer({ssl: true});
				o.onRequest = function(http) {
					var url = http.decomposeUrl(http.url);
					try {
						Launcher.launch("setup/" + url.path[0], http, url);
					} catch(e) {
						http.errorResponse(505, "Internal Server Error");
					}
				}
			}
			else
				var o = this._instance;
			o.onConnect(sock);
			return o;
		},
		discovery: function(mode) {
			if (mode == Connection.UAP) {
				var DHCPServer = require.weak("dhcpd");
				this.dhcpd = new DHCPServer();
			}
			this.mdns = require.weak("mdns");
			this.mdns.start("local", System.hostname);
			this.mdns.newService(this.fqst, this.port, System.hostname);
		},
		close: function() {
			if (this.dhcpd) {
				this.dhcpd.stop();
				this.dhcpd = undefined;
			}
			if (this.mdns) {
				this.mdns.removeService(this.fqst);
				this.mdns.stop();
				delete this.mdns;
			}
		},
	},
];

var inetd = {
	start: function(mode) {
		console.log("inetd: starting... mode = " + mode);
		conf.forEach(function(cp) {
			cp._instance = undefined;
			if (!(mode & cp.mode))
				return;
			try {
				console.log("inetd: starting " + cp.protocol + ":" + cp.port);
				cp._sock = new ListeningSocket({port: cp.port, proto: cp.protocol});
			} catch(e) {
				console.log("inetd: failed to start " + cp.port);
				return;
			}
			switch (cp.protocol) {
			case "tcp":
				cp._sock.onConnect = function() {
					try {
						cp._instance = cp.exec(this, mode);
					} catch(e) {
						console.log("inetd: exec failed " + cp.port);
						cp._sock.close();
						cp._instance = undefined;
					}
					if (cp._instance) {
						cp._instance.onClose = function() {
							console.log("inetd: onClose callback: " + cp.port);
							cp._instance = undefined;
						};
					}
					else
						console.log("inetd: no instance for " + cp.port);
				};
				break;
			case "udp":
				cp._sock.onMessage = function() {
					cp._instance = cp.exec(this);
				};
				break;
			}
			cp._sock.onClose = function() {
				console.log("inetd: socket.onClose for " + cp.port);
				if (cp._instance && "close" in cp._instance)
					cp._instance.close();
				this.close();	// close the listening socket which has been opened by inted
				cp._sock = undefined;
				cp._instance = undefined;
			};
			cp._sock.onError = function() {
				console.log("inetd: socket.onError for " + cp.port);
				this.close();
				cp._sock = undefined;
				cp._instance = undefined;
			};
			if ("discovery" in cp) {
				try {
					cp.discovery(mode);
				} catch(e) {
					console.log("inted: failed to start a discovery service for " + cp.port);
				}
			}
		}, this);
		console.log("inetd: running");
	},

	stop: function(mode) {
		console.log("inetd: stopping...");
		conf.forEach(function(cp) {
			if (!(mode & cp.mode))
				return;
			if ("close" in cp) {
				try {
					cp.close();
				} catch(e) {
					console.log("inted: failed to close for " + cp.port);
				}
			}
		}, this);
	},
};

export default inetd;
