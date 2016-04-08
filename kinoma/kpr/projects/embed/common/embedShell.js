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
import {
	HomeContainer,
	LaunchTransition,
	QuitTransition,
} from "embedHome";

var studioApplication = {
	id:"",
	program:"",
	title:"",
	version:"",
}

var g = new Grammar;
g.namespace("http://www.kinoma.com/kpr/application/1", "kpr");
g.object(studioApplication, "/kpr:application", {
	id: g.string("@id"),
	program: g.string("@program"),
	title: g.string("@title"),
	version: g.string("@version"),
});
g.link();
var zoneNumberMap = [ -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 ];
var standardTimeZonePosixMap = [ "SST+11:00", "HST+10:00", "AKST+09:00", "PST+08:00", "MST+07:00",
					"CST+06:00", "EST+05:00", "AST+04:00", "UYT+03:00", "PMST+02:00",
					"AZOST+01:00", "GMT+00:00", "CET-01:00", "EET-02:00", "IOT-03:00",
					"AST-04:00", "PKT-05:00", "BST-06:00", "THA-07:00",
					"CST-08:00", "JST-09:00", "AEST-10:00", "VUT-11:00", "NZST-12:00" ];
var daylightTimeZonePosixMap = [ "SST+11:00", "HADT+09:00", "AKDT+08:00", "PDT+07:00", "MDT+06:00",
					"CDT+05:00", "EDT+04:00", "ADT+03:00", "UYST+02:00", "PMDT+02:00",
					"AZOST+01:00", "GMT+00:00", "CEDT-02:00", "EEDT-03:00", "IOT-03:00",
					"AST-04:00", "PKT-05:00", "BST-06:00", "THA-07:00",
					"CST-08:00", "JST-09:00", "AEDT-11:00", "VUT-11:00", "NZDT-13:00" ];

export var model; 

export class EmbedShellBehavior extends Behavior {
	onComplete(shell, message, json) {
		if (message.url == "xkpr://wifi/level") {
			if (json && json.signal_level) {
				let network = this.network;
				if (network.signal_level != json.signal_level) {
					network.signal_level = json.signal_level;
					shell.distribute("onNetworkChanged", network);
				}
			}
		}
		else if (message.url == "xkpr://wifi/scanned") {
			this.network.scanned = json;
		}
	}
	onCreate() {
		model = this;
		this.firmware = getEnvironmentVariable("OS") + " " + getEnvironmentVariable("OSVersion");
		this.host = null;
		this.preferences = "embedShell.json";
		
		this.modulePath = getEnvironmentVariable("modulePath");
		this.pinsPath = "./" + getEnvironmentVariable("hwpModulePath") + "/";
		
		this.network = {
			active: false,
			ip_address: "",
			signal_level: 0,
			ssid: "",
		};
		this.networkInterfaces = [];
		
		this.onDefaults();
		this.readPreferences();
		this.configurePins();
		this.configureTimezone();
		this.onOpen();
		shell.share({ ssdp:true });
		shell.interval = 4000;
		shell.start();
	}
	onDefaults() {
		this.name = getEnvironmentVariable("NAME");
		this.startupApplication = { id:"shell", title:"Home" };
		this.timezone = { zone:-8, "daylight-savings":false };
	}
	onInvoke(shell, message) {
		trace(message.url + "\n");
		debugger
	}
	onOpen() {
		this.startup();
	}
	onQuit() {
		this.goHome();
		shell.share(false);
		this.writePreferences();
	}
	onTimeChanged() {
		if (this.scanning) {
			shell.invoke(new Message("xkpr://wifi/scan"));
			shell.invoke(new Message("xkpr://wifi/scanned"), Message.JSON);
		}
		else if (this.network.active) {
			shell.invoke(new Message("xkpr://wifi/level"), Message.JSON);
		}
	}

