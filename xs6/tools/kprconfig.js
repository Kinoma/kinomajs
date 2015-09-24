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
import TOOL from "tool";
import * as FS from "fs";

import MAKEFILE from "makefileGrammar";
import MANIFEST from "manifestGrammar";
import PACKAGE from "packageGrammar";

var configurationNames = {
	debug: true,
	Debug: true,
	release: false,
	Release: false,
};
var platformNames = {
	android: "android",
	Android: "android",
	iphone: "ios",
	iPhone: "ios",
	iphoneos: "ios",
	iphonesimulator: null,
	"iphone/device": "ios",
	"iphone/simulator": null,
	linux: "linux",
	Linux: "linux",
	mac: "mac",
	macosx: "mac",
	MacOSX: "mac",
	solaris: null,
	Solaris: null,
	win: "win",
	Windows: "win",
};

class Tool extends TOOL {
	constructor(argv) {
		super(argv);
		this.homePath = process.getenv("F_HOME");
		if (!this.homePath)
			throw new Error("F_HOME: variable not found!");
		this.environment = {
			F_HOME: this.homePath,
		};

		this.application = null;
		this.binPath = null;
		this.cmake = false;
		this.cmakeGenerator = null;
		this.debug = false;
		this.errorCount = 0;		
		this.ide = false;
		this.identityName = null;
		this.instrument = false;
		this.leak = false;
		this.make = false;
		this.makePath = null;
		this.manifestPath = null;
		this.outputPath = null;
		this.packages = [];
		this.patchesPath = this.resolveDirectoryPath("$(F_HOME)/xs6/patches");
		this.platform = null;
		this.provisionName = null;
		this.tmpPath = null;
		this.verbose = false;
		this.xsIncludes = this.resolveDirectoryPath("$(F_HOME)/xs/includes");
		this.xs6Includes = this.resolveDirectoryPath("$(F_HOME)/xs6/includes");
		this.windows = this.currentPlatform == "win";
		this.slash = this.windows ? "\\" : "/";
		
		this.inputDirectories = null;
		
		var name, path;
		var argc = argv.length;
		for (var argi = 1; argi < argc; argi++) {
			switch (argv[argi]) {
			case "-a":
				argi++;	
				if (argi >= argc)
					throw new Error("-a: no application!");
				name = argv[argi];
				if (this.application)
					throw new Error("-a '" + name + "': too many applications!");
				this.application = name;
				break;
			case "-b":
				argi++;	
				if (argi >= argc)
					throw new Error("-b: no directory!");
				name = argv[argi];
				if (this.binPath)
					throw new Error("-b '" + name + "': too many bin directories!");
				path = this.resolveDirectoryPath(name);
				if (!path)
					throw new Error("-b '" + name + "': directory not found!");
				this.binPath = path;
				break;
			case "-c":
				argi++;	
				name = argv[argi];
				if (name in configurationNames)
					this.debug = configurationNames[name];
				else
					throw new Error("-c '" + name + "': unknown configuration!");
				break;
			case "-csi":
				argi++;	
				if (argi >= argc)
					throw new Error("-csi: no code signing identity!");
				this.identityName = argv[argi];
				break;
			case "-d":
				this.debug = true;
				break;
			case "-g":
				argi++;
				if (argi >= argc)
					throw new Error("-g: no generator!");
				this.cmakeGenerator = argv[argi];
				while (argi++ && argi < argc && argv[argi].substring(0, 1) !== '-')
					this.cmakeGenerator += " " + argv[argi];
				break;
			case "-i":
				this.instrument = true;
				break;
			case "-I":
				this.cmake = true;
				this.ide = true;
				break;
			case "-l":
				this.leak = true;
				break;
			case "-m":
				this.make = true;
				break;
			case "-o":
				argi++;	
				if (argi >= argc)
					throw new Error("-o: no directory!");
				name = argv[argi];
				if (this.outputDirectory)
					throw new Error("-o '" + name + "': too many directories!");
				path = this.resolveDirectoryPath(name);
				if (!path)
					throw new Error("-o '" + name + "': directory not found!");
				this.outputPath = path;
				break;
			case "-p":
				argi++;	
				if (argi >= argc)
					throw new Error("-p: no platform!");
				name = argv[argi];
				if (this.platform)
					throw new Error("-p '" + name + "': too many platforms!");
				if (name in platformNames)
					name = platformNames[name];
				this.platform = name;
				break;
			case "-pp":
				argi++;	
				if (argi >= argc)
					throw new Error("-pp: no provisioning profile!");
				this.provisionName = argv[argi];
				break;
			case "-t":
				argi++;	
				if (argi >= argc)
					throw new Error("-t: no directory!");
				name = argv[argi];
				if (this.outputDirectory)
					throw new Error("-t '" + name + "': too many directories!");
				path = this.resolveDirectoryPath(name);
				if (!path)
					throw new Error("-t '" + name + "': directory not found!");
				this.tmpPath = path;
				break;
			case "-v":
				this.verbose = true;
				break;
			case "-x":
				this.cmake = true;
				break;
			case "-X":
				this.xsdebug = true;
				break;
		
			default:
				name = argv[argi];
				if (this.manifestPath)
					throw new Error("'" + name + "': too many manifests!");
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("'" + name + "': manifest not found!");
				this.manifestPath = path;
				break;
			}
		}
		if (!this.manifestPath) {
			path = this.resolveFilePath("manifest.json");
			if (path)
				this.manifestPath = path;
			else {
				path = this.resolveFilePath("manifest.xml");
				if (path)
					this.manifestPath = path;
				else
					throw new Error("no manifest!");
			}
		}
		if (!this.outputPath)
			this.outputPath = this.homePath;
		if (!this.platform)
			this.platform = this.currentPlatform;
			
		if (this.platform == "ios")
			path = this.resolveDirectoryPath("$(F_HOME)/kinoma/kpr/cmake/iphone");
		else
			path = this.resolveDirectoryPath("$(F_HOME)/kinoma/kpr/cmake/" + this.platform);
		if (!path)
			throw new Error("-p '" + this.platform + "': platform not found!");
		this.makePath = path;
		
		this.checkEnvironment();
	}
	checkEnvironment() {
		var variables;
		if (this.platform == "linux/aspen") {
			variables = [
				"FSK_SYSROOT_LIB",
				"ARM_MARVELL_LINUX_GNUEABI",
			]
		}
		else if (this.platform == "linux/bg3cdp") {
			variables = [
				"BG3CDP_TOP",
				"BG3CDP_SYSROOT_LIB",
			]
		}
		else {
			variables = [];
		}
		variables.forEach((variable, index) => {
			var value = process.getenv(variable);
			if (!value)
				throw new Error(variable + ": no environment variable!");
			this.environment[variable] = value;
		});
	}
	checkPlatform(name) {
		if (!name)
			return true;
		let names = name.split(",");
		for (name of names) {
			name = name.trim();
			if (name in platformNames)
				name = platformNames[name];
			if (this.platform == name)
				return true;
			name += "/";
			if (this.platform.indexOf(name) == 0)
				return true;
		}
		return false;
	}
	createDirectories(path, name, flag, skipVariantFlag) {
		FS.mkdirSync(path);
		path += this.slash + name;
		FS.mkdirSync(path);
		let parts = this.platform.split("/");
		path += this.slash + parts[0];
		FS.mkdirSync(path);
		if (parts.length > 1) {
			path += this.slash + parts[1];
			FS.mkdirSync(path);
		}
		if (!this.cmake) {
			if (this.debug) 
				path += this.slash + "debug";
			else
				path += this.slash + "release";
		}
		FS.mkdirSync(path);
		path += this.slash + this.application;
		if (flag)
			FS.mkdirSync(path);
		return path;
	}
	filterManifest(manifest) {
		if (!("build" in manifest))
			manifest.build = {};
		if (!("environment" in manifest))
			manifest.environment = {};
		if (!("extensions" in manifest))
			manifest.extensions = {};
		if (!("fonts" in manifest))
			manifest.fonts = {};
		if (!("info" in manifest))
			manifest.info = {};
		if (!("resources" in manifest))
			manifest.resources = {};
		if (!("separate" in manifest))
			manifest.separate = {};
		if (!("xsdebug" in manifest))
			manifest.xsdebug = {enabled: false};
		if ("platforms" in manifest) {
			let platforms = manifest.platforms;
			for (let name in platforms) {
				let value = platforms[name];
				if (name in platformNames)
					name = platformNames[name];
				if (this.platform == name) {
					this.filterPlatform(manifest, value);
				}
				else {
					name += "/";
					if (this.platform.indexOf(name) == 0) {
						this.filterPlatform(manifest, value);
						if ("platforms" in value) {
							let _platforms = value.platforms;
							for (let _name in _platforms) {
								let _value = _platforms[_name];
								if (_name in platformNames)
									_name = platformNames[_name];
								if (this.platform == name + _name) {
									this.filterPlatform(manifest, _value);
								}
							}
						}
					}
				}
			}
			delete manifest.platforms;
		}
		let properties = manifest.build;
		for (let name in properties) {
			let value = properties[name];
			if (typeof value == "string")
				this.environment[name] = this.resolveVariable(value);
			else
				this.environment[name] = value;
		}
		delete manifest.build;
		if ("variants" in manifest) {
			let variants = manifest.variants;
			for (let name in variants) {
				let value = variants[name];
				name = eval(this.resolveVariable(name));
				if (name)
					this.filterPlatform(manifest, value);
			}
			delete manifest.variants;
		}
		this.resolveProperties(manifest.environment);
		this.resolveProperties(manifest.extensions);
		this.resolveProperties(manifest.resources);
		this.resolveProperties(manifest.separate);
	}
	filterPlatform(manifest, platform) {
		this.filterProperties(manifest, platform, "build");
		this.filterProperties(manifest, platform, "environment");
		this.filterProperties(manifest, platform, "extensions");
		this.filterProperties(manifest, platform, "fonts");
		this.filterProperties(manifest, platform, "info");
		this.filterProperties(manifest, platform, "resources");
		this.filterProperties(manifest, platform, "separate");
	}
	filterProperties(manifest, platform, name) {
		if (name in platform) {
			let targets = manifest[name];
			let sources = platform[name];
			for (let name in sources) {
				let source = sources[name];
				if (source == null) 
					delete targets[name];
				else if (name in targets) {
					let target = targets[name];
					if (target instanceof Array)
						targets[name] = target.concat(source);
					else if (source instanceof Array)
						targets[name] = [target].concat(source);
					else
						targets[name] = source
				}
				else
					targets[name] = source
			}
		}
	}
	generateFskPlatform() {
		var srcPath, dstPath, srcText, dstText;
		srcPath = this.joinPath({ directory: this.makePath, name: "FskPlatform", extension: ".h" });
		srcText = FS.readFileSync(srcPath);
		srcText = srcText.replace(/#define[\s]+SUPPORT_INSTRUMENTATION[\s]+0/, "#define SUPPORT_INSTRUMENTATION " + (this.instrument ? 1 : 0));
		srcText = srcText.replace(/#define[\s]+SUPPORT_MEMORY_DEBUG[\s]+0/, "#define SUPPORT_MEMORY_DEBUG " + (this.leak ? 1 : 0));
		dstPath = this.joinPath({ directory: this.tmpPath, name: "FskPlatform", extension: ".h" });
		if (this.resolveFilePath(dstPath))
			dstText = FS.readFileSync(dstPath);
		else
			dstText = "";
		if (srcText != dstText) 
			FS.writeFileSync(dstPath, srcText);
	}
	getPath(name) {
		var parts = {
			directory: "",
			name: name,
			extension: "",
		};
		for (parts.directory of this.inputDirectories) {
			var path = this.resolveFilePath(this.joinPath(parts));
			if (path)
				return path;
		}
	}
	hasTarget(target) {
		return true;
	}
	insertUnique(array, string) {
		if (!string)
			return
		var c = array.length;
		for (var i = 0; i < c; i++)
			if (array[i] == string)
				return;
		array.push(string);		
	}
	insertUniqueDirectory(array, string) {
		if (!string)
			return
		var c = array.length;
		for (var i = 0; i < c; i++)
			if (array[i] == string)
				return;
		var parts = string.split("/");
		var path = "";
		for (let part of parts) {
			path += part;
			if (array.indexOf(path) == -1)
				array.push(path);		
			path += "/";
		}
	}
	isKPR(path) {
		var buffer = FS.readFileSync(path);
		return (buffer.indexOf("xmlns=\"http://www.kinoma.com/kpr/1\"") > 0)
				&& ((buffer.indexOf("<program") >= 0) || (buffer.indexOf("<module") >= 0) || (buffer.indexOf("<shell") >= 0));
	}
	loadPackage(path) {
		for (var _package of this.packages) {
			if (_package.__xs__path == path) {
				if (this.verbose)
					this.report("# Skipping '" + path + "'...");
				return null;
			}
		}
		if (this.verbose)
			this.report("# Loading '" + path + "'...");
		var buffer = FS.readFileSync(path);
		_package = PACKAGE.parse(buffer, path);
		this.packages.push(_package);
		return _package;
	}
	processAction(sourcePath, destinationPath, flag, tree) {
		if (sourcePath in this.excludes)
			return;
		this.excludes[sourcePath] = true;
		var kind = FS.existsSync(sourcePath);
		var parts = this.splitPath(sourcePath);
		if (kind < 0) {
			let names = FS.readDirSync(sourcePath);
			if (flag) {
				if (destinationPath == ".")
					destinationPath = parts.name;
				else
					destinationPath += this.slash + parts.name;
			}
			this.insertUniqueDirectory(tree.directoryPaths, destinationPath);
			sourcePath += this.slash;
			for (let name of names) {
				this.processAction(sourcePath + name, destinationPath, true, tree);
			}
		}
		else if (kind > 0) {
			if (destinationPath == ".")
				destinationPath = parts.name;
			else
				destinationPath += this.slash + parts.name;
			switch (parts.extension) {
			case ".xml":
				if (this.isKPR(sourcePath))
					tree.xmlPaths.push({
						sourcePath: sourcePath,
						destinationPath: destinationPath,
					});
				else
					tree.otherPaths.push({
						sourcePath: sourcePath,
						destinationPath: destinationPath + parts.extension,
					});
				break;
			case ".js":
				tree.jsPaths.push({
					sourcePath: sourcePath,
					destinationPath: destinationPath,
				});
				break;
			default:
				tree.otherPaths.push({
					sourcePath: sourcePath,
					destinationPath: destinationPath + parts.extension,
				});
				break;
			}
		}
	}
	processMakefile(manifestTree, makefileTree, path, separate) {
		if (this.verbose)
			this.report("# Loading '" + path + "'...");
		var buffer = FS.readFileSync(path);
		var makefile = MAKEFILE.parse(buffer, path);
		var currentDirectory = this.currentDirectory;
		var inputDirectories = this.inputDirectories;
		try {
			var directory = this.currentDirectory = makefileTree.directory;
			makefileTree.separate = separate;
			this.inputDirectories = [
				this.tmpPath,
				this.patchesPath,
				directory
			];
			var sourcesPath = this.resolveDirectoryPath(this.joinPath({ directory: directory, name: "sources" }));
			if (sourcesPath)
				this.inputDirectories.push(sourcesPath);
			if (!("cIncludes" in makefileTree))
				makefileTree.cIncludes = [];
			if (!("cOptions" in makefileTree))
				makefileTree.cOptions = [];
			if (!("headers" in makefileTree))
				makefileTree.headers = [];
			if (!("libraries" in makefileTree))
				makefileTree.libraries = [];
			if (!("sources" in makefileTree))
				makefileTree.sources = [];
			makefile.configure(this, manifestTree, makefileTree);
			if (this.errorCount)
				throw new Error("" + this.errorCount + " error(s)!");
			this.processPackages(path, manifestTree, makefileTree);
			manifestTree.makefiles.push(makefileTree);
		}
		finally {
			this.inputDirectories = inputDirectories;
			this.currentDirectory = currentDirectory;
		}
	}
	processPackages(path, manifestTree, makefileTree) {
		var parts = this.splitPath(path);
		parts.extension = ".xs";
		makefileTree.xs = this.resolveFilePath(this.joinPath(parts));
		if (makefileTree.xs) {
			var index = this.packages.length;
			var _package = this.loadPackage(makefileTree.xs);
			if (_package) {
				_package.target(this);
				var count = this.packages.length;
				for (; index < count; index++) {
					path = this.packages[index].__xs__path;
					this.insertUnique(manifestTree.xsSources, path);
					parts = this.splitPath(this.packages[index].__xs__path);
					this.insertUnique(manifestTree.xsIncludes, parts.directory);
					parts.extension = ".c";
					path = this.resolveFilePath(this.joinPath(parts));
					if (path)
						this.insertUnique(makefileTree.sources, path);
				}
				if (!_package.items.length) {
					makefileTree.xs = null;
					this.packages.length = index;
				}
			}
		}
	}
	resolveDirectoryPath(path) {
		return super.resolveDirectoryPath(this.resolveVariable(path));
	}
	resolveFilePath(path) {
		return super.resolveFilePath(this.resolveVariable(path));
	}
	resolvePath(path) {
		return super.resolvePath(this.resolveVariable(path));
	}
	resolveProperties(properties) {
		for (let name in properties) {
			let value = properties[name];
			if (typeof value == "string")
				properties[name] = this.resolveVariable(value);
			else if (value instanceof Array)
				properties[name] = value.map(item => this.resolveVariable(item));
		}
	}
	resolveVariable(value) {
		return value.replace(/\$\(([^\)]+)\)/g, (offset, value) => {
			if (value in this.environment)
				return this.environment[value];
			return process.getenv(value);
		});
	}
	run() {
		var path = this.manifestPath;
		var parts = this.splitPath(path);
		var buffer = FS.readFileSync(path);
		var manifestTree;
		if (parts.extension == ".json") {
			manifestTree = JSON.parse(buffer);
			this.filterManifest(manifestTree);
		}
		else {
			var manifest = MANIFEST.parse(buffer, path);
			manifestTree =  {
				build: {},
				environment: {},
				extensions: {},
				fonts: {},
				info: {},
				resources: {},
				separate: {},
			}
			manifest.configure(this, manifestTree);
			if (this.errorCount)
				throw new Error("" + this.errorCount + " error(s)!");
		}

		manifestTree.cmakefiles = [];
		manifestTree.makefiles = [];
		manifestTree.libraries = [];
		manifestTree.xsIncludes = [];
		manifestTree.xsOptions = [];
		manifestTree.xsSources = [];
		
		manifestTree.directoryPaths = [];
		manifestTree.xmlPaths = [];
		manifestTree.jsPaths = [];
		manifestTree.otherPaths = [];
		
		if (this.application)
			manifestTree.environment.NAME = this.application;
		else if (manifestTree.environment.NAME)
			this.application = manifestTree.environment.NAME;
		else {
			this.application = this.splitPath(parts.directory).name;
			manifestTree.environment.NAME = this.splitPath(parts.directory).name;
		}
		
		if (!("NAMESPACE" in manifestTree.environment))
			manifestTree.environment.NAMESPACE = parts.directory.toLowerCase().split(this.slash).join(".");
		if (!("VERSION" in manifestTree.environment))
			manifestTree.environment.VERSION = "1.0";

		manifestTree.application = this.application;
		
		if (!this.binPath)
			this.binPath = this.createDirectories(this.outputPath, "bin", this.platform != "mac" && !this.cmake)
		if (!this.tmpPath)
			this.tmpPath = this.createDirectories(this.outputPath, "tmp", true)
		this.generateFskPlatform();

		parts = { directory: this.makePath, name: "FskPlatform", extension: ".mk" };
		this.processMakefile(manifestTree, parts, this.joinPath(parts), false);
		parts = { directory: this.patchesPath, name: "FskCore", extension: ".mk" };
		this.processMakefile(manifestTree, parts, this.joinPath(parts), false);
		let extensions = manifestTree.extensions;
		for (let name in extensions) {
			let path = extensions[name];
			if (path) {
				path = this.resolveFilePath(path);
				let parts = tool.splitPath(path)
				if (parts.extension && parts.extension == ".txt") {
					manifestTree.cmakefiles.push({directory: parts.directory, name: name});
				} else {
					if (path)
						this.processMakefile(manifestTree, this.splitPath(path), path, false);
					else
						tool.reportError(null, null, extensions[name] + ": directory or file not found");
				}
			}
		}
		let separate = manifestTree.separate;
		for (let name in separate) {
			let path = separate[name];
			if (path) {
				path = this.resolveFilePath(path);
				if (path)
					this.processMakefile(manifestTree, this.splitPath(path), path, true);
				else
					tool.reportError(null, null, separate[name] + ": directory or file not found");
			}
		}
		this.excludes = {
			[this.manifestPath]: true
		};
		let flags = {};
		let includes = {};
		let resources = manifestTree.resources;
		for (let name in resources) {
			let value = resources[name];
			let flag = name == "~";
			if (!flag)
				this.insertUniqueDirectory(manifestTree.directoryPaths, name);
			for (let item of value) {
				if (item.endsWith("*")) {
					path = this.resolveDirectoryPath(item.slice(0, -2));
					if (path)
						flags[path] = false;
					else
						tool.reportError(null, null, item + ": directory not found");
				}
				else {
					path = this.resolvePath(item);
					if (path)
						flags[path] = true;
					else if (!flag)
						tool.reportError(null, null, item + ": directory or file not found");
				}
				if (flag && path)
					this.excludes[path] = true;
				else {
					includes[path] = name;
				}
			}
		}
		for (let path in includes) {
			this.processAction(path, includes[path], flags[path], manifestTree);
		}
		if (this.errorCount)
			throw new Error("" + this.errorCount + " error(s)!");
		
		try {
			if (this.cmake) {
				var Manifest = require("cmake/" + this.platform);
				var manifest = new Manifest(manifestTree);
			} else {
				var Manifest = require("make/" + this.platform);
				var manifest = new Manifest(manifestTree);
			}
		} catch (e) {
			throw new Error(`-p '${this.platform}': unsupported to build on this system`);
		}
		manifest.generate(this, this.tmpPath, this.binPath);
		if (this.cmake) {
			tool.report("Generating project...");
			var command = `cmake -H${this.tmpPath} -B${this.tmpPath} -DCMAKE_BUILD_TYPE=`;
			command += this.debug ? "Debug" : "Release";
			if (this.ide) {
				if (this.platform == "ios" || this.platform == "mac")
					command += " -GXcode";
			} else {
				if (this.cmakeGenerator)
					command += ` -G"${this.cmakeGenerator}"`;
			}
			tool.report(`${command}`)
			var output = tool.execute(command);
			tool.report(output.trim());
			if (this.ide) {
				tool.report("Opening the IDE...");
				if (this.platform == "ios" || this.platform == "mac")
					process.then("open", this.tmpPath + this.slash + "fsk.xcodeproj");
				else if (this.platform == "win")
					process.then("cmd.exe", "/c", this.tmpPath + this.slash + "fsk.sln");
				return;
			}
		}
		if (this.make) {
			if (this.cmake) {
				tool.report(`cmake --build ${this.tmpPath} --config ${this.debug ? "Debug" : "Release"}`);
				process.then("cmake", "--build", this.tmpPath, "--config", this.debug ? "Debug" : "Release");
			} else {
				if (this.windows)
					process.then("nmake", "/nologo", "/f", this.tmpPath + "\\makefile");
				else
					process.then("make", "-f", this.tmpPath + "/makefile");
			}
		}
	}
}

var tool = new Tool(process.execArgv());
tool.run();
