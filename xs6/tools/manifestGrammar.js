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
import Grammar from "grammar";
import infoset from "infoset";

function resolveManifestVariables(tool, value) {
	return value.replace(/\[([^\]]+)\]/g, (offset, value) => {
		if (value == "applicationPath")
			return "[applicationPath]";
		if (value in tool.environment)
			return tool.environment[value];
		return process.getenv(value);
	});
}

var kconfig = {
}
var loadableType = {
	unknown: 0,
	extension: 1,
	bytecode: 2,
	document: 3,
};
var loadable = {
	href: "",
	from: "",
	required: false,
	platform: "",
	application: "",
	embed: "",
	style: "",
	name: "",
	vmName: "",
	type: 0,
	needsBuild: true,
	
	configure(tool, tree) {
		if (tool.checkPlatform(this.platform)) {
			if ((this.platform == "android") && (this.embed != "true"))
				tree.separate[this.href] = resolveManifestVariables(tool, this.from);
			else
				tree.extensions[this.href] = resolveManifestVariables(tool, this.from);
		}
	}
};
var extension = {
	__proto__: loadable,
};
var bytecode = {
	__proto__: loadable,
};
var document = {
	__proto__: loadable,
	needsBuild: false,
};

var kind = {
	type: "",
	messages: "normal",
	flags: 0,
};

var xsdebug = {
	enabled: false,
};

var instrument = {
	log: "",
	syslog: "",
	trace: false,
	threads: false,
	times: false,
	androidlog: false,
	kinds: [],
};

var ssl = {
	CA_list: "",
};

var ui = {
	platform: "",
	fonts: [],
	configure(tool, tree) {
		if (tool.checkPlatform(this.platform))
			for (var font of this.fonts)
				font.configure(tool, tree);
	}
};
var font = {
	name: "",
	textSize: 0,
	path: "",
	engine: "",
	os: "",
	family: [],
	configure(tool, tree) {
		if (this.os)
			return;
		if ("fonts" in tree) 
			return;
		tree.fonts = {
			path: resolveManifestVariables(tool, this.path)
		}
		if (this.name)
			tree.fonts.default = this.name;
	}
};
var family = {
	name: "",
	uses: "",
};

var variable = {
	name: "",
	value: "",
	platform: "",
	application: "",
	style: "",
	resolved: "",
	
	configure(tool, tree) {
		if (this.platform == "build")
			tool.environment[this.name] = resolveManifestVariables(tool, this.value);
		else if (tool.checkPlatform(this.platform))
			tree.environment[this.name] = resolveManifestVariables(tool, this.value);
	}
};
var Variable = function(name, value) {
	this.name = name
	this.value = value
};
Variable.prototype = variable;


var buildVariable = {
	__proto__: variable,
};


var cOption = {
	name: "",
	platform: "",
};
var entitlement = {
	name: "",
	value: "",
	platform: "",

	configure(tool, tree) {
		if (tool.checkPlatform(this.platform)) {
			if (!("entitlements" in tree.info))
				tree.info.entitlements = [];
			tree.info.entitlements.push({ name: this.name, value: this.value });
		}
	},
};
var feature = {
	name: "",
	type: "",
	value: "",
	platform: "",

	configure(tool, tree) {
		if (tool.checkPlatform(this.platform)) {
			if (!("features" in tree.info))
				tree.info.features = [];
			tree.info.features.push({ name: this.name, type: this.type, value: this.value });
		}
	},
};
var orientation = {
	value: "",
	platform: "",

	configure(tool, tree) {
		if (tool.checkPlatform(this.platform)) {
			tree.info.orientation = this.value;
		}
	},
};
var permission = {
	name: "",
	platform: "",

	configure(tool, tree) {
		if (tool.checkPlatform(this.platform)) {
			if (!("permissions" in tree.info))
				tree.info.permissions = [];
			tree.info.permissions.push(this.name);
		}
	},
};
var version = {
	minimum: "",
	target: "",
	platform: "",

	configure(tool, tree) {
		if (tool.checkPlatform(this.platform)) {
			if (this.minimum)
				tree.info.version.minimum = this.minimum
			if (this.target)
				tree.info.version.target = this.target
		}
	},
};
var options = {
	entitlements: [],
	features: [],
	orientations: [],
	permissions: [],
	versions: [],

	configure(tool, tree) {
		if (tool.checkPlatform(this.platform)) {
			for (var entitlement of this.entitlements)
				entitlement.configure(tool, tree);
			for (var feature of this.features)
				feature.configure(tool, tree);
			for (var orientation of this.orientations)
				orientation.configure(tool, tree);
			for (var permission of this.permissions)
				permission.configure(tool, tree);
			for (var version of this.versions)
				version.configure(tool, tree);
		}
	},
};

