<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->
<package>
	<object name="xsMakefileItem">
		<string name="name" pattern="@name"/>
		<function name="crossReference" params="thePlatform, theMakefile"/>
		<function name="write" params="theFile, thePlatform, theDebugFlag"/>
	</object>
	
	<object name="xsMakefileList" prototype="xsMakefileItem">
		<array name="items" pattern="." contents="xsMakefileItem"/>
		<function name="crossReferenceItems" params="thePlatform, theMakefile">
			var items = this.items;
			var c = items.length;
			for (var i = 0; i < c; i++)
				items[i].crossReference(thePlatform, theMakefile);
		</function>
	</object>
	
	<object name="xsMakefile" pattern="/makefile" prototype="xsMakefileList">
		<array name="items" pattern="." contents="xsMakefileItem"/>

		<function name="crossReference" params="thePlatform, theDirectory, theName">
			this.name = theName;
			this.cmakeDepends = "";
			this.cmakeLinks = "";
			this.cmakeOutputType = this.separate ? "shared" : "object";
			this.cIncludes = [
				theDirectory,
				xsTool.joinPath(theDirectory, "sources"),
			];
			this.cOptions = [];
			this.asmOptions = [];
			this.headers = [];
			this.libraries = [];
			this.sources = [];
			this.versions = {
				debug: {
					cmakeDepends: this.cmakeDepends,
					cmakeLinks: this.cmakeLinks,
					cmakeOutputType: this.cmakeOutputType,
					separate: this.separate,
					cIncludes: [],
					cOptions: [],
					asmOptions: [],
					headers: [],
					libraries: [],
					sources: [],
				},
				release:  {
					cmakeDepends: this.cmakeDepends,
					cmakeLinks: this.cmakeLinks,
					cmakeOutputType: this.cmakeOutputType,
					separate: this.separate,
					cIncludes: [],
					cOptions: [],
					asmOptions: [],
					headers: [],
					libraries: [],
					sources: [],
				},
			};
			this.crossReferenceItems(thePlatform, this);
		</function>

		
		<function name="generateItemsVariables" params="theFile, suffix, items">
			theFile.write(this.name)
			theFile.write(suffix)
			theFile.write(" =")
			var c = items.length;
			for (var i = 0; i < c; i++)
				items[i].generateVariables(theFile, this);
			theFile.write("\n");
		</function>

		<function name="generateVariables" params="theFile">
			theFile.write(this.name + "_C_INCLUDES = ")
			var c = this.cIncludes.length;
			if (xsTool.windows) {
				for (var i = 0; i < c; i++) {
					theFile.write(" \\\n\t/I");
					theFile.write(this.cIncludes[i]);
				}
			}
			else {
				for (var i = 0; i < c; i++) {
					theFile.write(" \\\n\t-I");
					theFile.write(this.cIncludes[i]);
				}
			}
			theFile.write(" \\\n\t-I$(TMP_DIR)/");
			theFile.write(this.name);
			theFile.write("\n");
			this.generateItemsVariables(theFile, "_C_OPTIONS", this.cOptions);
			this.generateItemsVariables(theFile, "_HEADERS", this.headers);
			if (this.sources.length) {
				var path = xsTool.joinPath(xsTool.tmpPath, this.name);
				xsTool.createDirectory(path);
				theFile.write(this.name)
				theFile.write("_ARCHIVE = ")
				theFile.write(path)
				if (xsTool.windows)
					theFile.write(".lib\n")
				else
					theFile.write(".a\n")
				this.generateItemsVariables(theFile, "_OBJECTS", this.sources);
			}
			if (this.separate) {
				theFile.write(this.name + "_LIBRARIES = ")
				c = this.libraries.length;
				for (var i = 0; i < c; i++) {
					theFile.write(" \\\n\t");
					theFile.write(this.libraries[i]);
				}
				theFile.write(" \n");
			}
		</function>
		
		<function name="generateRules" params="theFile">
			var c = this.sources.length;
			if (c) {
				if (xsTool.windows) {
					theFile.write("$(");
					theFile.write(this.name);
					theFile.write("_ARCHIVE): $(");
					theFile.write(this.name);
					theFile.write("_OBJECTS)\n\tlib /NOLOGO /OUT:$(");
					theFile.write(this.name);
					theFile.write("_ARCHIVE) $(");
					theFile.write(this.name);
					theFile.write("_OBJECTS)\n");
				}
				else {
					theFile.write("$(");
					theFile.write(this.name);
					theFile.write("_ARCHIVE): $(");
					theFile.write(this.name);
					theFile.write("_OBJECTS)\n\t$(AR) $(");
					theFile.write(this.name);
					theFile.write("_ARCHIVE) $(");
					theFile.write(this.name);
					theFile.write("_OBJECTS)\n");
				}		
				for (var i = 0; i < c; i++) {
					var file = this.sources[i];
					file.generateRule(theFile, this);
				}
			}
		</function>
		
		<function name="generateCMake">
			var path = xsTool.joinPath(xsTool.tmpPath, this.name);
			xsTool.createDirectory(path);
			path = xsTool.joinPath(path, "CMakeLists", "txt");
			aFile = new XSFile(path);
			aFile.write('cmake_minimum_required(VERSION 2.8)\n');
			aFile.write('# WARNING: This file is automatically generated by kprconfig. Do not edit.\n');
			aFile.write('# SOURCE: ' + this.mkPath + '\n\n');
			aFile.write('project('); aFile.write(this.name);aFile.write(')\n'); 
			aFile.write('include_directories(${BUILD_TMP}/)\n'); 
			aFile.write('include_directories(${BUILD_TMP}/'); aFile.write(this.name); aFile.write(')\n\n'); 

			if (xsTool.platform == "android") {
				aFile.write('STRING(REGEX REPLACE "/" "_" KPR_NATIVE_JAVA_NAMESPACE ${FSK_JAVA_NAMESPACE})\n');
				aFile.write('set(CMAKE_C_FLAGS "-DKPR_NATIVE_JAVA_NAMESPACE=${KPR_NATIVE_JAVA_NAMESPACE} ${BASE_C_FLAGS}")\n');
				aFile.write('set(CMAKE_CXX_FLAGS ${BASE_CXX_FLAGS})\n\n');
			}

			if (xsTool.platform == "android")
				aFile.write('set(LIBRARY_OUTPUT_PATH ${NDK_LIBS_PATH})\n');
			aFile.write('\n');

			this.generateCMakeVersion(this);
			if (xsTool.both) {
				aFile.write('if (CMAKE_BUILD_TYPE STREQUAL "DEBUG")\n');
				this.generateCMakeVersion(this.versions.debug);
				aFile.write('else ()\n');
				this.generateCMakeVersion(this.versions.release);
				aFile.write('endif ()\n\n');
			}

			aFile.close();
		</function>
		<function name="generateCMakeVersion" params="version">
			var c = version.sources.length;
			for (var i = 0; i < c; i++)
				version.sources[i].generateCMake(aFile, version);
			aFile.write('\n');

			aFile.write('if (${CMAKE_GENERATOR} STREQUAL "Xcode")\n');
			aFile.write('\tforeach (source ${sources})\n');
			aFile.write('\t\tget_filename_component(extname ${source} EXT)\n');
			aFile.write('\t\t\tif (${extname} STREQUAL ".gas7" OR ${extname} STREQUAL ".gas")\n');
			aFile.write('\t\t\tset_source_files_properties(${source} PROPERTIES LANGUAGE ASM)\n');
			aFile.write('\t\tendif ()\n');
			aFile.write('\tendforeach ()\n');
			aFile.write('endif ()\n\n');
			
			var c = version.cOptions.length;
			var defs = [];
			var copts = [];
			for (var i = 0; i < c; i++) {
				optionPath = version.cOptions[i].name;
				results = optionPath.match(/"?[-\/][^ ]*(\s*[^-\/]*)/g);
				if (results) {
					for (var j = 0; j < results.length; j++) {
						var option = results[j].trim();
						var regexpVar = /\$\(([^)]*)\)/;
						var regexpStr = /\$([^)]*\))/;
						var matches = option.match(regexpVar);
						if (matches)
							option = option.replace(regexpStr, "${" + matches[1] + "}");
						switch (option.match(/"?(..)/)[1]) {
							case "-D":
								defs.push(option);
								break;
							case "/D":
								defs.push(option);
								break;
							case "-U":
								defs.push(option);
								break;
							case "/U":
								defs.push(option);
								break;
							case "-I":
								version.cIncludes.push(option.substring(2));
								break;
							case "/I":
								version.cIncludes.push(option.substring(2));
								break;
							default:
								copts.push(option);
						}
					}
				}
			}

			var c = version.cIncludes.length;
			if (c) {
				for (var i = 0; i < c; i++) {
					path = xsTool.toCMakePath(version.cIncludes[i]);
					aFile.write('include_directories('); aFile.write(path); aFile.write(')\n');
				}
				aFile.write('\n');

				aFile.write('file(GLOB headers\n');
				for (var i = 0; i < c; i++) {
					path = xsTool.toCMakePath(version.cIncludes[i]);
					aFile.write('\t"'); aFile.write(path); aFile.write('/*.h"\n');
				}
				aFile.write(')\n');
				aFile.write('list(APPEND sources ${headers})\n\n');
			}
			
			var c = defs.length;
			if (c) {
				for (var i = 0; i < c; i++) {
					aFile.write('add_definitions("');
					aFile.write(defs[i]);
					aFile.write('")\n'); 
				}
				aFile.write('\n');
			}

			var c = copts.length;
			if (c) {
				for (var i = 0; i < c; i++) {
					aFile.write('set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ')
					aFile.write(copts[i]);
					aFile.write('")\n'); 
				}
				aFile.write('\n');
			}

			var c = version.asmOptions.length;
			if (c) {
				for (var i = 0; i < c; i++) {
					aFile.write('set(CMAKE_ASM-gas7_FLAGS "${CMAKE_ASM-gas7_FLAGS} ');
					aFile.write(version.asmOptions[i].name);
					aFile.write('")\n');
				}
				aFile.write('\n');
			}

			var c = version.libraries.length;
			var lincs = [];
			var lflags = [];
			for (i = 0; i < c; i++) {
				libraries = version.libraries[i].replace("(", "{").replace(")", "}");
				results = libraries.match(/-[^ ]*(\s*[^-]*)/g);
				if (results) {
					for (var j = 0; j < results.length; j++) {
						var option = results[j].trim();
						switch (option.substring(0,2)) {
							case "-L":
								lincs.push(option.substring(2));
								break;
							case "-l":
								lflags.push(option);
								break;
						}
					}
				}
			}
			
			var c = lincs.length;
			if (c) {
				if (xsTool.platform == "android")
					aFile.write('link_directories(${NDK_LIBS_PATH})\n');
				for (i = 0; i < lincs.length; i++)
					aFile.write('link_directories(' + xsTool.toCMakePath(lincs[i]) + ')\n');
				aFile.write('\n');
			}

			aFile.write('set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")\n\n')

			if ((this.sources.length > 0) || (this.versions.debug.sources > 0) || (this.versions.release.sources > 0)) {
				aFile.write('add_library(${PROJECT_NAME} ' + this.cmakeOutputType.toUpperCase() + ' ${sources})\n');
			
				if (this.separate) {
					aFile.write('target_link_libraries(${PROJECT_NAME} ${SHARED_LINK_FLAGS} ' + lflags.join(' '));
							if (version.cmakeLinks)
								aFile.write(' ' + version.cmakeLinks);
							aFile.write(')\n');
				}

				aFile.write('add_dependencies(${PROJECT_NAME} FskManifest');
				if (this.separate || version.cmakeDepends)
				{
					aFile.write(' ${SHARED_DEPENDS} ');
					if (version.cmakeDepends)
						aFile.write(version.cmakeDepends)
				}
				aFile.write(')\n');
			}
		</function>
	</object>
	
	<object name="xsInput" pattern="input" prototype="xsMakefileItem">
		<function name="crossReference" params="thePlatform, theMakefile">
			if (!this.name)
				xsTool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
			else {
				var aPath = xsTool.searchPath(this.name, true);
				if (aPath) {
					xsTool.insertUnique(theMakefile.cIncludes, aPath);
					xsTool.inputPaths[xsTool.inputPaths.length] = aPath;
				}
				else
					xsTool.reportError(this.__xs__path, this.__xs__line, "input directory not found");
			}
		</function>
	</object>
	
	<object name="xsConfiguration" pattern="configuration" prototype="xsMakefileList">
		<function name="crossReference" params="thePlatform, theMakefile">
			if (!this.name)
				xsTool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
			else if (xsTool.both) {
				if (this.name.indexOf("debug") >= 0)
					this.crossReferenceItems(thePlatform, theMakefile.versions.debug);
				else if (this.name.indexOf("release") >= 0)
					this.crossReferenceItems(thePlatform, theMakefile.versions.release);
				else
					this.crossReferenceItems(thePlatform, theMakefile);
			}
			else {
				var context = {
					debug: xsTool.debug,
					release: !xsTool.debug,
					cmake: xsTool.cmake
				};
				var name = this.name.replace(",", "&&");
				with (context) 
					var flag = eval(this.name);
				if (flag)
					this.crossReferenceItems(thePlatform, theMakefile);
			} 
		</function>
	</object>
	<object name="xsPlatform" pattern="platform" prototype="xsMakefileList">
		<function name="crossReference" params="thePlatform, theMakefile">
			var c, i;
			if (!this.name)
				xsTool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
			else {
				var platforms = this.name.split(",");
				c = platforms.length;
				for (i = 0; i < c; i++) {
					var platform = xsTool.trimString(platforms[i]);
					var subplatform;
					var split = platform.split("/");
					if (split.length > 1) {
						platform = split[0];
						subplatform = split[1];
					}
					if (platform in xsTool.platformNames)
						platform = xsTool.platformNames[platform];
					if ((platform == thePlatform) && (subplatform ? (subplatform == xsTool.subplatform) : true)) {
						this.crossReferenceItems(thePlatform, theMakefile);
						break;
					}
				}
			}
		</function>
	</object>
	
	<object name="xsCOption" pattern="c" prototype="xsMakefileItem">
		<string name="name" pattern="@option"/>
		<function name="crossReference" params="thePlatform, theMakefile">
			theMakefile.cOptions.push(this);
		</function>
		<function name="generateVariables" params="theFile">
			theFile.write(" \\\n\t");
			theFile.write(this.name);
		</function>
	</object>
	<object name="xsASMOption" pattern="asm" prototype="xsMakefileItem">
		<string name="name" pattern="@option"/>
		<function name="crossReference" params="thePlatform, theMakefile">
			if (xsTool.cmake)
				theMakefile.asmOptions.push(this);
			else
				theMakefile.cOptions.push(this);
		</function>
		<function name="generateVariables" params="theFile">
			theFile.write(" \\\n\t");
			theFile.write(this.name);
		</function>
	</object>
	<object name="cmakeOptions" pattern="cmake" prototype="xsMakefileItem">
		<string name="type" pattern="@type" />
		<string name="depends" pattern="@depends" />
		<string name="links" pattern="@links" />
		<function name="crossReference" params="thePlatform, theMakefile">
			if (xsTool.cmake) {
				theMakefile.cmakeOutputType = this.type;
				theMakefile.cmakeDepends = this.depends;
				theMakefile.cmakeLinks = this.links;
			}
		</function>
		<function name="generateVariables" params="theFile">
			theFile.write(" \\\n\t");
			theFile.write(this.name);
		</function>
	</object>
	<object name="xsLibrary" pattern="library" prototype="xsMakefileItem">
		<function name="crossReference" params="thePlatform, theMakefile">
			if (theMakefile.separate)
				xsTool.insertUnique(theMakefile.libraries, this.name);
			else
				xsTool.insertUnique(xsTool.libraries, this.name);
		</function>
		<function name="generateVariables" params="theFile">
			theFile.write(" \\\n\t");
			theFile.write(this.name);
		</function>
	</object>
	<object name="xsSource" pattern="source" prototype="xsMakefileItem">
		<function name="add" params="thePlatform, theMakefile">
			theMakefile.sources.push(this);
		</function>
		<function name="crossReference" params="thePlatform, theMakefile">
			if (!this.name)
				xsTool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
			else {
				var aPath = xsTool.getPath(this.name);
				if (!aPath)
					xsTool.reportError(this.__xs__path, this.__xs__line, this.name + ": file not found");
				else if ((aPath in xsTool.sources) && !theMakefile.separate) {
					if (xsTool.verbose)
						xsTool.report("Ignoring " + aPath + "...");
				}
				else {
					if (!theMakefile.separate)
						xsTool.sources[aPath] = true;
					this.path = aPath;
					var aSplit = xsTool.splitPath(aPath);
					this.name = aSplit[1];
					this.extension = aSplit[2];
					this.add(thePlatform, theMakefile);
				}
			}
		</function>
		<function name="generateCMake" params="theFile, theMakefile">
			var regexp = /(?:\.([^.]+))?$/;
			var extension = regexp.exec(this.path)[1]
			if (extension == "java" && xsTool.platform == "android") {
				var ndk = xsTool.joinPath(xsTool.tmpPath, "ndk");
				var project_temp = xsTool.joinPath(ndk, "project_temp");
				var java = xsTool.joinPath(project_temp, "java");
				var fileName = this.path.split("/").pop();
				var outputPath = xsTool.joinPath(java, fileName);

				xsTool.createDirectory(ndk);
				xsTool.createDirectory(project_temp);
				xsTool.createDirectory(java);

				var aFile = new XSFile(outputPath);
				aFile.include(this.path);
				aFile.close();
			} else if (this.extension == ".xs") {
				theFile.write("xscc(" + this.path + " " + theMakefile.name + ")\n");
			} else {
				theFile.write("list(APPEND sources ");
				theFile.write(xsTool.toCMakePath(this.path.replace("(", "{").replace(")", "}")));
				theFile.write(")\n");
			}
		</function>
		<function name="generateRule" params="theFile, theMakefile">
			var aName = this.name
			var aPath = this.path
			if (this.extension == ".gas") {
				theFile.write("$(TMP_DIR)/");
				theFile.write(theMakefile.name);
				theFile.write("/");
				theFile.write(aName);
				theFile.write(".gas.o: $(HEADERS) $(");
				theFile.write(theMakefile.name);
				theFile.write("_HEADERS) ");
				theFile.write(aPath);
				if (aName.indexOf("neon") >= 0)
					theFile.write("\n\t$(AS_NEON) $(AS_NEON_OPTIONS) ");
				else
					theFile.write("\n\t$(AS) $(AS_OPTIONS) ");
				theFile.write(aPath);
				theFile.write(" -o $@\n");
			}
			else if (this.extension == ".gas7") {
				theFile.write("$(TMP_DIR)/");
				theFile.write(theMakefile.name);
				theFile.write("/");
				theFile.write(aName);
				theFile.write(".gas7.o: $(HEADERS) $(");
				theFile.write(theMakefile.name);
				theFile.write("_HEADERS) ");
				theFile.write(aPath);
				theFile.write("\n\t$(AS_V7) $(AS_V7_OPTIONS) ");
				theFile.write(aPath);
				theFile.write(" -o $@\n");
			}
			else if (this.extension == ".wmmx") {
				theFile.write("$(TMP_DIR)/");
				theFile.write(theMakefile.name);
				theFile.write("/");
				theFile.write(aName);
				theFile.write(".wmmx.o: $(HEADERS) $(");
				theFile.write(theMakefile.name);
				theFile.write("_HEADERS) ");
				theFile.write(aPath);
				theFile.write("\n\t$(AS_WMMX) $(AS_WMMX_OPTIONS) ");
				theFile.write(aPath);
				theFile.write(" -o $@\n");
			}
			else if (this.extension == ".xs") {
				var outDir = "$(TMP_DIR)/" + theMakefile.name;
				var outFile = outDir + "/" + aName + ".xs.c";
				theFile.write(outFile + ": " + aPath);
				theFile.write("\n\t$(XSC) " + aPath + " $(XSC_OPTIONS) -o " + outDir);
				theFile.write("\n");
				var out = new XSSource(aName + ".xs", ".c", outFile);
				out.generateRule(theFile, theMakefile);
			}
			else if (xsTool.windows) {
				theFile.write("$(TMP_DIR)\\");
				theFile.write(theMakefile.name);
				theFile.write("\\");
				theFile.write(aName);
				theFile.write(".o: $(HEADERS) $(");
				theFile.write(theMakefile.name);
				theFile.write("_HEADERS) ");
				theFile.write(aPath);
				theFile.write("\n\tcl ");
				theFile.write(aPath);
				theFile.write(" $(C_OPTIONS) $(");
				theFile.write(theMakefile.name);
				theFile.write("_C_OPTIONS) ");
				if (theMakefile.separate) {
					theFile.write("-UFSK_EMBED -UFSK_EXTENSION_EMBED ");
				}
				theFile.write("$(");
				theFile.write(theMakefile.name);
				theFile.write("_C_INCLUDES) $(C_INCLUDES) /Fo$@\n");
			}
			else {
				theFile.write("$(TMP_DIR)/");
				theFile.write(theMakefile.name);
				theFile.write("/");
				theFile.write(aName);
				theFile.write(".o: $(HEADERS) $(");
				theFile.write(theMakefile.name);
				theFile.write("_HEADERS) ");
				theFile.write(aPath);
				theFile.write("\n\t$(CC) ");
				theFile.write(aPath);
				theFile.write(" $(C_OPTIONS) $(");
				theFile.write(theMakefile.name);
				theFile.write("_C_OPTIONS) ");
				if (theMakefile.separate) {
					theFile.write("-UFSK_EMBED -UFSK_EXTENSION_EMBED ");
				}
				theFile.write("$(C_INCLUDES) ");
				theFile.write("$(");
				theFile.write(theMakefile.name);
				theFile.write("_C_INCLUDES) -c -o $@\n");
			}
		</function>
		<function name="generateVariables" params="theFile, theMakefile">
			if (this.extension == ".gas") {
				theFile.write(" \\\n\t$(TMP_DIR)/");
				theFile.write(theMakefile.name);
				theFile.write("/");
				theFile.write(this.name);
				theFile.write(".gas.o");
			}
			else if (this.extension == ".gas7") {
				theFile.write(" \\\n\t$(TMP_DIR)/");
				theFile.write(theMakefile.name);
				theFile.write("/");
				theFile.write(this.name);
				theFile.write(".gas7.o");
			}
			else if (this.extension == ".wmmx") {
				theFile.write(" \\\n\t$(TMP_DIR)/");
				theFile.write(theMakefile.name);
				theFile.write("/");
				theFile.write(this.name);
				theFile.write(".wmmx.o");
			}
			else if (this.extension == ".xs") {
				theFile.write(" \\\n\t$(TMP_DIR)/");
				theFile.write(theMakefile.name);
				theFile.write("/");
				theFile.write(this.name);
				theFile.write(".xs.o");
			}
			else if (xsTool.windows) {
				theFile.write(" \\\n\t$(TMP_DIR)\\");
				theFile.write(theMakefile.name);
				theFile.write("\\");
				theFile.write(this.name);
				theFile.write(".o");
			}
			else {
				theFile.write(" \\\n\t$(TMP_DIR)/");
				theFile.write(theMakefile.name);
				theFile.write("/");
				theFile.write(this.name);
				theFile.write(".o");
			}
		</function>
	</object>
	<function name="XSSource" params="theName, theExtension, thePath" prototype="xsSource">
		this.name = theName;
		this.extension = theExtension;
		this.path = thePath;
	</function>
	
	<object name="xsHeader" pattern="header" prototype="xsSource">
		<function name="add" params="thePlatform, theMakefile">
			theMakefile.headers.push(this);
		</function>
		<function name="generateVariables" params="theFile">
			theFile.write(" \\\n\t");
			theFile.write(this.path);
		</function>
	</object>
	
	<object name="xsBuildStyle" pattern="buildstyle" prototype="xsMakefileList">
	</object>
	<object name="xsCommon" pattern="common" prototype="xsMakefileItem">
		<string name="text" pattern="."/>
		<function name="concatOptions" params="text, options">
			var regexpTmp = /-I\$\(F_HOME\)(\/|)tmp\/(include|android)/g;
			var regexp = /^C_OPTIONS\s+\+?=\s+(.+)$/gm;
			var results = regexp.exec(text);
			if (results) {
				if (options) options += " ";
				options += results[1].replace(regexpTmp, "");
			}
			var regexp = /^DEFAULT_C_OPTIONS\s+\+?=\s+(.+)$/gm;
			var results = regexp.exec(text);
			if (results) {
				if (options) options += " ";
				options += results[1].replace(regexpTmp, "");
			}
			var regexp = /^COMMON_C_OPTIONS\s+\+?=\s+(.+)$/gm;
			var results = regexp.exec(text);
			if (results) {
				if (options) options += " ";
				options += results[1].replace(regexpTmp, "");
			}
			options = options.replace("$(C_OPTIONS)", "");
			return options;
		</function>
		<function name="concatLibraries" params="text, libraries">
			var regexp = /^LIBRARIES\s+\+?=\s+(.+)$/gm;
			var results = regexp.exec(text);
			if (results) {
				if (libraries) libraries += " ";
				libraries += results[1];
			}
			var regexp = /^DEFAULT_LIBRARIES\s+\+?=\s+(.+)$/gm;
			var results = regexp.exec(text);
			if (results) {
				if (libraries) libraries += " ";
				libraries += results[1];
			}
			var regexp = /^COMMON_LIBRARIES\s+\+?=\s+(.+)$/gm;
			var results = regexp.exec(text);
			if (results) {
				if (libraries) libraries += " ";
				libraries += results[1];
			}
			libraries = libraries.replace(/\s+/g, " ");
			return libraries;
		</function>
		<function name="concatObjects" params="text, theMakefile">
			var results = [];
			var c, separators;

			separators = text.split(/^\$\(TMP_DIR\)\/.*(\.gas)?\.o: *\$\(F_HOME\).*\.gas$/gm);
			c = separators.length;
			if (c > 1) {
				var temp = text;
				for (var i = 0; i < c; i++)
					temp = temp.replace(separators[i], ",");
				results = results.concat(temp.split(","));
			}
			separators = text.split(/^\$\(TMP_DIR\)\/.*(\.gas7)?\.o: *\$\(F_HOME\).*\.gas7$/gm);
			c = separators.length;
			if (c > 1) {
				var temp = text;
				for (var i = 0; i < c; i++)
					temp = temp.replace(separators[i], ",");
				results = results.concat(temp.split(","));
			}
			separators = text.split(/^\$\(TMP_DIR\)\/.*(\.wmmx)?\.o: *\$\(F_HOME\).*\.wmmx$/gm);
			c = separators.length;
			if (c > 1) {
				var temp = text;
				for (var i = 0; i < c; i++)
					temp = temp.replace(separators[i], ",");
				results = results.concat(temp.split(","));
			}
			c = results.length;
			for (var i = 1; i < c; i++) {
				var item = results[i];
				var pathIndex = item.lastIndexOf("$(F_HOME)");
				if (pathIndex >= 0) {
					var path = item.slice(pathIndex);
					var split = xsTool.splitPath(path);
					theMakefile.sources.push(new XSSource(split[1], split[2], path));
				}
			}
		</function>
		<function name="concatXSCOptions" params="text, options">
			var regexp = /^XSC_OPTIONS\s+\+?=\s+(.+)$/gm;
			var results = regexp.exec(text);
			var options = xsTool.xscOptions;
			if (results) {
				if (options) options += " ";
				options += results[1].replace("$(XSC_OPTIONS)", "");
			}
			var regexp = /^DEFAULT_XSC_OPTIONS\s+\+?=\s+(.+)$/gm;
			var results = regexp.exec(text);
			if (results) {
				if (options) options += " ";
				options += results[1].replace("$(DEFAULT_XSC_OPTIONS)", "");
			}
			var regexp = /^COMMON_XSC_OPTIONS\s+\+?=\s+(.+)$/gm;
			var results = regexp.exec(text);
			if (results) {
				if (options) options += " ";
				options += results[1].replace("$(COMMON_XSC_OPTIONS)", "");
			}
			xsTool.xscOptions = options;
		</function>
		<function name="crossReference" params="thePlatform, theMakefile">
			if (this.text) {
				var text = this.filter(this.text);
				var options = this.concatOptions(text, "");
				var libraries = this.concatLibraries(text, "");
				this.concatObjects(text, theMakefile);
				this.concatXSCOptions(text);
				if (options || libraries) {
					this.items = [];
					if (options) {
						var option = xs.newInstanceOf(xsCOption);
						option.name = options;
						theMakefile.cOptions.push(option);
					}
					if (libraries) {
						var split = libraries.split(" ");
						var c = split.length;
						for (var i = 0; i < c; i++) {
							var name = split[i];
							if (name == "-framework") {
								i++;
								if (theMakefile.separate)
									xsTool.insertUnique(theMakefile.libraries, name + " " + split[i]);
								else
									xsTool.insertUnique(xsTool.libraries, name + " " + split[i]);
							}
							else {
								if (theMakefile.separate)
									xsTool.insertUnique(theMakefile.libraries, split[i]);
								else
									xsTool.insertUnique(xsTool.libraries, split[i]);
							}
						} 
					}
				}
				delete this.text;
			}
		</function>
		<function name="filter" params="text">
			text = text.replace(/#.*\\[\n\r]?/gm, "");
			text = text.replace(/\s+\\\n\s+/gm, " ");
			text = text.replace(/\s+\\\r\s+/gm, " ");
			text = text.replace(/\s+\\\r\n\s+/gm, " ");
			text = text.replace(/\\\s+/gm, " ");
			return text;
		</function>
	</object>
	<object name="xsDebug" pattern="debug" prototype="xsCommon">
		<function name="crossReference" params="thePlatform, theMakefile">
			if (xsTool.both)
				xsCommon.crossReference.call(this, thePlatform, theMakefile.versions.debug);
			else if (xsTool.debug) 
				xsCommon.crossReference.call(this, thePlatform, theMakefile);
		</function>
	</object>
	<object name="xsRelease" pattern="release" prototype="xsCommon">
		<function name="crossReference" params="thePlatform, theMakefile">
			if (xsTool.both)
				xsCommon.crossReference.call(this, thePlatform, theMakefile.versions.release);
			else if (!xsTool.debug) 
				xsCommon.crossReference.call(this, thePlatform, theMakefile);
		</function>
	</object>
	<object name="xsInclude" pattern="include" prototype="xsMakefileItem">
	</object>
	<object name="xsImportMakefile" pattern="import" prototype="xsMakefileItem">
		<function name="crossReference" params="thePlatform, theMakefile">
			var aPath = xsTool.getPath(this.name);
			if (aPath) {
				if (this.verbose)
					this.report("Loading " + this.path + "...");
				var aMakeFile = xsTool.load(aPath);
				aMakeFile.crossReferenceItems(thePlatform, theMakefile);
			}
		</function>
	</object>
	<object name="xsOutput" pattern="output" prototype="xsMakefileItem">
	</object>
	<object name="xsVersion" pattern="version" prototype="xsConfiguration">
	</object>
	<object name="xsWrap" pattern="wrap" prototype="xsSource">
	</object>

</package>