	configurePins() {
		shell.invoke(new MessageWithObject("pins:configure", {
			pinmux: {
				require: "pinsconfigure",
				pins: {
				}
			},
		}));
	}
	configureTimezone() {
		let timezone = this.timezone;
		let zone = timezone.zone;
		let dst = timezone["daylight-savings"];
		zone = zoneNumberMap.findIndex(number => number == zone);
		system.timezone = (dst ? daylightTimeZonePosixMap[zone] : standardTimeZonePosixMap[zone]);
	}
	deleteApplications() {
		let directory = mergePath("applications/");
		let info, iterator = new Files.Iterator(directory);
		let urls = [];
		while (info = iterator.getNext()) {
			if (Files.directoryType == info.type) {
				this.deleteApplicationPreferences(info.path);
				urls.push(mergeURI(directory, info.path + "/"));
			}
		}
		for (let url of urls) {
			try {
				Files.deleteDirectory(url, true);
			}
			catch(e) {
			}
		}
	}
	deleteApplicationPreferences(di) {
		let directory = Files.preferencesDirectory;
		let info, iterator = new Files.Iterator(directory);
		let urls = [];
		while (info = iterator.getNext()) {
			if ((Files.fileType == info.type) && (0 == info.path.indexOf(di)))
				urls.push(mergeURI(directory, info.path));
		}
		for (let url of urls) {
			try {
				Files.deleteFile(url);
			}
			catch(e) {
			}
		}
	}
	deleteApplicationsPreferences() {
		let directory = mergePath("applications/");
		let info, iterator = new Files.Iterator(directory);
		let urls = [];
		while (info = iterator.getNext()) {
			if (Files.directoryType == info.type)
				this.deleteApplicationPreferences(info.path);
		}
	}
	getApplication(directoryURL) {
		let projectURL = mergeURI(directoryURL, "project.json");
		let result = null;
		if (Files.exists(projectURL)) {
			result = Files.readJSON(projectURL);
		}
		else {
			projectURL = mergeURI(directoryURL, "application.xml");
			if (Files.exists(projectURL)) {
				result = g.parse(Files.readText(projectURL));
				result.main = result.program;
				delete result.program;
			}
		}
		return result;
	}
	getApplications() {
		let results = [ { id: "shell", title: "Home" } ];
		let directory = mergePath("applications/");
		let info, iterator = new Files.Iterator(directory);
		while (info = iterator.getNext()) {
			if (Files.directoryType == info.type) {
				let result = this.getApplication(mergeURI(directory, info.path + "/"));
				if (result)
					results.push(result);
			}
		}
		return results;
	}
	getHostContainer() {
		return shell;
	}
	getName() {
		return this.name;
	}
	getStartupApplication() {
		return this.startupApplication;
	}
	getTimezone() {
		return this.timezone;
	}
	goHome() {
		let container = this.getHostContainer();
		if (this.home)
			return;
		this.home = new HomeContainer;
		if (this.host) {
			let message = new Message("pins:close");
			message.setRequestHeader("referrer", "xkpr://" + this.host.id);
			shell.invoke(message);
        	
			container.run(new QuitTransition, this.home, this.host);
			this.host = null;
        	setEnvironmentVariable("modulePath", this.modulePath);
		}
		else {
			container.add(this.home);
		}
	}
	goHost(modulePath, url, id) {
		let container = this.getHostContainer();
		if (this.host)
			this.goHome();
		setEnvironmentVariable("modulePath", modulePath);
		this.host = new Host({left:0, right:0, top:0, bottom:0}, url, id, false, true);
		if (this.home) {
			container.run(new LaunchTransition, this.home, this.host);
			this.home = null;
		}
		else {
			container.add(this.host);
			this.host.debugging = true;
			this.host.launch();
			this.host.adapt();
		}
	}
	launchApplication(id) {
		let di = id.split('.').reverse().join('.');
		let directoryURL = mergePath("applications/" + di + "/");
		let result = this.getApplication(directoryURL);
		if (result) {
			let mainURL = mergeURI(directoryURL, result.main);
			let pinsURL = mergeURI(directoryURL, this.pinsPath);
			let modulePath = this.modulePath + ";" + directoryURL.slice(7) + ";" + pinsURL.slice(7);
			this.goHost(modulePath, mainURL, id);
			return true;
		}
		this.goHome();
		return false;
	}
	readPreferences() {
		try {
			let url = mergeURI(Files.preferencesDirectory, this.preferences);
			if (Files.exists(url)) {
				let preferences = JSON.parse(Files.readText(url));
				this.readPreferencesObject(preferences);
			}
		}
		catch(e) {
		}
	}
	readPreferencesObject(preferences) {
		if ("name" in preferences)
			this.name = preferences.name;
		if ("startupApplication" in preferences)
			this.startupApplication = preferences.startupApplication;
		if ("timezone" in preferences)
			this.timezone = preferences.timezone;
	}
	setName(name) {
		shell.share(false);
		this.name = name;
		shell.share({ ssdp:true });
		this.writePreferences();
		shell.distribute("onNameChanged");
	}
	setStartupApplication(startupApplication) {
		this.startupApplication = startupApplication;
		this.writePreferences();
	}
	setTimezone(timezone) {
		this.timezone = timezone;
		this.writePreferences();
		shell.distribute("onTimezoneChanged");
		this.configureTimezone();
	}
	startup() {
		if (this.startupApplication.id == "shell")
			this.goHome();
		else
			this.launchApplication(this.startupApplication.id);
	}
	writePreferences() {
		try {
			let url = mergeURI(Files.preferencesDirectory, this.preferences);
			let preferences = {}
			this.writePreferencesObject(preferences);
			Files.deleteFile(url);
			Files.writeText(url, JSON.stringify(preferences, null, 4));
		}
		catch(e) {
		}
	}
	writePreferencesObject(preferences) {
		preferences.name = this.name;
		preferences.startupApplication = this.startupApplication;
		preferences.timezone = this.timezone;
	}
};