var action = {
};
var exclude = {
	__proto__: action,
	path: "",
	pattern: "",
	platform: "",
	application: "",
	configure(tool, tree) {
		if (tool.checkPlatform(this.platform)) {
			var name = resolveManifestVariables(tool, this.path);
			var value = "~";
			if (value in tree.resources)
				tree.resources[value].push(name);
			else
				tree.resources[value] = [ name ];
		}
	}
};
var install = {
	__proto__: action,
	sourcePath: "",
	destinationPath: "",
	selfRegister: false,
	platform: "",
	application: "",
};
var uninstall = {
	__proto__: action,
	destinationPath: "",
	platform: "",
	application: "",
};
var copy = {
	__proto__: action,
	sourcePath: "",
	destinationPath: ".",
	platform: "",
	application: "",
	configure(tool, tree) {
		if (tool.checkPlatform(this.platform)) {
			var sourcePath = resolveManifestVariables(tool, this.sourcePath);
			var destinationPath = resolveManifestVariables(tool, this.destinationPath);
			if (tool.resolveDirectoryPath(sourcePath)) {
				if (sourcePath.endsWith("/"))
					sourcePath += "*";
				else
					sourcePath += "/*";
			}
			if (destinationPath in tree.resources)
				tree.resources[destinationPath].push(sourcePath);
			else
				tree.resources[destinationPath] = [ sourcePath ];
		}
	}
};
var registry = {
	__proto__: action,
	entry: "",
	platform: "",
	application: "",
};
var cabsetup = {
	__proto__: action,
	sourcePath: "",
	platform: "",
	application: "",
};

var vm = {
	name: "",
	alloc: "",
	
	instrument: instrument,
	ssl: ssl,
	uis: [],
	variables: [],
	
	loadables: [],
	
	options: [],
	
	configure(tool, tree) {
		if (this.instrument != instrument)
			tree.instrument = this.instrument;
		if (this.ssl.CA_list) {
			tree.environment.CA_list = this.ssl.CA_list;
			var path = tool.homePath + "/data/sslcert/" + this.ssl.CA_list;
			if ("." in tree.resources)
				tree.resources["."].push(path);
			else
				tree.resources["."] = [path];
		}
		for (var ui of this.uis)
			ui.configure(tool, tree);
		for (var variable of this.variables)
			variable.configure(tool, tree);
		for (var loadable of this.loadables)
			loadable.configure(tool, tree);
		for (var option of this.options)
			option.configure(tool, tree);
	}
};

var rootVM = {
	__proto__: vm,
};

var manifest = {
	vms: [],
	actions: [],
	cOptions: [],
	
	configure(tool, tree) {
		for (var vm of this.vms)
			vm.configure(tool, tree);
		for (var action of this.actions)
			action.configure(tool, tree);
		tree.xsdebug = this.xsdebug? this.xsdebug : xsdebug;
	}
};


