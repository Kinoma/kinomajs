<?xml version="1.0" encoding="UTF-8"?>
<!--
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
-->
<package>
	<object name="xsMakefileItem">
		<string name="name" pattern="@name"/>

		<function name="crossReference" params="thePlatform"/>
		<function name="write" params="theFile, thePlatform, theDebugFlag"/>
	</object>

	<object name="xsMakefile" pattern="/makefile">
		<array name="items" pattern="." contents="xsMakefileItem"/>

		<function name="crossReference" params="thePlatform">
			var aCount, anIndex;

			aCount = this.items.length;
			for (anIndex = 0; anIndex < aCount; anIndex++)
				this.items[anIndex].crossReference(thePlatform);
		</function>

		<function name="generate" params="thePlatform, useCMake">
			var aFile, anIndex, aCount, aPath, xsAntPath, xsAntFile;
			
			if (thePlatform == "Windows")
				aPath = xsTool.makePath(xsTool.inputPaths[0], xsTool.programName, ".mak");
			else if (useCMake) {
				thePlatform = thePlatform + "-cmake"
				aPath = xsTool.makePath(xsTool.tmpPath, xsTool.programName + ".buildfiles", null);
				xsTool.report("# Creating build directory '" + aPath + "'...");
				xsTool.createDirectory(aPath);
				xsAntPath = xsTool.makePath(aPath, "generate", ".xml");
				aPath = xsTool.makePath(aPath, "CMakeLists", ".txt");
			}
			else
				aPath = xsTool.makePath(xsTool.inputPaths[0], xsTool.programName, ".make");

			if (xsTool.verbose)
				xsTool.report("# Generating '" + aPath + "'...");
			aFile = new XSFile(aPath);

			if (useCMake) {
				xsAntFile = new XSFile(xsAntPath)
				this.generateAntFile(xsAntFile, thePlatform)
				xsAntFile.close();
			}

			this.generateHeader(aFile, thePlatform, useCMake);

			this.write(aFile, thePlatform);

			this.generateFooter(aFile, thePlatform, useCMake);

			aFile.close();
		</function>
		
		<function name="generateHeader" params="theFile, thePlatform, useCMake">
			var aCount, anIndex, aPackage, aSplit;

			if (useCMake) {
				this.generateCMake(theFile, thePlatform)
				this.writeBATFile(thePlatform, useCMake)
				return
			}

			theFile.write("PROGRAM = ");
			theFile.write(xsTool.programName);
			theFile.write("\n\n");
			
			theFile.write("GRAMMARS =");
			aCount = xsTool.packages.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				theFile.write(" \\\n\t");
				theFile.write(xsTool.packages[anIndex].__xs__path);
			}
			aCount = xsTool.scriptPaths.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				theFile.write(" \\\n\t");
				theFile.write(xsTool.scriptPaths[anIndex]);
			}
			theFile.write("\n\n");
			
			theFile.write("OBJECTS =");
			aCount = xsTool.packages.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				aPackage = xsTool.packages[anIndex];
				if (aPackage.linked) {
					aSplit = xsTool.splitPath(aPackage.__xs__path);
					if (xsTool.resolvePath(xsTool.makePath(aSplit[0], aSplit[1], "c"), false)) {
						theFile.write(" \\\n\t");
						theFile.write(xsTool.makePath(xsTool.tmpPath, aSplit[1], "o"));
					}
				}
			}
			aCount = xsTool.wrapPaths.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				aSplit = xsTool.splitPath(xsTool.wrapPaths[anIndex]);
				theFile.write(" \\\n\t");
				theFile.write(xsTool.makePath(xsTool.tmpPath, aSplit[1], "o"));
			}
			theFile.write("\n\n");
			
			theFile.write("BIN_DIR = ");
			theFile.write(xsTool.binPath);
			theFile.write("\n");
			theFile.write("TMP_DIR = ");
			theFile.write(xsTool.tmpPath);
			theFile.write("\n");
			
			aCount = xsTool.inputPaths.length;
			theFile.write("C_INCLUDES =");
			if (thePlatform == "Windows")
				for (anIndex = 0; anIndex < aCount; anIndex++) {
					theFile.write(" \\\n\t/I");
					theFile.write(xsTool.inputPaths[anIndex]);
				}
			else
				for (anIndex = 0; anIndex < aCount; anIndex++) {
					theFile.write(" \\\n\t-I");
					theFile.write(xsTool.inputPaths[anIndex]);
				}
			theFile.write("\n\n");

			if (xsTool.embed) {
				this.writeBATFile(thePlatform, useCMake);
			}
			else {
				theFile.write("XSC_INCLUDES =");
				for (anIndex = 0; anIndex < aCount; anIndex++) {
					theFile.write(" \\\n\t-i ");
					theFile.write(xsTool.inputPaths[anIndex]);
				}
				theFile.write("\n\n");
			}

			theFile.write("FSK_EXTENSION_EMBED = " + xsTool.embed + "\n");
			theFile.write("FSK_APPLICATION = " + xsTool.application + "\n");
			theFile.write("\n\n");
		</function>
		
		<function name="generateFooter" params="theFile, thePlatform, useCMake">
			var aCount, anIndex, aPackage, aSplit;

			if (useCMake) {
				if (xsTool.embed) {
					theFile.write("# embed -- \n\n")
					theFile.write("set_source_files_properties(${EXT_TMP}/" + xsTool.programName + ".xs.c PROPERTIES GENERATED true)\n\n")
					theFile.write("add_library(" + xsTool.programName + " OBJECT ${EXT_TMP}/" + xsTool.programName + ".xs.c ${sources})\n")
					theFile.write("\n")

					theFile.write("list(APPEND FskObjects $<TARGET_OBJECTS:" + xsTool.programName + ">)\n")
					theFile.write("set(FskObjects \"${FskObjects}\" PARENT_SCOPE)\n")
				}
				else {
					theFile.write("# not-embed -- \n")
					theFile.write("if (NOT DEFINED XSB_PATH)\n")
					theFile.write("\tset_source_files_properties(${EXT_TMP}/" + xsTool.programName + ".xs.c PROPERTIES GENERATED true)\n\n")
					theFile.write("\tadd_library(" + xsTool.programName + " SHARED ${EXT_TMP}/" + xsTool.programName + ".xs.c ${sources})\n")
					theFile.write("\tif (TARGET FskLib)\n")
					theFile.write("\t\ttarget_link_libraries(" + xsTool.programName + " ${EXTENSION_LINK_LIBS})\n")
					theFile.write("\telse ()\n")
					theFile.write("\t\ttarget_link_libraries(" + xsTool.programName + " Fsk ${EXTENSION_LINK_LIBS})\n")
					theFile.write("\tendif ()\n")
					theFile.write("\n")
					theFile.write("\tif (TARGET_DEPENDENCIES)\n")
					theFile.write("\t\tadd_dependencies(" + xsTool.programName + " ${TARGET_DEPENDENCIES})\n")
					theFile.write("\tendif ()\n")
					theFile.write("\n")
					theFile.write("\tif (NOT ${TARGET_PLATFORM} STREQUAL \"android\")\n")
					theFile.write("\t\tset_target_properties(" + xsTool.programName + " PROPERTIES PREFIX \"\" SUFFIX \".so\")\n")
					theFile.write("\tendif ()\n")
					theFile.write("\tinstall(TARGETS " + xsTool.programName + " DESTINATION ${BUILD_BIN})\n")
					theFile.write("endif ()\n")
	
				}
				theFile.write("\n")
				return
			}

			/*aCount = xsTool.inputPaths.length;
			if (thePlatform == "Windows") {
				for (anIndex = 0; anIndex < aCount; anIndex++) {
					theFile.write("{");
					theFile.write(xsTool.inputPaths[anIndex]);
					theFile.write("}.c{$(TMP_DIR)}.o:\n");
					theFile.write("\tcl $< $(ALL_C_OPTIONS) /Fo$@\n");
				}
			}
			else {
				theFile.write("VPATH = ");
				theFile.write(xsTool.inputPaths[0]);
				for (anIndex = 1; anIndex < aCount; anIndex++) {
					theFile.write(":");
					theFile.write(xsTool.inputPaths[anIndex]);
				}
				theFile.write("\n\n");
				theFile.write("$(OBJECTS): $(TMP_DIR)/%.o: %.c\n");
				theFile.write("\t$(CC) $< $(ALL_C_OPTIONS) -c -o $@\n");
			}*/
			
			aCount = xsTool.packages.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				aPackage = xsTool.packages[anIndex];
				if (aPackage.linked) {
					aSplit = xsTool.splitPath(aPackage.__xs__path);
					if (xsTool.resolvePath(xsTool.makePath(aSplit[0], aSplit[1], "c"), false)) {
						this.generateRule(theFile, thePlatform, aSplit[0], aSplit[1], "c", useCMake);
					}
				}
			}
			aCount = xsTool.wrapPaths.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				aSplit = xsTool.splitPath(xsTool.wrapPaths[anIndex]);
				this.generateRule(theFile, thePlatform, aSplit[0], aSplit[1], aSplit[2], useCMake);
			}
			
		</function>
		
		<function name="generateRule" params="theFile, thePlatform, theDirectory, theName, theExtension, useCMake">
			var aPath = xsTool.makePath(theDirectory, theName, theExtension)
			if (useCMake) {
				theFile.write("list(APPEND sources " + theName + "." + theExtension + ")\n")
			}
			else if (thePlatform == "Windows") {
				theFile.write("$(TMP_DIR)\\");
				theFile.write(theName);
				theFile.write(".o : ");
				theFile.write(aPath);
				theFile.write("\n\tcl ");
				theFile.write(aPath);
				theFile.write(" $(ALL_C_OPTIONS) /Fo$@\n");
			}
			else if ((thePlatform == "iPhone") && ((theExtension == ".c") || (theExtension == ".m"))) {
				theFile.write("$(TMP_DIR)/");
				theFile.write(theName);
				theFile.write(".o: ");
				theFile.write(aPath);
				theFile.write("\n\t$(CC) $< $(ALL_C_OPTIONS) -std=gnu99 -c -o $@\n");
			}
			else {
				theFile.write("$(TMP_DIR)/");
				theFile.write(theName);
				theFile.write(".o: ");
				theFile.write(aPath);
				theFile.write("\n\t$(CC) $< $(ALL_C_OPTIONS) -c -o $@\n");
			}
		</function>
		
		<function name="write" params="theFile, thePlatform">
			var aCount, anIndex;

			aCount = this.items.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				this.items[anIndex].write(theFile, thePlatform);
			}
		</function>

		<function name="writeBATFile" params="thePlatform, useCMake">
			var aCount, anIndex;
			var batPath;
			var amtRead;
			var contents;

			if (thePlatform == "android" || thePlatform == "Linux" || thePlatform == "MacOSX" || thePlatform == "MacOSX64" || thePlatform == "iPhone" || useCMake) {
				if (xsTool.verbose)
					xsTool.report("# writing '" + xsTool.fskTmpPath + "/xs.includes.txt" + "'...");
				batPath = xsTool.fskTmpPath + "/xs.includes.txt";
			}
			else
				batPath = xsTool.fskTmpPath + "\\xs.includes.txt";

			var xsBATFile = new XSFile(batPath, "a+");
			contents = xsBATFile.load();
			xsBATFile.close();
			xsBATFile = new XSFile(batPath, "a+");
	
			aCount = xsTool.inputPaths.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				if (-1 == contents.indexOf(xsTool.inputPaths[anIndex] + " ")) {
					xsBATFile.write("-i " + xsTool.inputPaths[anIndex] + " ");
					contents = contents + "-i " + xsTool.inputPaths[anIndex] + " ";
				}
			}

			xsBATFile.close();
		</function>

		<function name="generateCMake" params="theFile, thePlatform">
			theFile.write("cmake_minimum_required(VERSION 2.8)\n")
			theFile.write("# WARNING: This file is automatically generated by xsconfigMakefile (generateHeader).\n# Your edits will be lost.\n\n")
			theFile.write("\n")

			theFile.write("project(" + xsTool.programName + " C)\n")
			theFile.write("include_directories(${BUILD_TMP})\n")
			theFile.write("include_directories(${BUILD_TMP}/" + xsTool.programName + ")\n\n")

			theFile.write("set(EXT_TMP ${BUILD_TMP}/" + xsTool.programName + "/)\n\n")
			theFile.write("set(EXT_XS_FILE " + xsTool.packages[0].__xs__path + ")\n\n")

			aCount = xsTool.packages.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				theFile.write("list(APPEND grammars ")
				theFile.write(xsTool.packages[anIndex].__xs__path);
				theFile.write(")\n")
			}

			theFile.write("\n")
			aCount = xsTool.inputPaths.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				theFile.write("list(APPEND includes -i ");
				theFile.write(xsTool.inputPaths[anIndex]);
				theFile.write(")\n")
				theFile.write("include_directories(" + xsTool.inputPaths[anIndex] + ")\n")
			} 

			theFile.write("\n")
			aCount = xsTool.packages.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				aPackage = xsTool.packages[anIndex];
				if (aPackage.linked) {
					aSplit = xsTool.splitPath(aPackage.__xs__path);
					tryPath = xsTool.resolvePath(xsTool.makePath(aSplit[0], aSplit[1], "c"), false);
					// this attempts to build the <extensionName>.c file
					if (tryPath) {
						theFile.write("list(APPEND sources ");
						theFile.write(tryPath);
						theFile.write(")\n")
					}
				}

			}
			aCount = xsTool.wrapPaths.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				aSplit = xsTool.splitPath(xsTool.wrapPaths[anIndex]);
				theFile.write("list(APPEND sources " + xsTool.wrapPaths[anIndex] + ")\n")
			}
			theFile.write("\n");
			theFile.write("include_directories(${EXT_TMP})\n")

			theFile.write("\n")

			if (xsTool.embed)
				theFile.write("add_definitions(-DFSK_EXTENSION_EMBED=1)\n");
			else {
				theFile.write("add_definitions(-DFSK_EXTENSION_EMBED=0)\n");
			}
			theFile.write("\n\n");
		</function>

		<function name="generateAntFile" params="theFile, thePlatform">
			theFile.write('<project name="' + xsTool.programName + '">\n');
			theFile.write('\t<target name="generate">\n');
			theFile.write('\t\t<mkdir dir="${build.tmp}/' + xsTool.programName + '" />\n');
			theFile.write('\t\t<exec executable="${xs.tools.xsc}">\n');

			includes = "";
			aCount = xsTool.inputPaths.length;
			for (anIndex = 0; anIndex < aCount; anIndex++)
				includes = includes + " -i " + xsTool.inputPaths[anIndex];

			theFile.write('\t\t\t<arg line="-t XSCONFIG -o ${build.tmp}/' + xsTool.programName + ' -t ' + thePlatform + ' ' + includes + ' '  + xsTool.inputPaths[0] + '/' + xsTool.programName + '.xs" />\n');
			theFile.write('\t\t<' + '/exec>\n');
			theFile.write('\t<' + '/target>\n');
			theFile.write('<' + '/project>');
		</function>
	</object>
	
	<object name="xsInclude" pattern="include" prototype="xsMakefileItem">
		<undefined name="makefile"/>
		
		<function name="crossReference" params="thePlatform">
			if (!this.name)
				xsTool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
			else {
				var aPath = xsTool.searchPath(this.name, false);
				if (aPath) {
					if (xsTool.verbose)
						xsTool.report("# Parsing '" + aPath + "'...");
					this.makefile = xsTool.load(aPath);
					this.makefile.crossReference(thePlatform);
				}
				else
					xsTool.reportError(this.__xs__path, this.__xs__line, "makefile not found");
			}
		</function>
		
		<function name="write" params="theFile, thePlatform">
			if (this.makefile) {
				this.makefile.write(theFile, thePlatform);
				theFile.write("\n");
			}
		</function>
	</object>
	
	<object name="xsInput" pattern="input" prototype="xsMakefileItem">
		<function name="crossReference" params="thePlatform">
			if (!this.name)
				xsTool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
			else {
				var aPath = xsTool.searchPath(this.name, true);
				if (aPath)
					xsTool.inputPaths[xsTool.inputPaths.length] = aPath;
				else
					xsTool.reportError(this.__xs__path, this.__xs__line, "input directory not found");
			}
		</function>
	</object>
	
	<object name="xsOutput" pattern="output" prototype="xsMakefileItem">
		<function name="crossReference" params="thePlatform">
			if (!this.name)
				xsTool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
			else if (xsTool.outputPath) 
				xsTool.reportError(this.__xs__path, this.__xs__line, "too many output directories");
			else {
				var aPath = xsTool.searchPath(this.name, true);
				if (aPath)
					xsTool.outputPath = aPath;
				else
					xsTool.reportError(this.__xs__path, this.__xs__line, "output directory not found");
			}
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
	
	<object name="xsPlatform" pattern="platform" prototype="xsMakefileItem">
		<string name="common" pattern="common"/>
		<string name="debug" pattern="debug"/>
		<string name="release" pattern="release"/>
		<null name="platforms"/>
		<array name="items" pattern="." contents="xsMakefileItem"/>
		
		<function name="crossReference" params="thePlatform">
			var c, i;

			if (!this.name)
				xsTool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
			else
				this.platforms = this.name.split(",");

			c = this.platforms.length;
			for(i = 0; i < c; i++) {
				if (xsTool.trimString(this.platforms[i]) == thePlatform) {
					var c1, i1;
					
					c1 = this.items.length;
					for (i1 = 0; i1 < c1; i1++)
						this.items[i1].crossReference(thePlatform);
					break;
				}
			}
		</function>

		<function name="write" params="theFile, thePlatform">
			var c, i;
			c = this.platforms.length;
			for(i = 0; i < c; i++) {
				if (xsTool.trimString(this.platforms[i]) == thePlatform) {
					if (this.common) 
						theFile.write(this.common);
					if (this.debug && xsTool.debug) 
						theFile.write(this.debug);
					else if (this.release && !xsTool.debug) 
						theFile.write(this.release);
					theFile.write("\n");
				}
			}
		</function>
	</object>
	
	<object name="xsWrap" pattern="wrap" prototype="xsMakefileItem">
		<function name="crossReference" params="thePlatform">
			if (!this.name)
				xsTool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
			else {
				var aPath = xsTool.getPath(this.name);
				if (!aPath)
					xsTool.reportError(this.__xs__path, this.__xs__line, this.name + ": file not found");
				else
					xsTool.wrapPaths[xsTool.wrapPaths.length] = aPath;
			}
		</function>
	</object>

	<object name="xsBuildStyle" pattern="buildstyle" prototype="xsMakefileItem">
		<null name="styles"/>
		<array name="items" pattern="." contents="xsMakefileItem"/>
		
		<function name="crossReference" params="thePlatform">
			var c, i;

			if (!this.name)
				xsTool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
			else
				this.styles = this.name.split(",");

			c = this.styles.length;
			for (i = 0; i < c; i++) {
				var style = xsTool.trimString(this.styles[i]);
				if ((xsTool.embed && (style == "embed")) || 
					(!xsTool.embed && (style == "nonembed"))) {
					var c1, i1;
					
					c1 = this.items.length;
					for (i1 = 0; i1 < c1; i1++)
						this.items[i1].crossReference(thePlatform);
					break;
				}
			}
		</function>
		<function name="write" params="theFile, thePlatform">
			var c, i;
			c = this.styles.length;
			for (i = 0; i < c; i++) {
				var style = xsTool.trimString(this.styles[i]);
				if ((xsTool.embed && (style == "embed")) || 
					(!xsTool.embed && (style == "nonembed"))) {
					var aCount, anIndex;
					aCount = this.items.length;
					for (anIndex = 0; anIndex < aCount; anIndex++) {
						this.items[anIndex].write(theFile, thePlatform);
					}
				}
			}
		</function>
	</object>

</package>