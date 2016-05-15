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
import inetd from "inetd";

const serverURL = "https://auth.developer.cloud.kinoma.com/kinoma-device-update";

function setup(vers)
{
	let Files = require.weak("files");
	let path = Files.variableDirectory + "/";
	Files.deleteFile(path + "log0");
	Files.deleteFile(path + "log1");
	Files.deleteFile(path + "log2");
	Files.deleteFile(path + "log3");
}

function download(http, args)
{
	let HTTPClient = require.weak("HTTPClient");

	function start(queue, finish) {
		function check(q, len) {
			let File = require.weak("file");
			let Crypt = require.weak("crypt");
			let Arith = require.weak("arith");
			let Bin = require.weak("bin");
			var f = new File("/" + q.file);
			if (q.file == "wififw")
				f._read(8);		// size of the fw header
			var md5 = new Crypt.MD5();
			var blksz = md5.blockSize;
			while (len > 0) {
				md5.update(f._read(blksz > len ? len : blksz));
				len -= blksz;
			}
			f.close();
			return Bin.comp(md5.close(), (new Arith.Integer("0x" + q.md5)).toChunk(md5.outputSize)) == 0;
		};
		try {
			var q = queue.shift();
			if (!q) {
				finish(true);
				return;
			}
			trace("download: start: " + q.url + " > " + q.file + "\n");
			var req = new HTTPClient(q.url);
			req.setHeader("Connection", "close");	// until HTTPClient supports 1.1
			var len = 0, f;
			req.onHeaders = function() {
				var len = parseInt(this.getHeader("Content-Length"));
				var progress = {};
				progress[q.file] = len;
				http.putChunk(JSON.stringify(progress));
				if (len == 0)
					return;
				let File = require.weak("file");
				f = new File("/" + q.file, 1);
				if (q.file == "wififw") {
					// needs a header
					f.write("WLFW");	// magic
					f.write(len, len >> 8, len >> 16, len >> 24);	// length - native byte order
				}
			};
			req.onDataReady = function(buf) {
				f.write(buf);
				len += buf.byteLength;
				http.putChunk(JSON.stringify(len));
			};
			req.onTransferComplete = function(status) {
				trace("download: completed: " + q.file + ", size = " + len + "\n");
				if (f) f.close();
				System.gc();
				var progress = {};
				if (status && check(q, len)) {
					progress[q.file] = "OK";
					http.putChunk(JSON.stringify(progress));
					start(queue, finish);	// next
				}
				else {
					if (!status)
						console.log(q.url + " http error");
					else
						console.log(q.url + " corrupted");
					progress[q.file] = status ? "corrupted" : "http error";
					http.putChunk(JSON.stringify(progress));
					let Files = require.weak("files");
					Files.deleteDirectory("/" + q.file);
					finish(false);
				}
			};
			req.start();
			_download._f = f;
			_download._req = req;
		} catch(e) {
			finish(false);
		}
	}

	var url = serverURL, first = true;
	var noupdate = false;
	for (var i in args.query) {
		if (i == "test" && args.query[i] == "true") {
			noupdate = true;
			continue;
		}
		if (first) {
			url += "?";
			first = false;
		}
		else
			url += "&";
		url += i + "=" + args.query[i];
	}
	var req = new HTTPClient(url);
	req.setHeader("Connection", "close");	// until HTTPClient supports 1.1
	req.onTransferComplete = function(status) {
		if (!status) {
			let Launcher = require.weak("launcher");
			trace("download: HTTP failed\n");
			http.errorResponse(500, "Internal Server Error");
			Launcher.quit();
			return;
		}
		if (!this.content || this.content.byteLength == 0) {
			let Launcher = require.weak("launcher");
			trace("download: no content!\n");
			http.response();
			Launcher.quit();
			return;
		}
		var cont = String.fromArrayBuffer(this.content);
		trace(cont); trace("\n");
		var conf = JSON.parse(cont);
		var queue = [];
		for (var partition in conf.bin)
			queue.push({file: partition, url: conf.url + partition, md5: conf.bin[partition].md5});
		http.responseWithChunk("text/plain");	// transfer-encoding: "chunked"
		http.putChunk(JSON.stringify(queue.map(q => q.file)));
		_download._status = 0;
		start(queue, function(status) {
			trace("download: finished\n");
			http.terminateChunk({status: status});
			if (status && !noupdate) {
				// set active only when all partitions are succesfully updated
				let Files = require.weak("files");
				for (var partition in conf.bin)
					Files.setActive(partition);
				// update the FW version
				let Environment = require.weak("env");
				var env = new Environment();
				var currentVersion = env.get("FW_VER");
				env.set("FW_VER", conf.ver);
				env.save();
				setup(currentVersion);
				_download._status++;
				System.reboot();
			}
			else {
				let Launcher = require.weak("launcher");
				Launcher.quit();
			}
		});
	}
	req.start();
	_download._req = req;
}

let _download = {
	onLaunch(http, args) {
		let LED = require.weak("board_led");
		this.led = new LED({onColor: [0, 1, 0], offColor: [0, 0, 1], interval: 500, pattern: 1});
		this.led.run();
		inetd._stop(System.connection.FWUPDATE);
		System.gc();
		this._status = -1;
		download(http, args);
		this._http = http;
	},
	onQuit() {
		if (this._status >= 0)
			this._http.terminateChunk({status: false});
		if (this._status < 1)
			inetd.restart();
		this.led.stop();
	},
};

export default _download;
