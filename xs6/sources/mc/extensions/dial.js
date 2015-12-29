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
import Launcher from "launcher";

export default class Dial {
	static onQuery(http, args) {
		let url = http.url;
		let index = url.indexOf("?");
		let dial;
		try {
			let query = "";
			let path;
			if (index != -1)
				path = url.slice(11, index);
			else
				path = url.slice(11);;
			let parts = path.split("/");
			let name = parts[0];
			path = "dial/" + name + "/" + name;
			dial = require.weak(path);
			let running = dial.running ? "running" : "stopped";
			switch (http.method) {
				case "GET":
					if (parts.length == 1) {
						let additionalData = dial.additionalData;
						let response;
						let wifi = require.weak("wifi");
						response = '<?xml version="1.0" encoding="UTF-8"?>'
							+ '<service xmlns="urn:dial-multiscreen-org:schemas:dial" dialVer="1.7">'
							+ '<name>' + name + '</name>'
							+ '<options allowStop="true"/>'
							+ '<state>' + running + '</state>'
							+ '<link rel="run" href="http://' + wifi.ip + ':10000/dial/apps/' + name + '/run"/>'
							+ additionalData
							+ '</service>';
						http.response("text/xml", response);
					}
					else
						http.errorResponse(400, "Bad Request");
				break;
				case "POST":
					if ((parts.length == 2) && (parts[1] == "run") && running) {
						dial.stop();
						http.response();
					}
					else if (parts.length == 1) {
						dial.start(args);
						http.response();
					}
					else
						http.errorResponse(400, "Bad Request");
				break;
				default:
					http.errorResponse(400, "Bad Request");
				break;
			}
		} catch(error) {
			if (!dial)
				http.errorResponse(404, "Not Found");
			else
				http.errorResponse(505, "Internal Server Error");
		}
	}

	constructor() {
		this.reset();
	}
	onLaunch(args) {
		trace("Dial onLaunch\n");
		this.running = true;
	}
	onQuit() {
		trace("Dial onQuit\n");
		this.reset();
	}
	reset() {
		this.additionalData = "";
		this.running = false;
	}
	start(args) {
		trace("Dial start\n");
		Launcher.run(this, [ args ]);
	}
	stop() {
		trace("Dial stop\n");
		Launcher.quit();
	}
}