// HANDLERS

function mergePath(path) {
	return mergeURI(Files.preferencesDirectory, "./embedShell/" + path);
}

Handler.Bind("/", class extends Behavior {
	onInvoke(handler, message) {
		let text = Files.readText(mergeURI(shell.url, "dd.xml"));
		text = text.replace("[friendlyName]", model.name);
		text = text.replace("[udn]", shell.uuid);
		message.responseText = text;
		message.setResponseHeader("Content-Type", "text/xml; charset=\"utf-8\"");
		message.status = 200;
	}
});

Handler.Bind("/app/check", class extends Behavior {
	onInvoke(handler, message) {
		let query = parseQuery(message.query);
		let di = query.id.split('.').reverse().join('.');
		let directoryURL = mergePath("applications/" + di + "/");
		let result = model.getApplication(directoryURL);
		let json = { success:false };
		if (result) {
			json.success = true;
			json.url = directoryURL.slice(0, -1);
		}
		message.responseText = JSON.stringify(json);
		message.status = 200;
	}
});

Handler.Bind("/close", class extends Behavior {
	onInvoke(handler, message) {
		model.goHome();
		message.status = 200;
	}
});

Handler.Bind("/connect", class extends Behavior {
	onInvoke(handler, message) {
		let query = parseQuery(message.query);
		setEnvironmentVariable("debugger", query.host + ":" + query.port);
		message.status = 200;
	}
});

Handler.Bind("/description", class extends Behavior {
	onInvoke(handler, message) {
		let url = mergeURI(shell.url, "description.json");
		let description = JSON.parse(Files.readText(url));
		description.firmware = model.firmware;
		description.name = model.name;
		description.uuid = shell.uuid;
		description.version = getEnvironmentVariable("CORE_VERSION");
		message.responseText = JSON.stringify(description);
		message.status = 200;
	}
});

Handler.Bind("/description/icon", class extends Behavior {
	onInvoke(handler, message) {
		message.responsePath = mergeURI(shell.url, "icon.png");
		message.status = 200;
	}
});

Handler.Bind("/description/picture", class extends Behavior {
	onInvoke(handler, message) {
		message.responsePath = mergeURI(shell.url, "picture.png");
		message.status = 200;
	}
});

Handler.Bind("/disconnect", class extends Behavior {
	onInvoke(handler, message) {
		var query = parseQuery(message.query);
		var terminate = ("terminate" in query) ? query.terminate == 'true' : true;
		if (terminate)
			model.goHome();
		else
			model.host.debugging = false;
		message.status = 200;
	}
});

Handler.Bind("/install", class extends Behavior {
	onInvoke(handler, message) {
		message.status = 200;
	}
});

Handler.Bind("/launch", class extends Behavior {
	onInvoke(handler, message) {
		let id;
		if (message.method == "POST") {
			let json = JSON.parse(message.requestText);
			id = json.application.id;
		}
		else {
			let query = parseQuery(message.query);
			id = query.id;
		}
		message.status = model.launchApplication(id) ? 200 : 404;
	}
});

Handler.Bind("/manifest", class extends Behavior {
	onAccept(handler, message) {
		if (message.method == "PUT") {
			let query = parseQuery(message.query);
			let url = mergePath(query.path + "/.manifest");
			Files.ensureDirectory(url);
			message.requestPath = url
		}
		return true;
	}
	onInvoke(handler, message) {
		if (message.method == "GET") {
			let query = parseQuery(message.query);
			let url = mergePath(query.path + "/.manifest");
			if (Files.exists(url))
				message.responsePath = url;
			else
				message.responseText = JSON.stringify(new Object());
		}
		message.status = 200;
	}
});

Handler.Bind("/network/connect", class extends Behavior {
	onInvoke(handler, message) {
		// ??
	}
});

Handler.Bind("/network/connect/status", class extends Behavior {
	onInvoke(handler, message) {
		message.responseText = JSON.stringify(model.network);
		message.status = 200;
	}
});

Handler.Bind("/network/interface/add", class extends Behavior {
	onComplete(handler, message, json) {
		var network = model.network;
		if ("ip_address" in json && "ssid" in json) {
			if (network.ip_address != json.ip_address)
				debugger;
			network.ip_address = json.ip_address;
			network.ssid = json.ssid;
		}
		shell.distribute("onNetworkChanged", network);
	}	
	onInvoke(handler, message) {
		var networkInterfaces = model.networkInterfaces;
		var query = parseQuery(message.query);
		networkInterfaces.push(query);
		if (networkInterfaces.length == 1) {
			var network = model.network;
			network.active = true;
			network.address = query.MAC;
			network.ip_address = query.ip;
			network.ssid = system.SSID;
			handler.invoke(new Message("xkpr://wifi/status"), Message.JSON);
		}
	}
});

