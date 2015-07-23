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

import TEMPLATE from "templateGrammar";

class Tool extends TOOL {
	constructor(argv) {
		super(argv);
		this.debug = false;
		this.inputFiles = [];
		this.outputDirectory = null;
		this.verbose = false;
		var name, path;
		var argc = argv.length;
		for (var argi = 1; argi < argc; argi++) {
			switch (argv[argi]) {
			case "-d":
				this.debug = true;
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
			case "-v":
				this.verbose = true;
				break;
			default:
				name = argv[argi];
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("'" + name + "': file not found!");
				this.inputFiles.push(path);
				break;
			}
		}
		if (!this.outputDirectory)
			this.outputDirectory = this.resolveDirectoryPath(".");
		if (!this.inputFiles.length)
			throw new Error("no file");
	}
	escapeQualifiedName(s) {
		s = s.replace("[", "\\u005B");
		s = s.replace("]", "\\u005D");
		s = s.replace(".", "\\u002E");
		s = s.replace(" ", "\\u0020");
		return s;
	}
	getPath(name) {
		var parts = this.base;
		parts.name = name;
		parts.extension = ".xml";
		return this.resolveFilePath(this.joinPath(parts));
	}
	getScript(name) {
		var parts = this.base;
		parts.name = name;
		parts.extension = ".js";
		return this.resolveFilePath(this.joinPath(parts));
	}
	load(path) {
		if (this.verbose)
			this.report("# Loading '" + path + "'...");
		var buffer = FS.readFileSync(path);
		return TEMPLATE.parse(buffer, path);
	}
	run() {
		var paths = this.inputFiles;
		var c = paths.length;
		for (var i = 0; i < c; i++) {
			var path = paths[i];
			this.base = this.splitPath(path);
			var program = this.load(path);
			program.prepare(this);
			if (this.errorCount)
				throw new Error("" + this.errorCount + " error(s)!");
			var code = program.print(this);
			var parts = this.splitPath(path);
			parts.directory = this.outputDirectory;
			parts.extension = ".js";
			var path = this.joinPath(parts);
			FS.writeFileSync(path, code);
		}
	}
}

try {
	var tool = new Tool(process.execArgv());
	tool.run();
}
catch(e) {
	console.log("### " + e.message);
}
