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

import PACKAGE from "packageGrammar";

class OutputFile {
	constructor(path, tool) {
		this.fd = FS.openSync(path, "w+");
		this.tabCount = 0;
		this.tool = tool;
		this.__xs__path = null;
	}
	CR(delta = 0) {
		this.write("\n");
		this.tabCount += delta;
		var c = this.tabCount;
		while (c > 0) {
			this.write("\t");
			c--;
		}
	}
	SP() {
		this.write(" ");
	}
	close() {
		FS.closeSync(this.fd);
		this.fd = -1;
	}
	write(s) {
		FS.writeSync(this.fd, s);
	}
	writeName(s) {
		s = s.replace(".", "\\u002E");
		s = s.replace(" ", "\\u0020");
		FS.writeSync(this.fd, s);
	}
	writePathLine(path, line) {
		this.write("\n//@line ");
		this.write(line);
		if (this.__xs__path != path) {
			this.__xs__path = path;
			this.write(" \"");
			this.write(path);
			this.write("\"");
		}
	}
}

class Tool extends TOOL {
	constructor(argv) {
		super(argv);
		this.debug = false;
		this.grammar = false;
		this.inputDirectories = [];
		this.kind = "module";
		this.module = null;
		this.outputDirectory = null;
		this.packages = [];
		this.path = null;
		this.targets = new Set;
		this.verbose = false;
		var name, path;
		var argc = argv.length;
		for (var argi = 1; argi < argc; argi++) {
			switch (argv[argi]) {
			case "-b":
			case "-c":
			case "-xsID":
				// compatibility with xsc
				break;
			case "-d":
				this.debug = true;
				break;
			case "-i":
				argi++;	
				if (argi >= argc)
					throw new Error("-i: no directory!");
				name = argv[argi];
				path = this.resolveDirectoryPath(name);
				if (!path)
					throw new Error("-i '" + name + "': directory not found!");
				this.inputDirectories.push(path);
				break;
			case "-m":
				argi++;	
				this.module = argv[argi];
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
				this.outputDirectory = path;
				break;
			case "-p":
				this.kind = "program";
				break;
			case "-t":
				argi++;	
				if (argi >= argc)
					throw new Error("-t: no target!");
				name = argv[argi];
				this.targets.add(name);
				break;
			case "-v":
				this.verbose = true;
				break;
			default:
				name = argv[argi];
				if (this.path)
					throw new Error("'" + name + "': too many files!");
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("'" + name + "': file not found!");
				this.path = path;
				var parts = this.splitPath(path);
				this.inputDirectories.push(parts.directory);
				break;
			}
		}
		if (!this.outputDirectory)
			this.outputDirectory = this.resolveDirectoryPath(".");
		if (!this.path)
			throw new Error("no file");
	}
	addInputDirectory(directory) {
		this.inputDirectories.push(directory);
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
		var regexp = /([\w_]+)/g;
		var target = target.replace(regexp, name => ((target == "true") || this.targets.has(target)) ? "true" : "false");
		return eval(target);
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
	run() {
		var path = this.path;
		var parts = this.splitPath(this.path);
		var _package = this.loadPackage(path);
		_package.__xs__path = path;
		_package.target(this, "");
		if (this.errorCount)
			throw new Error("" + this.errorCount + " error(s)!");
		_package.crossReference(this, null, "", false);
		if (this.errorCount)
			throw new Error("" + this.errorCount + " error(s)!");
		parts.directory = this.outputDirectory;
		parts.extension = ".js";
		path = this.joinPath(parts);
		var file = new OutputFile(path, this);
		file.write("/* XS2JS GENERATED FILE; DO NOT EDIT! */");
		file.CR(0);
		_package.print(file);
		if (this.grammar) {
			file.CR();
			PACKAGE.printGrammar(file, _package);
		}
		if (this.module) {
			file.CR();
			file.write("export default ");
			file.write(this.module);
			file.write(";");
		}
		file.CR();
		file.close();
	}
}

try {
	var tool = new Tool(process.execArgv());
	tool.run();
}
catch(e) {
	console.log("### " + e.message);
}
