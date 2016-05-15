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

		this.m32 = false;
		this.m64 = false;
		this.application = null;
		this.binPath = null;
		this.clean = false;
		this.cleanall = false;
		this.cmake = false;
		this.cmakeGenerator = null;
		this.cmakeGenerate = true;
		this.cmakeFlags = [];
		this.debug = false;
		this.docker = null;
		this.errorCount = 0;		
		this.help = false;
		this.helpItems = {};
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

		this.helpItems.nameMapping = {
			"buildOptions": "Build Options",
			"debugOptions": "Debug Options",
			"generalOptions": "General Options",
			"iosOptions": "iOS Options",
			"outputOptions": "Output Options"
		};

		this.appendHelpItems("buildOptions", "-64", "64-bit build on supported platforms");
		this.appendHelpItems("buildOptions", "-32", "32-bit build on supported platforms");
		this.appendHelpItems("generalOptions", "-a", "application name");
		this.appendHelpItems("outputOptions", "-b", "bin directory");
		this.appendHelpItems("debugOptions", "-c", "debug or release configuration");
		this.appendHelpItems("buildOptions", "-clean", "clean the current project and platform");
		this.appendHelpItems("buildOptions", "-cleanall", "clean all projects");
		this.appendHelpItems("iosOptions", "-csi", "code signing identity to use on iOS");
		this.appendHelpItems("debugOptions", "-d", "debug build (same as \"-c debug\")");
		this.appendHelpItems("buildOptions", "-g", "sets CMake generator (build tool used)");
		this.appendHelpItems("generalOptions", "-h", "display this message");
		this.appendHelpItems("debugOptions", "-i", "enable instrumentation");
		this.appendHelpItems("buildOptions", "-I", "use the CMake generated IDE (sets -x)");
		this.appendHelpItems("debugOptions", "-l", "debug memory leaks");
		this.appendHelpItems("buildOptions", "-m", "build the application after generating the make files");
		this.appendHelpItems("buildOptions", "-ng", "Create CMakeLists.txt but do not generate a project");
		this.appendHelpItems("outputOptions", "-o", "output directory to place default bin and tmp directories");
		this.appendHelpItems("generalOptions", "-p", "platform name");
		this.appendHelpItems("iosOptions", "-pp", "provisioning profile for iOS");
		this.appendHelpItems("outputOptions", "-t", "tmp directory");
		this.appendHelpItems("outputOptions", "-v", "verbose");
		this.appendHelpItems("buildOptions", "-x", "use CMake build");
		this.appendHelpItems("debugOptions", "-X", "support xsdebug");
		
		var name, path;
		var argc = argv.length;
		if (argc == 1)
			this.help = true;
		for (var argi = 1; argi < argc; argi++) {
			var option = argv[argi];
			switch (option) {
			case '-32':
				this.m32 = true;
				break;
			case '-64':
				this.m64 = true;
				break;
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
			case "-clean":
				this.clean = true;
				break;
			case "-cleanall":
				this.cleanall = true;
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
			case "-docker":
				argi++;
				if (argi >= argc)
					throw new Error("--docker: no image name!");
				this.docker = { image: argv[argi], args: argv };
				break;
			case "-D":
				argi++;
				if (argi >= argc)
					throw new Error("-D: No option given!");
				this.cmakeFlags.push(argv[argi]);
				break;
			case "-g":
				argi++;
				if (argi >= argc)
					throw new Error("-g: no generator!");
				this.cmakeGenerator = argv[argi];
				break;
			case "-h":
				this.help = true;
				break;
			case "-help":
				this.help = true;
				break;
			case "--help":
				this.help = true;
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
			case "-ng":
				this.cmakeGenerate = false;
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
				else if (!this.cleanall && !this.help)
					throw new Error("no manifest!");
			}
		}
		if (!this.outputPath)
			this.outputPath = this.homePath;
		if (!this.platform) {
			if (this.currentPlatform == "linux")
				this.platform = "linux/gtk";
			else
				this.platform = this.currentPlatform;
		}

		if (this.platform == "android" && this.homePath.indexOf(" ") > 0)
			throw new Error("The Android NDK does not support spaces in it's path. Please move your FSK folder accordingly.");
			
		path = this.resolveDirectoryPath("$(F_HOME)/build/" + this.platform);
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
			if (!value && !this.docker)
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
		if (!skipVariantFlag) {
			if (this.cmake) {
				if (this.debug)
					path += this.slash + "Debug";
				else
					path += this.slash + "Release";
			} else {
				if (this.debug) 
					path += this.slash + "debug";
				else
					path += this.slash + "release";
			}
		}
		FS.mkdirSync(path);
		if (name == "tmp" && this.platform == "android")
			path += this.slash + this.application.replace(/\s/g, "-").toLowerCase();
		else
			path += this.slash + this.application;
		if (flag)
			FS.mkdirSync(path);
		return path;
	}
	escapeMakePath(path) {
		return path.replace(/ /i, '\\ ');
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
		if (!("sources" in manifest))
			manifest.sources = [];
		if (!("xsdebug" in manifest))
			manifest.xsdebug = {enabled: false};
		if (!("xsprofile" in manifest))
			manifest.xsprofile = {enabled: false};
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
		this.resolveProperties(manifest.sources);
	}
	filterPlatform(manifest, platform) {
		this.filterProperties(manifest, platform, "build");
		this.filterProperties(manifest, platform, "environment");
		this.filterProperties(manifest, platform, "extensions");
		this.filterProperties(manifest, platform, "fonts");
		this.filterProperties(manifest, platform, "info");
		this.filterProperties(manifest, platform, "resources");
		this.filterProperties(manifest, platform, "separate");
		this.filterArray(manifest, platform, "sources");
		this.filterProperties(manifest, platform, "xsdebug");
		this.filterProperties(manifest, platform, "xsprofile");
	}
	filterArray(manifest, platform, name) {
		if (name in platform) {
			let targets = manifest[name];
			let sources = platform[name];
			manifest[name] = targets.concat(sources);
		}
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
					destinationPath = parts.name + parts.extension;
				else
					destinationPath += this.slash + parts.name + parts.extension;
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
			if (tree.sources.find(path => {
				let regex = new RegExp(path);
				let result = regex.exec(sourcePath);
				return result
			})) {
				tree.otherPaths.push({
					sourcePath: sourcePath,
					destinationPath: destinationPath + parts.extension,
				});
			}
			else {
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
				case ".mk":
					parts.jsDestinationPath = destinationPath;
					this.processMakefile(tree, parts, sourcePath, false);
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
			if (!this.cmake)
				this.inputDirectories.push(this.patchesPath, this.xs6Includes);
			var sourcesPath = this.resolveDirectoryPath(this.joinPath({ directory: directory, name: "sources" }));
			if (sourcesPath)
				this.inputDirectories.push(sourcesPath);
			if (!("cIncludes" in makefileTree))
				makefileTree.cIncludes = [];
			if (!("cOptions" in makefileTree))
				makefileTree.cOptions = [];
			if (!("headers" in makefileTree))
				makefileTree.headers = [];
			if (!("jsDestinationPath" in makefileTree))
				makefileTree.destinationPath = null;
			if (!("jsSourcePath" in makefileTree))
				makefileTree.destinationPath = null;
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
			return;
		}
		parts.extension = ".js";
		makefileTree.jsSourcePath = this.resolveFilePath(this.joinPath(parts));
		if (makefileTree.jsSourcePath) {
			parts.extension = ".c";
			path = this.resolveFilePath(this.joinPath(parts));
			if (path)
				this.insertUnique(makefileTree.sources, path);
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
		if (this.help) {
			this.displayHelp(tool);
			return;
		}

		if (this.cleanall) {
			this.deleteDirectory(this.outputPath + this.slash + "bin");
			this.deleteDirectory(this.outputPath + this.slash + "tmp");
			return;
		}

		if (this.docker != null) {
			let args = this.docker.args;
			let image = this.docker.image;
			let command = ["docker", "run", "--rm", "-e", "F_HOME=" + this.homePath, "-v", this.homePath + ":" + this.homePath, "-it", image];
			let dockerIndex = this.docker.args.indexOf("-docker");
			args.splice(dockerIndex, 2);
			args.splice(0, 1);
			command = command.concat(args);
			this.report(command.join(' '));
			process.then.apply(this, command);
			return;
		}

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
				sources: [],
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
			this.binPath = this.createDirectories(this.outputPath, "bin", this.platform != "mac");
		if (!this.tmpPath)
			this.tmpPath = this.createDirectories(this.outputPath, "tmp", true, this.cmake);

		if (this.clean) {
			if (this.platform == "mac")
				this.deleteDirectory(this.binPath + ".app");
			else
				this.deleteDirectory(this.binPath);

			this.deleteDirectory(this.tmpPath);

			this.debug = this.debug ? false : true;

			let otherBinPath = this.createDirectories(this.outputPath, "bin", this.platform != "mac");
			if (this.platform == "mac")
				this.deleteDirectory(otherBinPath + ".app");
			else
				this.deleteDirectory(otherBinPath);

			if (!this.cmake) {
				let otherTmpPath = this.createDirectories(this.outputPath, "tmp", true, this.cmake);
				this.deleteDirectory(otherTmpPath);
			}

			return;
		}

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
			if (this.cmake)
				var Manifest = require("cmake/" + this.platform);
			else
				var Manifest = require("make/" + this.platform);
			var manifest = new Manifest(manifestTree);
		} catch (e) {
			throw new Error(`-p '${this.platform}': unsupported to build on this system`);
		}
		manifest.generate(this, this.tmpPath, this.binPath);
		if (this.make)
			manifest.make(tool);
	}
	deleteDirectory(path) {
		if (this.verbose)
			this.report(`Deleting ${path}`);
		if (FS.existsSync(path) < 0) {
			var contents = FS.readDirSync(path);
			for (let item of contents)
				this.deleteDirectory(path + this.slash + item);
			FS.deleteDirectory(path);
		} else {
			FS.deleteFile(path);
		}
	}
	appendHelpItems(array, option, message) {
		if (!this.helpItems.commands)
			this.helpItems.commands = {};
		if (!this.helpItems.commands[array])
			this.helpItems.commands[array] = [];
		this.helpItems.commands[array].push([option, message]);
	}
	displayHelp(tool) {
		tool.report(`\nusage: kprconfig6 [options] [manifest]\n`);
		for (let name in this.helpItems.nameMapping) {
			let array = this.helpItems.commands[name];
			if (array) {
				tool.report(this.helpItems.nameMapping[name] + ":");
				for (let item of array) {
					let width = 12
						let command = item[0];
					for (let i = 0; i < width - item[0].length; i++)
						command += " ";
					command += item[1];
					tool.report(`  ${command}`);
				}
			}
		}
		tool.report("");
	}
}

var tool = new Tool(process.execArgv());
tool.run();
