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
import HTTPServer from "HTTPServer";
import HTTPClient from "HTTPClient";
import Pins from "pins";
import GPIOPin from "pinmux";
import System from "system";

function decomposeUrl(url) {
	var parts = url.split("/"), args;
	var idx = parts[parts.length - 1].indexOf("?");
	if (idx >= 0) {
		var last = parts[parts.length - 1];
		parts[parts.length - 1] = last.substring(0, idx);
		args = {};
		last.substring(idx + 1).split("&").forEach(function(elm, i, arr) {
			if ((i = elm.indexOf("=")) > 0)
				args[decodeURIComponent(elm.substring(0, i))] = decodeURIComponent(elm.substring(i + 1));
		});
	}
	return {
		path: parts.map(e => decodeURIComponent(e)),
		query: args,
	};
}

var PinsHTTPHandler = {
	init(port, configuration) {
		this.http = new HTTPServer({port: port});
		this.http.onRequest = this.onRequest;
		this.http.configuration = configuration;
	},
	close() {
		this.http.close();
	},
	onRequest(http) {
		var url = decomposeUrl(http.url);
		if (url.path.length > 1 && url.path[1]) {
			// a command
			var command = url.path[1];
			Pins.invoke(command, function(result) {
				if (result)
					http.response("application/json", JSON.stringify(result));
				else
					http.errorResponse(204, "No Content");
			});
		}
		else if (url.query.path) {
			if (url.query.path == "configuration") {
				var command = url.query.path;
				Pins.invoke(command, function(result) {
					if (result)
						http.response("application/json", JSON.stringify(result));
					else
						http.errorResponse(204, "No Content");
				});
			}
			else if (url.query.repeat) {
				// repeat
				var addr, ti;
				if (url.query.callback) {
					var wild = url.query.callback.indexOf("*");
					if (wild >= 0) {
						addr = url.query.callback.substr(0, wild);
						addr += http.sock.peerAddr;
						addr += url.query.callback.substr(wild + 1);
					}
					else
						addr = url.query.callback;
				}
				if (url.query.repeat == "on")
					var ti = url.query.interval ? parseInt(url.query.interval) : url.query.timer;
				// trace("Pins.repeat: path = " + url.query.path + ", ti = " + ti + "\n");
				Pins.repeat(url.query.path, ti, function(result) {
					GPIOPin.led(1, 1);
					if (result != undefined) {
						if (addr in Pins.https) {
							GPIOPin.led(1, 0);
							return;
						}
						// trace("Pins.repeat: responding to " + addr + ", " + JSON.stringify(result) + "\n");
						var client = new HTTPClient(addr);
						client.onTransferComplete = function (status) {
							delete Pins.https[addr];
							GPIOPin.led(0, 0);
						};
						client.addHeader("Connection", "close");
						if (result instanceof ArrayBuffer) {
							client.addHeader("Content-Type", "application/octet-stream");
							client.addHeader("Content-Length", result.byteLength);
							client.start(result);
						}
						else {
							var res = JSON.stringify(result);
							client.addHeader("Content-Type", "application/json");
							client.addHeader("Content-Length", res.length);
							client.start(res);
						}
						Pins.https[addr] = client;
						GPIOPin.led(0, 1);
					}
					GPIOPin.led(1, 0);
				}, addr);
				http.errorResponse(204, "No Content");
			}
			else {
				// invoke
				var body = http.content ? JSON.parse(String.fromArrayBuffer(http.content)) : undefined;
				GPIOPin.led(0, 1);
				Pins.invoke(url.query.path, body, function(result) {
					if (result) {
						if (result instanceof ArrayBuffer)
							http.response("application/octet-stream", result);
						else
							http.response("application/json", JSON.stringify(result));
					}
					else
						http.errorResponse(204, "No Content");
					GPIOPin.led(0, 0);
				});
			}
		}
		else
			http.errorResponse(400, "Bad Request");
	},
};
export default PinsHTTPHandler;
