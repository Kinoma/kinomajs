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

import * as FS from "fs";
import * as CMAKE from "cmake";

class Makefile extends CMAKE.Makefile {
	constructor(tree) {
		super(tree);
	}
	generateRules(tool, file, path) {
		file.line(`add_library(${this.name} STATIC \${${this.name}_SOURCES})`);
		file.line(`set_target_properties(${this.name} PROPERTIES STATIC_LIBRARY_FLAGS /LTCG)`);
		file.line(`add_dependencies(${this.name} FskManifest.xsa)`);
		file.line(`list(APPEND OBJECTS ${this.name})`);
		file.line(`set(OBJECTS \${OBJECTS} PARENT_SCOPE)`);
	}
};

class Manifest extends CMAKE.Manifest {
	constructor(tree) {
		super(tree);
	}
	get Makefile() {
		return Makefile;
	}
	getGenerator(tool) {
		let vs = null;
		let VisualStudioVariants = {
			10: "Visual Studio 10 2010",
			11: "Visual Studio 11 2012",
			12: "Visual Studio 12 2013",
			14: "Visual Studio 14 2015"
		};
		for (let key in VisualStudioVariants) {
			let version = key * 10;
			let comntools = process.getenv(`VS${version}COMNTOOLS`);
			if (comntools) {
				vs = VisualStudioVariants[key];
				if (key < 12)
					throw new Error(vs + " is not supported. Please upgrade Visual Studio");
			}
		}

		if (vs) {
			if (tool.m64)
				return `${vs} Win64`;
			else
				return vs;
		} else {
			let envPath = process.getenv("PATH");
			let nmake = false;
			let supported = false;
			for (let dir of envPath.split(";")) {
				let path = tool.joinPath({ directory: dir, name: "nmake.exe"});
				let nmakeFound = FS.existsSync(path);
				if (nmakeFound) {
					nmake = true;
					let version = path.match(/Visual Studio ([0-9]*)/);
					if (version) {
						let key = version[1];
						if (key > 12)
							supported = true;
						else
							throw new Error(vs + " is not supported. Please upgrade Visual Studio");
					}
				}
			}
			if (nmake) {
				if (!supported) {
					tool.report("# WARNING: NMake was found but cannot determine Visual Studio Version.");
					tool.report("# WARNING: This product may not build properly!");
				}
				return "NMake Makefiles";
			}
			else
				throw new Error("Unable to find Visual Studio or NMake");
		}
	}
	getIDEGenerator(tool) {
		let vs =  this.getGenerator(tool);
		if (vs == "NMake Makefiles")
			throw new Error("Unable to determine your Visual Studio version");
		else
			return vs;
	}
	openIDE(tool, path) {
		process.then("cmd.exe", "/c", `${path}${tool.slash}fsk.sln`);
	}
	getPlatformVariables(tool, tmp, bin) {
		var parts = tool.splitPath(tool.manifestPath);
		var resource = parts.directory + "\\win\\resource.rc";
		var path = process.debug ? "$(F_HOME)\\xs6\\bin\\win\\debug" : "$(F_HOME)\\xs6\\bin\\win\\release";
		return {
			APP_DIR: `${tool.outputPath}/bin/${tool.platform}/$<CONFIG>/${tool.application}`,
			TMP_DIR: tmp,
			
			APP_NAME: tool.application,
			RESOURCE: FS.existsSync(resource) ? resource : "$(F_HOME)\\build\\win\\resource.rc",
			// FskPlatform.mk
			BUILD_TMP: tmp,
		};
	}
	getTargetRules(tool) {
		return "BUILD()";
	}
};

export default Manifest;