Handler.Bind("/network/interface/remove", class extends Behavior {
	onInvoke(handler, message) {
		var networkInterfaces = model.networkInterfaces;
		var query = parseQuery(message.query);
		var index = networkInterfaces.findIndex(networkInterface => networkInterface.name == query.name);
		if (index >= 0) {
			networkInterfaces.splice(index, 1);
			if (networkInterfaces.length == 0) {
				var network = model.network;
				network.active = false;
				network.address = "";
				network.ip_address = "";
				network.ssid = "";
				network.signal_level = 0;
				shell.distribute("onNetworkChanged", network);
			}
		}
	}
});

Handler.Bind("/network/known", class extends Behavior {
	onInvoke(handler, message) {
		if (method == "PUT") {
		}
		else if (method == "DELETE") {
		}
		else {
			message.responseText = JSON.stringify([]);
		}
		message.status = 200;
	}
});

Handler.Bind("/network/status", class extends Behavior {
	onInvoke(handler, message) {
		message.responseText = JSON.stringify(model.network);
		message.status = 200;
	}
});

Handler.Bind("/network/wifi/scan/start", class extends Behavior {
	onInvoke(handler, message) {
		model.scanning = true;
		model.network.scanned = [];
		model.onTimeChanged();
		message.status = 200;
	}
});

Handler.Bind("/network/wifi/scan/stop", class extends Behavior {
	onInvoke(handler, message) {
		model.scanning = false;
		model.network.scanned = [];
		message.status = 200;
	}
});

Handler.Bind("/ping", class extends Behavior {
	onInvoke(handler, message) {
    	message.responseText = "OK: " + new Date();
		message.status = 200;
	}
});

Handler.Bind("/settings/clear-apps", class extends Behavior {
	onInvoke(handler, message) {
		model.deleteApplications();
		message.status = 200;
	}
});

Handler.Bind("/settings/clear-apps-prefs", class extends Behavior {
	onInvoke(handler, message) {
		model.deleteApplicationsPreferences();
		message.status = 200;
	}
});

Handler.Bind("/settings/name", class extends Behavior {
	onInvoke(handler, message) {
		if (message.method == "PUT")
			model.setName(JSON.parse(message.requestText));
		else
			message.responseText = JSON.stringify(model.getName());
		message.status = 200;
	}
});

Handler.Bind("/settings/startup-app", class extends Behavior {
	onInvoke(handler, message) {
		if (message.method == "PUT")
			model.setStartupApplication(JSON.parse(message.requestText));
		else
			message.responseText = JSON.stringify(model.getStartupApplication());
		message.status = 200;
	}
});

Handler.Bind("/settings/startup-app-list", class extends Behavior {
	onInvoke(handler, message) {
		message.responseText = JSON.stringify(model.getApplications());
		message.status = 200;
	}
});

Handler.Bind("/settings/pinmux/dialog", class extends Behavior {
	onComplete(handler, message, json) {
		handler.invoke(new MessageWithObject(this.callbackURL, json));
	}
	onInvoke(handler, message) {
		let query = parseQuery(message.query);
		let referrer = message.getRequestHeader("referrer");
		this.callbackURL = mergeURI(referrer, query.ok);
        handler.invoke(new MessageWithObject("pins:/pinmux/set", message.requestObject), Message.JSON);
		message.status = 200;
	}
});

Handler.Bind("/settings/timezone", class extends Behavior {
	onInvoke(handler, message) {
		if (message.method == "PUT")
			model.setTimezone(JSON.parse(message.requestText));
		else
			message.responseText = JSON.stringify(model.getTimezone());
		message.status = 200;
	}
});

Handler.Bind("/uninstall", class extends Behavior {
	onInvoke(handler, message) {
		message.status = 200;
	}
});

Handler.Bind("/unload", class extends Behavior {
	onInvoke(handler, message) {
		let query = parseQuery(message.query);
		let url = mergePath(query.path);
		let info = Files.getInfo(url);
		if (info) {
			if (info.type == Files.directoryType)
				Files.deleteDirectory(url);
			else
				Files.deleteFile(url);
		}
		message.status = 200;
	}
});

Handler.Bind("/upload", class extends Behavior {
	onAccept(handler, message) {
		let query = parseQuery(message.query);
		let url = mergePath(query.path);
        Files.ensureDirectory(url);
		message.requestPath = url;
		shell.distribute("onUploadBegan", url, message.getRequestHeader("Content-Length"));
		return true;
	}
	onInvoke(handler, message) {
		let query = parseQuery(message.query);
		let url = mergePath(query.path);
		shell.distribute("onUploadEnded", url);
		message.status = 200;
	}
});