var g = new Grammar(infoset);
g.namespace("http://www.kinoma.com/Fsk/manifest/1", "build");
g.namespace("http://www.kinoma.com/Fsk/1", "xs0");
g.object(variable, "xs0:variable", {
	name: g.string("@name"),
	value: g.string("@value"),
	platform: g.string("@platform"),
	application: g.string("@application"),
	style: g.string("@build:style")
});
g.object(buildVariable, "build:variable", {});
g.object(loadable, "", {
	href: g.string("@href"),
	from: g.string("@build:from"),
	required: g.boolean("@required"),
	platform: g.string("@platform"),
	application: g.string("@build:application"),
	embed: g.string("@build:embed"),
	style: g.string("@build:style")
});
g.object(extension, "xs0:extension", {});
g.object(bytecode, "xs0:bytecode", {});
g.object(document, "xs0:document", {});
g.object(kind, "xs0:kind", {
	type: g.string("@name"),
	messages: g.string("@messages"),
	flags: g.number("@flags")
});
g.object(xsdebug, "xs0:xsdebug", {
	enabled: g.boolean("@enabled"),
});
g.object(instrument, "xs0:instrument", {
	log: g.string("@log"),
	syslog: g.string("@syslog"),
	trace: g.boolean("@trace"),
	threads: g.boolean("@threads"),
	times: g.boolean("@times"),
	androidlog: g.boolean("@androidlog"),
	kinds: g.array(".", kind)
});
g.object(ssl, "xs0:ssl", {
	CA_list: g.string("xs0:CA_list/@href")
});
g.object(family, "xs0:family", {
	name: g.string("@name"),
	uses: g.string("@uses")
});
g.object(font, "xs0:font", {
	name: g.string("@face"),
	textSize: g.number("@size"),
	path: g.string("@href"),
	engine: g.string("@engine"),
	os: g.string("@os"),
	family: g.array(".", family)
});
g.object(ui, "xs0:ui", {
	platform: g.string("@platform"),
	fonts: g.array(".", font)
});
g.object(cOption, "xs0:c", {
	name: g.string("@option"),
	platform: g.string("@platform")
});
g.object(entitlement, "xs0:entitlement", {
	name: g.string("@name"),
	value: g.string("@value"),
	platform: g.string("@platform")
});
g.object(feature, "xs0:feature", {
	name: g.string("@name"),
	type: g.string("@type"),
	value: g.string("@value"),
	platform: g.string("@platform")
});
g.object(orientation, "xs0:orientation", {
	value: g.string("@value"),
	platform: g.string("@platform")
});
g.object(permission, "xs0:permission", {
	name: g.string("@name"),
	platform: g.string("@platform")
});
g.object(version, "xs0:versions", {
	minimum: g.string("@minimum"),
	target: g.string("@target"),
	platform: g.string("@platform")
});
g.object(options, "xs0:options", {
	entitlements: g.array(".", entitlement),
	features: g.array(".", feature),
	orientations: g.array(".", orientation),
	permissions: g.array(".", permission),
	versions: g.array(".", version)
});
g.object(exclude, "build:exclude", {
	path: g.string("@path"),
	pattern: g.string("@pattern"),
	platform: g.string("@platform"),
	application: g.string("@application")
});
g.object(install, "build:install", {
	sourcePath: g.string("@sourcePath"),
	destinationPath: g.string("@destinationPath"),
	selfRegister: g.boolean("@selfRegister"),
	platform: g.string("@platform"),
	application: g.string("@application")
});
g.object(uninstall, "build:uninstall", {
	destinationPath: g.string("@destinationPath"),
	platform: g.string("@platform"),
	application: g.string("@application")
});
g.object(copy, "build:copy", {
	sourcePath: g.string("@sourcePath"),
	destinationPath: g.string("@destinationPath"),
	platform: g.string("@platform"),
	application: g.string("@application")
});
g.object(registry, "build:registry", {
	entry: g.string("@entry"),
	platform: g.string("@platform"),
	application: g.string("@application")
});
g.object(cabsetup, "build:cabsetup", {
	sourcePath: g.string("@sourcePath"),
	platform: g.string("@platform"),
	application: g.string("@application")
});

g.object(vm, "xs0:vm", {
	name: g.string("@name"),
	alloc: g.string("@alloc"),
	loadables: g.array(".", loadable),
	
	instrument: g.reference(".", instrument),
	ssl: g.reference(".", ssl),
	uis: g.array(".", ui),
	variables: g.array("xs0:environment", variable),
	options: g.array(".", options),
});
g.object(rootVM, "xs0:rootvm", {
});
g.object(manifest, "/xs0:fsk", {
	vms: g.array(".", vm),
	actions: g.array(".", action),
	cOptions: g.array(".", cOption),
	xsdebug: g.reference(".", xsdebug),
});
g.link();
export default g;
