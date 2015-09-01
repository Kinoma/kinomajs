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
import * as FS from "fs";
import * as MAKE from "make";

function toCMakePath(tool, path) {
	var homePath = tool.resolveDirectoryPath(tool.homePath);
	path =  path.replace(homePath, "${F_HOME}");
	if (tool.platform == "win")
		path =  path.replace(/\\/g, "/")
	return fixVariable(tool, path);
}

function fixVariable(tool, item) {
	return item.replace(/\$\(([^\)]*)\)/g, "${$1}");
}

function fixDefinitions(tool, item) {
	item = fixVariable(tool, item);
	let matches = item.match(/^"(.*)"/);
	if (matches)
		return "\"'" + matches[1] + "'\"";
	else
		return item;
}

class File {
	constructor(path) {
		this.fd = FS.openSync(path, "w");
		this.slash = "/";
	}
	close() {
		FS.closeSync(this.fd);
		delete this.fd;
	}
	line(...strings) {
		for (var string of strings)
			this.write(string);
		this.write("\n");
	}
	write(string) {
		FS.writeSync(this.fd, string);
	}
}

export class Makefile extends MAKE.Makefile {
	generateRule(tool, file, path) {
		file.line("LIST(APPEND ", this.name, "_SOURCES ", toCMakePath(tool, path), ")");
		var parts = tool.splitPath(path);
		if (parts.name.indexOf("neon") >= 0)
			file.line("SET_SOURCE_FILES_PROPERTIES(", toCMakePath(tool, path), " PROPERTIES COMPILE_FLAGS ${AS_NEON_OPTIONS})");
		if (parts.name.indexOf("wmmx") >= 0)
			file.line("SET_SOURCE_FILES_PROPERTIES(", toCMakePath(tool, path), " PROPERTIES COMPILE_FLAGS ${AS__OPTIONS})");	//@
	}
	generateRules(tool, file, path) {
		file.line("ADD_LIBRARY(", this.name, " STATIC ${", this.name, "_SOURCES})");
		file.line("ADD_DEPENDENCIES(", this.name, " FskManifest.xsa)");
		file.line("LIST(APPEND OBJECTS ", this.name, ")");
		file.line("SET(OBJECTS ${OBJECTS} PARENT_SCOPE)");
	}
	processCOptions(tool, file, options, variant) {
		var variantSuffix = variant ? "_" + variant.toUpperCase() : "";
		if (options && options.length) {
			for (let item of options) {
				var regexpStr = /\$([^)]*\))/;
				var defs = [];
				var incs = [];
				var copts = [];
				var matches = item.match(/"?([-\/].)/);
				if (matches) {
					switch (matches[1]) {
						case "-D":
							defs.push(item);
							break;
						case "/D":
							defs.push(item);
							break;
						case "-U":
							defs.push(item);
							break;
						case "/U":
							defs.push(item);
							break;
						case "-I":
							incs.push(item.match(/"?-I(.*)/)[1]);
							break;
						case "/I":
							incs.push(item.match(/"?\/I(.*)/)[1]);
							break;
						default:
							copts.push(item);
					}
				} else
					copts.push(item);

				for (let item of incs)
					file.line("INCLUDE_DIRECTORIES(", toCMakePath(tool, item), ")");
				for (let item of defs)
					if (variant)
						file.line(`SET(CMAKE_C_FLAGS${variantSuffix} "\${CMAKE_C_FLAGS${variantSuffix}} ${fixDefinitions(tool, item)}")`);
					else
						file.line("ADD_DEFINITIONS(", fixDefinitions(tool, item), ")");
				for (let item of copts)
					file.line(`SET(CMAKE_C_FLAGS${variantSuffix} "\${CMAKE_C_FLAGS${variantSuffix}} ${fixVariable(tool, item)}")`);
			}
		}
	}
	generateVariables(tool, file, path) {
		this.processCOptions(tool, file, this.cOptions);
		this.processCOptions(tool, file, this.cOptionsDebug, "Debug");
		this.processCOptions(tool, file, this.cOptionsRelease, "Release");
		if (this.cIncludes.length) {
			for (let item of this.cIncludes)
				file.line("INCLUDE_DIRECTORIES(", toCMakePath(tool, item), ")");
		}
		file.line("SET(CMAKE_CXX_FLAGS \"${CMAKE_C_FLAGS}\")");
		file.line("SET(CMAKE_CXX_FLAGS_DEBUG \"${CMAKE_C_FLAGS_DEBUG}\")");
		file.line("SET(CMAKE_CXX_FLAGS_RELEASE \"${CMAKE_C_FLAGS_RELEASE}\")\n");
		if (this.headers.length) {
			for (let item of this.headers)
				file.line("LIST(APPEND ", this.name, "_HEADERS ", toCMakePath(tool, item), ")");
			file.line();
		}
		if (this.sources.length) {
			for (let item of this.sources)
				this.generateRule(tool, file, item)
		}
		if (this.separate) {
			for (let library of this.libraries) {
				file.line("LIST(APPEND ", this.name, "_LIBRARIES ", library, ")");
			}
		}
	}
}

export class Manifest extends MAKE.Manifest {
	get Makefile() {
		return Makefile;
	}
	generate(tool, tmp, bin) {
		this.generateDirectories(tool, tmp);
		this.generateC(tool, tmp);
		this.generateXS(tool, tmp);
		this.generateMAKE(tool, tmp, bin);
	}
	generateMAKE(tool, tmp, bin) {
		var path = tool.joinPath({ directory: tmp, name: "CMakeLists", extension: ".txt" });
		var file = new File(path);

		file.line("# WARNING: This file is automatically generated by config. Do not edit. #\n");

		file.line("CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8)\n");

		file.line("FILE(TO_CMAKE_PATH $ENV{F_HOME} F_HOME)");

		var split = tool.platform.split("/");
		file.line("SET(PLATFORM ", split[0], ")");
		if (split[1])
			file.line("SET(SUBPLATFORM ", split[1], ")");
		if (!tool.debug)
			file.line("SET(RELEASE true)");
		file.line();

		file.line("SET(BIN_DIR ", toCMakePath(tool, bin), ")");
		file.line("SET(TMP_DIR ", toCMakePath(tool, tmp), ")");
		file.write("SET(SUPPORT_XS_DEBUG ");
		file.write((tool.debug || this.tree.xsdebug.enable) ? 1 : 0);
		file.write(")\n\n");

		file.line("LIST(APPEND CMAKE_MODULE_PATH ${F_HOME}/xs6/cmake/modules)");
		file.line("INCLUDE(XS6)");
		file.line("INCLUDE(Kinoma)");
		file.line("INCLUDE(" + tool.platform + " OPTIONAL)\n");
		file.line();

		this.generatePlatformVariables(tool, file, tmp, bin);

		file.line("PROJECT(fsk)\n");

		file.line("IF(NOT DEFINED CMAKE_MACOSX_RPATH)");
		file.line("\tSET(CMAKE_MACOSX_RPATH 0)");
		file.line("ENDIF()\n");

		file.line("IF(NOT CMAKE_BUILD_TYPE)");
		file.line("\tMESSAGE(STATUS \"Setting build type to 'Release' as none was specified.\")");
		file.line(`\tSET(CMAKE_BUILD_TYPE ${tool.debug ? "Debug" : "Release"} CACHE STRING "Choose the type of build." FORCE)`);
		file.line("\tSET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS \"Debug\" \"Release\")");
		file.line("ENDIF()\n");

		file.line("IF(CMAKE_CONFIGURATION_TYPES)");
		file.write("\tSET(CMAKE_CONFIGURATION_TYPES");
		file.write(this.debug? ' "Debug"' : ' "Release"'); 
		file.write(this.debug? ' "Release"' : ' "Debug"'); 
		file.line(" CACHE STRING \"Reset the configurations to what we need\" FORCE)");
		file.line("ENDIF()\n");

		this.generateXSVariables(tool, file);
		this.generateResourcesVariables(tool, file);

		file.line("INCLUDE_DIRECTORIES(${F_HOME}/xs6/includes)");
		file.line("INCLUDE_DIRECTORIES(${TMP_DIR})");
		file.line("INCLUDE_DIRECTORIES(${TMP_DIR}/src)");
		file.line("INCLUDE_DIRECTORIES(${FREETYPE_DIR}/include)");
		file.write("SET(CMAKE_C_FLAGS \"${CMAKE_C_FLAGS} ");
		if (tool.windows)
			file.line("/Fd${TMP_DIR}\\\\fsk.pdb /DXS6=1\")");
		else
			file.line("-DXS6=1\")");

		var FskManifest = this.makefiles[0];
		this.makefiles.splice(0, 1);
		FskManifest.generateVariables(tool, file, tmp);

		for (let makefile of this.makefiles)
			if (!makefile.separate)
				file.line("ADD_SUBDIRECTORY(", makefile.name, ")");
		file.line();

		for (let item of this.tree.libraries) {
			var parts = item.split(" ");
			if (parts[0] === "-framework")
				file.line("LOCAL_FIND_LIBRARY(LIBNAME ", parts[1], " LIST LIBRARIES)");
			else {
				if (tool.platform == "win") {
					if (item.indexOf("/") >= 0)
						file.line("SET(CMAKE_EXE_LINKER_FLAGS \"${CMAKE_EXE_LINKER_FLAGS} ", toCMakePath(tool, item), "\")");
					else
						file.line("LIST(APPEND LIBRARIES ", toCMakePath(tool, item), ")");
				} else {
					file.line("LIST(APPEND LIBRARIES ", toCMakePath(tool, item), ")");
				}
			}
		}
		file.line()

		file.line("SET(SEPARATE");
		for (let makefile of this.makefiles)
			if (makefile.separate)
				file.line("\t", makefile.name);
		file.line("\t)\n");

		this.generateManifestRules(tool, file);
		this.generateResourcesRules(tool, file);
		this.generateXSRules(tool, file);
		for (let makefile of this.makefiles) {
			var makePath = tool.tmpPath + "/" + makefile.name + "/CMakeLists.txt";
			var makeFile = new File(makePath);
			makeFile.line("# WARNING: This file is automatically generated by config. Do not edit. #\n");
			makefile.generateVariables(tool, makeFile, tmp);
			if (makefile.sources.length)
				makefile.generateRules(tool, makeFile, tmp);
			makeFile.line("# vim: ft=cmake");
			makeFile.close();
		}
		file.line();

		file.line("LIST(APPEND SOURCES ${TMP_DIR}/FskManifest.c)");
		file.line("LIST(APPEND SOURCES ${TMP_DIR}/src/FskManifest.xs.c)");
		file.line("SET_SOURCE_FILES_PROPERTIES(${TMP_DIR}/src/FskManifest.xs.c ${TMP_DIR}/src/FskManifest.xs.h PROPERTIES GENERATED TRUE)");

		this.generateTargetRules(tool, file);

		for (let makefile of this.makefiles)
			if (makefile.separate)
				file.line("ADD_SUBDIRECTORY(", makefile.name, ")");

		file.line();
		file.line("# vim: ft=cmake");

		file.close();
	}
	generateManifestRules(tool, file) {
	}
	generateResourcesVariables(tool, file) {
		file.line("SET(MODULES");
		file.line("\t${TMP_DIR}/FskManifest.xsb");
		for (let item of this.tree.xmlPaths)
			file.line("\t${TMP_DIR}/", toCMakePath(tool, item.destinationPath), ".xsb");
		for (let item of this.tree.jsPaths)
			file.line("\t${TMP_DIR}/", toCMakePath(tool, item.destinationPath), ".xsb");
		file.line("\t)\n");
	}
	generateTargetRules(tool, file) {
		file.write(this.getTargetRules(tool));
	}
	generateXSRules(tool, file) {
		file.line("XS2JS(SOURCE ${TMP_DIR}/FskManifest.xs DESTINATION ${TMP_DIR} OPTIONS ${XSC_OPTIONS})");
		file.line("XSC(SOURCE_FILE ${TMP_DIR}/FskManifest.js DESTINATION ${TMP_DIR} OPTIONS -c -d -e -p)");
		file.line("XSL(NAME FskManifest SOURCES ${MODULES} TMP ${TMP_DIR} DESTINATION ${APP_DIR} SRC_DIR ${TMP_DIR}/src)");
	}
	generateXSVariables(tool, file) {
		file.line("SET(XSC_OPTIONS");
		file.line("\t-b");
		file.line("\t$<$<CONFIG:Debug>:-d>");
		for (let item of this.xsIncludes)
			file.line("\t-i ", toCMakePath(tool, item));
		file.line("\t$<$<CONFIG:Debug>:-t> $<$<CONFIG:Debug>:debug>");
		file.line("\t-t KPR_CONFIG");
		file.line("\t-t XS6");
		for (let item of this.xsOptions)
			file.line("\t", item);
		file.line("\t)\n");
		file.line("set(XSC_PACKAGES");
		for (let item of this.xsSources)
			file.line("\t", toCMakePath(tool, item));
		file.line("\t)\n");
	}
	generatePlatformVariables(tool, file, tmp, bin) {
		let variables = this.getPlatformVariables(tool, tmp, bin);
		for (let name in variables) {
			let value = variables[name];
			if (typeof value == "string") {
				value = toCMakePath(tool, value);
			}
			file.line("SET(", name, " ", value, ")");
		}
		var languages = this.getPlatformLanguages();
		if (languages) {
			file.line();
			for (let language of languages)
				file.line("ENABLE_LANGUAGE(", language, ")");
			file.line();
		}
	}
	generateResourcesRules(tool, file) {
		for (let item of this.tree.xmlPaths) {
			let parts = tool.splitPath(item.destinationPath);
			let sourcePath = toCMakePath(tool, item.sourcePath);
			let destinationPath = toCMakePath(tool, item.destinationPath);
			let directory = toCMakePath(tool, parts.directory);
			file.line("KPR2JS(SOURCE ", sourcePath, " DESTINATION ${TMP_DIR}/", directory, ")");
			file.line("XSC(SOURCE_FILE ${TMP_DIR}/", directory, "/", parts.name, ".js DESTINATION ${TMP_DIR}/", directory, ")");
		}
		file.line();
		for (let item of this.tree.jsPaths) {
			let parts = tool.splitPath(item.destinationPath);
			let sourcePath = toCMakePath(tool, item.sourcePath);
			let destinationPath = toCMakePath(tool, item.destinationPath);
			let directory = toCMakePath(tool, parts.directory);
			file.line("XSC(SOURCE_FILE ", sourcePath, " DESTINATION ${TMP_DIR}/", directory, ")");
		}
		file.line();
		for (let item of this.tree.otherPaths) {
			let sourcePath = toCMakePath(tool, item.sourcePath);
			let destinationPath = toCMakePath(tool, item.destinationPath);
			file.line("COPY(SOURCE ", sourcePath, " DESTINATION ${APP_DIR}/", destinationPath, ")");
		}
		file.line();
	}
	getPlatformLanguages() {
		return [];
	}
}
