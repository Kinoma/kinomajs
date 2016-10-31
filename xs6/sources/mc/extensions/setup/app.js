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
const HTTP_ERROR_400 = "Bad Request";
const HTTP_ERROR_404 = "Not Found";
const HTTP_ERROR_500 = "Internal Server Error";
const HTTP_ERROR_507 = "Insufficient Storage";

const HTTP_TYPE_JSON = "application/json";

let handlers = {
	app_check(http, query) {
		try {
			if (query && 'id' in query) {
				let id = query.id;
				let Environment = require.weak("env");
				let env = new Environment();
				let app = env.get("APP_ID");
				if (app != id) {
					// remove files from previous app
					let Files = require.weak("files");
					Files.deleteVolume("k0");
					let manifest = Files.applicationDirectory + "/.manifest";
					Files.write(manifest, "{}");
					env.set("APP_ID", id);
					env.save();
				}
				http.response(HTTP_TYPE_JSON, JSON.stringify({
					"success": false
				}));
			}
			else
				http.errorResponse(400, HTTP_ERROR_400);
		}
		catch(error) {
			http.errorResponse(500, HTTP_ERROR_500);
		}
	},
	connect(http, query) {
		if (query && 'host' in query) {
			let name, host = query.host;
			if ('port' in query)
				host += ":" + query.port;
			if ('title' in query)
				name = query.title;
			let xs = require.weak("debug");
			if (xs.login(host, name))
				http.response();
			else
				http.errorResponse(500, HTTP_ERROR_500);
		}
		else
			http.errorResponse(400, HTTP_ERROR_400);
	},
	disconnect(http, query) {
		let xs = require.weak("debug");
		xs.logout();
		let Launcher = require.weak("launcher");
		Launcher.quit();	//stop app from running
		http.response();
	},
	manifest(http, query) {
		let Files = require.weak("files");
		let path = Files.applicationDirectory + "/.manifest";
		if ('put' in query) {
			if (http.content)
				Files.write(path, http.content);
			http.response();
		}
		else {
			let json;
			if (Files.getInfo(path))
				json = Files.read(path);
			else {
				Files.deleteVolume("k0");
				json = "{}";
			}
			http.response(HTTP_TYPE_JSON, json);
		}
	},
	launch(http, query) {
		let file;
		{
		let Files = require.weak("files");
		file = Files.getInfo(Files.applicationDirectory + "/main.jsb") ? "main.jsb" : "main.js";
		}
		if (query && 'file' in query && query.file)
			file = query.file;
		try {
			http.response();
			let Launcher = require.weak("launcher");
			Launcher.launch(file, query);
		}
		catch(error) {
		}
	},
	ping(http, query) {
		http.response();
	},
	install(http, query) {
		http.response();
	},
	uninstall(http, query) {
		http.response();
	},
	unload(http, query) {
		try {
			if (!('path' in query)) {
				http.response(400, HTTP_ERROR_400);
				return;
			}
			let parts = query.path.split("/");
			let f = parts.slice(2).join("/");	// file path
			let Files = require.weak("files");
			Files.deleteFile(Files.applicationDirectory + "/" + f);
			http.response();
		} catch(error) {
			http.errorResponse(500, HTTP_ERROR_500);
		}
	},
	upload(http, query) {
		let Files = require.weak("files");
		try {
			if (!('path' in query)) {
				http.response(400, HTTP_ERROR_400);
				return;
			}
			if (http.content) {
				let parts = query.path.split("/");
				let f = parts.slice(2).join("/");	// file path
				Files.write(Files.applicationDirectory + "/" + f, http.content);
			}
			http.response();
		} catch(error) {
			Files.deleteVolume("k0");
			http.errorResponse(507, HTTP_ERROR_507);
		}
	}
};

export default {
	onLaunch(http, url) {
		let handler = url.path.slice(1).join('_');
		if (handler in handlers) {
			handlers[handler](http, url.query);
		}
		else {
			http.errorResponse(404, HTTP_ERROR_404);
		}
	},
};

