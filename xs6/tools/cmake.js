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
import * as MAKE from "make";
import TEMPLATE from "templateGrammar";

function toCMakePath(tool, path) {
	var homePath = tool.resolveDirectoryPath(tool.homePath);
	path =  path.replace(tool.homePath, "${F_HOME}").replace(/"/g, '');
	path =  path.replace(homePath, "${F_HOME}").replace(/"/g, '');
	if (tool.platform == "win")
		path =  path.replace(/\\/g, "/")
	if (path.indexOf("{F_HOME}/") < 0)
		path = path.replace("{F_HOME}", "{F_HOME}/");
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
	constructor(tree) {
		super(tree);
		this.cmakeOptions = tree.cmakeOptions;
		this.cOptionsDebug = tree.cOptionsDebug;
		this.cOptionsRelease = tree.cOptionsRelease;
	}
	generateRule(tool, file, path) {
		file.line("list(APPEND ", this.name, "_SOURCES ", toCMakePath(tool, path), ")");
		var parts = tool.splitPath(path);
		if (parts.name.indexOf("neon") >= 0)
			file.line("set_source_files_properties(", toCMakePath(tool, path), " PROPERTIES COMPILE_FLAGS ${AS_NEON_OPTIONS})");
		if (parts.name.indexOf("wmmx") >= 0)
			file.line("set_source_files_properties(", toCMakePath(tool, path), " PROPERTIES COMPILE_FLAGS ${AS__OPTIONS})");	//@
	}
	generateRules(tool, file, path) {
		var depends = [];
		for (let name in this.cmakeOptions) {
			var option = this.cmakeOptions[name]
			if (option.build) {
				var path = tool.splitPath(option.build);
				file.line("add_subdirectory(\"", toCMakePath(tool, path.directory), "\" ", name, ")");
				depends.push(name);
			}
		}
		file.line("add_library(", this.name, " STATIC ${", this.name, "_SOURCES})");
		file.write("add_dependencies(" + this.name + " FskManifest.xsa");
		if (depends.length > 0)
			file.write(" " + depends.join(" "));
		file.line(')');
		file.line("list(APPEND OBJECTS ", this.name, ")");
		file.line("set(OBJECTS ${OBJECTS} PARENT_SCOPE)");
		if (depends.length > 0)
			file.line("set(LIBRARIES ${LIBRARIES} PARENT_SCOPE)");
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
							defs.push(item.replace("/D ", "/D"));
							break;
						case "-U":
							defs.push(item);
							break;
						case "/U":
							defs.push(item.replace("/U ", "/U"));
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
					file.line("include_directories(\"", toCMakePath(tool, item), "\")");
				for (let item of defs)
					if (variant)
						file.line("set(CMAKE_C_FLAGS", variantSuffix, " \"${CMAKE_C_FLAGS", variantSuffix, "} ", fixDefinitions(tool, item), "\")");
					else
						file.line("add_definitions(", fixDefinitions(tool, item), ")");
				for (let item of copts)
					file.line("set(CMAKE_C_FLAGS", variantSuffix, " \"${CMAKE_C_FLAGS", variantSuffix, "} ", toCMakePath(tool, item), "\")");
			}
		}
	}
	generateVariables(tool, file, path) {
		this.processCOptions(tool, file, this.cOptions);
		this.processCOptions(tool, file, this.cOptionsDebug, "Debug");
		this.processCOptions(tool, file, this.cOptionsRelease, "Release");

		if (tool.platform == "linux/gtk")
			if (tool.m32)
				file.line("set(CMAKE_C_FLAGS \"${CMAKE_C_FLAGS} -m32\")");

		if (this.cIncludes.length) {
			for (let item of this.cIncludes)
				file.line("include_directories(\"", toCMakePath(tool, item), "\")");
		}
		file.line("set(CMAKE_CXX_FLAGS \"${CMAKE_C_FLAGS}\")");
		file.line("set(CMAKE_CXX_FLAGS_DEBUG \"${CMAKE_C_FLAGS_DEBUG}\")");
		file.line("set(CMAKE_CXX_FLAGS_RELEASE \"${CMAKE_C_FLAGS_RELEASE}\")\n");
		file.line("set(CMAKE_C_FLAGS_RELWITHDEBINFO \"${CMAKE_C_FLAGS_RELEASE}\")\n");
		file.line("set(CMAKE_CXX_FLAGS_RELWITHDEBINFO \"${CMAKE_C_FLAGS_RELEASE}\")\n");
		file.line("fix_flags(FLAGS CMAKE_C_FLAGS)\n");
		if (this.headers.length) {
			for (let item of this.headers)
				file.line("list(APPEND ", this.name, "_HEADERS \"", toCMakePath(tool, item), "\")");
			file.line();
		}
		if (this.sources.length) {
			for (let item of this.sources)
				this.generateRule(tool, file, item)
		}
		if (this.separate) {
			for (let library of this.libraries) {
				file.line("list(APPEND ", this.name, "_LIBRARIES ", library, ")");
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
		this.generateProject(tool);
	}
	generateMAKE(tool, tmp, bin) {
		var path = tool.joinPath({ directory: tmp, name: "CMakeLists", extension: ".txt" });
		var file = new File(path);
		var parts = tool.splitPath(tool.manifestPath);
		this.cmakePrefix = tool.joinPath({ directory: parts.directory, name: "prefix", extension: ".cmake" });
		this.cmakeSuffix = tool.joinPath({ directory: parts.directory, name: "suffix", extension: ".cmake" });

		file.line("# WARNING: This file is automatically generated by config. Do not edit. #\n");

		file.line("cmake_minimum_required(VERSION 2.8.12.2)\n");

		file.line("file(TO_CMAKE_PATH $ENV{F_HOME} F_HOME)");
		file.line("if(ENV{XS6})");
		file.line("\tfile(TO_CMAKE_PATH $ENV{XS6} XS6)");
		file.line("else()");
		file.line("\tset(XS6 \"${F_HOME}/xs6\")");
		file.line("endif()");

		var split = tool.platform.split("/");
		file.line("set(PLATFORM ", split[0], ")");
		if (split[1])
			file.line("set(SUBPLATFORM ", split[1], ")");

		file.line("list(APPEND CMAKE_MODULE_PATH ${F_HOME}/xs6/cmake/modules)");

		file.line("set(TMP_DIR \"", toCMakePath(tool, tmp), "\")");
		file.line("set(RES_DIR \"${TMP_DIR}/res\")");

		file.line("set(APP_VERSION " + this.tree.environment.VERSION + ")\n");

		file.line("include(", tool.platform, " OPTIONAL)\n");

		file.line("project(fsk)\n");

		file.line("if(CMAKE_CONFIGURATION_TYPES)");
		file.write("\tset(CMAKE_CONFIGURATION_TYPES");
		file.write((this.debug? ' "Debug"' : ' "Release"') + " RelWithDebInfo"); 
		file.write((this.debug? ' "Release"' : ' "Debug"') + " RelWithDebInfo"); 
		file.line(" CACHE STRING \"Reset the configurations to what we need\" FORCE)");
		file.line("\tset(CONFIG_TYPE $(CONFIGURATION))");
		file.line("else()");
		file.line("\tif(NOT CMAKE_BUILD_TYPE)");
		file.line("\t\tmessage(STATUS \"Setting build type to 'Release' as none was specified.\")");
		file.line("\t\tset(CMAKE_BUILD_TYPE ", tool.debug ? "Debug" : "Release", " CACHE STRING \"Choose the type of build.\" FORCE)");
		file.line("\tendif()");
		file.line("\tset_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS \"Debug\" \"Release\" \"RelWithDebInfo\")\n");
		file.line("\tset(CONFIG_TYPE ${CMAKE_BUILD_TYPE})");
		file.line("endif()\n");

		file.line("include(XS6)");
		file.line("include(Kinoma)\n");

		file.line("find_xs_tool(XSC xsc6)");
		file.line("find_xs_tool(XSL xsl6)");
		file.line("find_xs_tool(XSR xsr6)");
		file.line("get_filename_component(XS_BIN_DIR ${XSR} PATH)");
		file.line("set(KPR2JS ${XSR} -a ${XS_BIN_DIR}/modules/tools.xsa kpr2js)");
		file.line("set(XS2JS ${XSR} -a ${XS_BIN_DIR}/modules/tools.xsa xs2js)\n");

		this.generatePlatformVariables(tool, file, tmp, bin);

		file.line("include(", toCMakePath(tool, this.cmakePrefix), " OPTIONAL)\n")

		file.line("if(NOT DEFINED CMAKE_MACOSX_RPATH)");
		file.line("\tset(CMAKE_MACOSX_RPATH 0)");
		file.line("endif()\n");

		this.generateXSVariables(tool, file);
		this.generateResourcesVariables(tool, file);

		file.line("include_directories(${F_HOME}/xs6/patches)");
		file.line("include_directories(${F_HOME}/xs6/includes)");
		file.line("include_directories(${TMP_DIR})");
		file.line("include_directories(${TMP_DIR}/src)");
		file.line("include_directories(${FREETYPE_DIR}/include)");
		file.write("set(CMAKE_C_FLAGS \"${CMAKE_C_FLAGS} ");
		if (tool.windows)
			file.line("/FS /DXS6=1\")");
		else
			file.line("-DXS6=1\")");

		file.write("set(CMAKE_C_FLAGS_RELEASE \"${CMAKE_C_FLAGS_RELEASE} ");
		if (tool.windows)
			file.write("/DSUPPORT_XS_DEBUG=");
		else
			file.write("-DSUPPORT_XS_DEBUG=");
		if (this.tree.xsdebug.enabled || tool.xsdebug)
			file.write("1");
		else
			file.write("0")
		if (tool.windows)
			file.write(" /DSUPPORT_XS_PROFILE=");
		else
			file.write(" -DSUPPORT_XS_PROFILE=");
		if (this.tree.xsprofile.enabled)
			file.write("1");
		else
			file.write("0")
		file.line("\")");

		if (tool.windows)
			file.write("set(CMAKE_C_FLAGS_DEBUG \"${CMAKE_C_FLAGS_DEBUG} /DSUPPORT_XS_DEBUG=1");
		else
			file.write("set(CMAKE_C_FLAGS_DEBUG \"${CMAKE_C_FLAGS_DEBUG} -DSUPPORT_XS_DEBUG=1");
		if (tool.windows)
			file.write(" /DSUPPORT_XS_PROFILE=");
		else
			file.write(" -DSUPPORT_XS_PROFILE=");
		if (this.tree.xsprofile.enabled)
			file.write("1");
		else
			file.write("0")
		file.line("\")");

		var FskManifest = this.makefiles[0];
		this.makefiles.splice(0, 1);
		FskManifest.generateVariables(tool, file, tmp);

		for (let cmakefile of this.tree.cmakefiles)
			file.line("add_subdirectory(\"", toCMakePath(tool, cmakefile.directory), "\" \"", cmakefile.name, "\")");
		for (let makefile of this.makefiles)
			if (!makefile.separate)
				file.line("add_subdirectory(", makefile.name, ")");
		file.line();

		for (let item of this.tree.libraries) {
			var parts = item.split(" ");
			if (parts[0] === "-framework")
				file.line("local_find_library(LIBNAME ", parts[1], " LIST LIBRARIES)");
			else {
				if (tool.platform == "win") {
					if (item.indexOf("/") >= 0)
						file.line("set(CMAKE_EXE_LINKER_FLAGS \"${CMAKE_EXE_LINKER_FLAGS} ", toCMakePath(tool, item), "\")");
					else
						file.line("list(APPEND LIBRARIES ", toCMakePath(tool, item), ")");
				} else {
					file.line("list(APPEND LIBRARIES ", toCMakePath(tool, item), ")");
				}
			}
		}
		file.line()

		file.line("list(APPEND SEPARATE");
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

		file.line("list(APPEND SOURCES \"${TMP_DIR}/FskManifest.c\")");
		file.line("list(APPEND SOURCES \"${TMP_DIR}/src/FskManifest.xs.c\")");
		file.line("set_source_files_properties(\"${TMP_DIR}/src/FskManifest.xs.c\" \"${TMP_DIR}/src/FskManifest.xs.h\" PROPERTIES GENERATED TRUE)");

		this.generateTargetRules(tool, file);

		for (let makefile of this.makefiles)
			if (makefile.separate)
				file.line("add_subdirectory(", makefile.name, ")");

		file.line();
		file.line("# vim: ft=cmake");

		file.close();
	}
	generateManifestRules(tool, file) {
	}
	generateResourcesVariables(tool, file) {
		file.line("set(MODULES");
		file.line("\t\"FskManifest.xsb\"");
		for (let item of this.tree.xmlPaths)
			file.line("\t\"", toCMakePath(tool, item.destinationPath), ".xsb\"");
		for (let item of this.tree.jsPaths)
			file.line("\t\"", toCMakePath(tool, item.destinationPath), ".xsb\"");
		for (let makefile of this.makefiles)
			if (makefile.jsSourcePath && makefile.jsDestinationPath)
				file.line("\t\"", toCMakePath(tool, makefile.jsDestinationPath), ".xsb\"");
		file.line("\t)\n");
	}
	generateTargetRules(tool, file) {
		if (FS.existsSync(this.cmakeSuffix))
			file.line("include(", toCMakePath(tool, this.cmakeSuffix), ")");
		else
			file.write(this.getTargetRules(tool));
	}
	generateXSRules(tool, file) {
		file.line("xs2js(SOURCE ${TMP_DIR}/FskManifest.xs DESTINATION ${TMP_DIR} OPTIONS ${XSC_OPTIONS})");
		file.line("xsc(SOURCE_FILE ${TMP_DIR}/FskManifest.js DESTINATION ${TMP_DIR} OPTIONS -c -d -e -p)");
		file.line("xsl(NAME FskManifest SOURCES ${MODULES} TMP ${TMP_DIR} DESTINATION ${RES_DIR}/ SRC_DIR ${TMP_DIR}/src COPY ${APP_DIR} ${BUILD_APP_DIR})");
	}
	generateXSVariables(tool, file) {
		file.line("set(XSC_FLAGS $<$<CONFIG:Debug>:-d>)");
		file.line("set(XSC_OPTIONS");
		file.line("\t-b");
		file.line("\t$<$<CONFIG:Debug>:-d>");
		for (let item of this.xsIncludes)
			file.line("\t-i \"", toCMakePath(tool, item), "\"");
		file.line("\t$<$<CONFIG:Debug>:-t> $<$<CONFIG:Debug>:debug>");
		file.line("\t-t KPR_CONFIG");
		file.line("\t-t XS6");
		for (let item of this.xsOptions)
			file.line("\t", item);
		file.line("\t)\n");
		file.line("set(XSC_PACKAGES");
		for (let item of this.xsSources)
			file.line("\t\"", toCMakePath(tool, item), "\"");
		file.line("\t)\n");
	}
	generatePlatformVariables(tool, file, tmp, bin) {
		let variables = this.getPlatformVariables(tool, tmp, bin);
		for (let name in variables) {
			let value = variables[name];
			if (typeof value == "string") {
				value = toCMakePath(tool, value);
			}
			file.line("set(", name, " \"", value, "\")");
		}
		var languages = this.getPlatformLanguages();
		if (languages) {
			file.line();
			for (let language of languages)
				file.line("enable_language(", language, ")");
			file.line();
		}
	}
	generateResourcesRules(tool, file) {
		for (let item of this.tree.xmlPaths) {
			let parts = tool.splitPath(item.destinationPath);
			let sourcePath = toCMakePath(tool, item.sourcePath);
			let destinationPath = toCMakePath(tool, item.destinationPath);
			let directory = toCMakePath(tool, parts.directory);
			let deps = this.resolveXMLIncludes(tool, item.sourcePath);
			file.write("kpr2js(SOURCE \"" + sourcePath + "\" DESTINATION \"${TMP_DIR}/" + directory + "\"");
					if (deps.length > 0) {
						file.write(" DEPENDS ");
						for (let i in deps) {
							file.write("\"" + deps[i] + "\"");
							if (i < deps.length)
								file.write(" ");
						}
					}
					file.line(")");
			file.line("xsc(SOURCE_FILE \"${TMP_DIR}/", directory, "/", parts.name, ".js\" DESTINATION \"${TMP_DIR}/", directory, "\" OPTIONS ${XSC_FLAGS})");
		}
		file.line();
		for (let item of this.tree.jsPaths) {
			let parts = tool.splitPath(item.destinationPath);
			let sourcePath = toCMakePath(tool, item.sourcePath);
			let destinationPath = toCMakePath(tool, item.destinationPath);
			let directory = toCMakePath(tool, parts.directory);
			file.line("xsc(SOURCE_FILE \"", sourcePath, "\" DESTINATION \"${TMP_DIR}/", directory, "\" OPTIONS ${XSC_FLAGS} -c -e)");
		}
		file.line();
		for (let makefile of this.makefiles) {
			if (makefile.jsSourcePath && makefile.jsDestinationPath) {
				let parts = tool.splitPath(makefile.jsDestinationPath);
				let sourcePath = toCMakePath(tool, makefile.jsSourcePath);
				let destinationPath = toCMakePath(tool, makefile.jsDestinationPath);
				let directory = toCMakePath(tool, parts.directory);
				file.line("xsc(SOURCE_FILE \"", sourcePath, "\" DESTINATION \"${TMP_DIR}/", directory, "\" OPTIONS ${XSC_FLAGS} COMPILE)");
			}
		}
		for (let item of this.tree.otherPaths) {
			let sourcePath = toCMakePath(tool, item.sourcePath);
			let destinationPath = toCMakePath(tool, item.destinationPath);
			file.line("copy(SOURCE \"", sourcePath, "\" DESTINATION \"${RES_DIR}/", destinationPath, "\")");
		}
		file.line();
	}
	getPlatformLanguages() {
		return [];
	}
	getGenerator() {
		return;
	}
	getIDEGenerator() {
		return;
	}
	generateProject(tool) {
		if (tool.cmakeGenerate) {
			tool.report("Generating project...");
			var command = "cmake -H\"" + tool.tmpPath + "\" -B\"" + tool.tmpPath + "\" -DCMAKE_BUILD_TYPE=\"" + (tool.debug ? "Debug" : "Release") + "\"";
			var generator = this.getGenerator(tool);
			if (tool.ide) {
				let ide = this.getIDEGenerator(tool);
				if (ide)
					generator = ide;
			} else {
				if (tool.cmakeGenerator)
					generator = tool.cmakeGenerator;
			}
			if (generator)
				command += " -G\"" + generator + "\"";
			for (let flag of tool.cmakeFlags) {
				let parts = flag.split(/=/);
				if (parts && parts.length > 1)
					command += " -D" + parts[0] + "=\"" + parts[1] + "\"";
				else
					command += " -D" + flag + "=\"TRUE\"";
			}
			if (tool.verbose)
				command += " -DCMAKE_VERBOSE_MAKEFILE=\"TRUE\"";
			var cmakeCache = tool.joinPath({ directory: tool.tmpPath, name: "CMakeCache", extension: ".txt" });
			if (FS.existsSync(cmakeCache)) {
				var cache = FS.readFileSync(cmakeCache);
				var currentGenerator = cache.match(/CMAKE_GENERATOR:INTERNAL=(.*)/)[1];
				if (currentGenerator && currentGenerator != generator)
					FS.deleteFile(cmakeCache);
			}
			tool.report(command);
			var output = tool.execute(command);
			tool.report(output.trim());
			if (tool.ide) {
				tool.report("Opening the IDE...");
				this.openIDE(tool, tool.tmpPath + tool.slash);
				return;
			}
		}
	}
	resolveXMLIncludes(tool, path, deps) {
		if (!deps)
			var deps = [];
		if (FS.existsSync(path)) {
			var parts =  tool.splitPath(path);
			var buffer = FS.readFileSync(path);
			var program = TEMPLATE.parse(buffer, path);
			if (program.items) {
				for (let item of program.items) {
					if (item.path && item.path != "/" + parts.name) {
						var itemName = item.path.replace(/\//g, tool.slash);
						var xmlPath = tool.joinPath({ directory: parts.directory, name: itemName, extension: ".xml" });
						if (!FS.existsSync(xmlPath)) {
							for (let xsPath of this.xsIncludes) {
								xmlPath = tool.joinPath({ directory: xsPath, name: itemName, extension: ".xml" });
								if (FS.existsSync(xmlPath))
									break;
							}
						}
						if (FS.existsSync(xmlPath)) {
							var fixedXMLPath = toCMakePath(tool, xmlPath);
							if (deps.indexOf(fixedXMLPath) == -1)
								deps.push(fixedXMLPath);
							this.resolveXMLIncludes(tool, xmlPath, deps)
						}
					}
				}
			}
		}
		return deps;
	}
	make(tool) {
		if (tool.cmakeGenerate && !tool.ide) {
			tool.report("cmake --build \"" + tool.tmpPath + "\" --config \"" + (tool.debug ? "Debug" : "Release") + "\"");
			process.then("cmake", "--build", tool.tmpPath, "--config", tool.debug ? "Debug" : "Release");
		}
		return;
	}
}
