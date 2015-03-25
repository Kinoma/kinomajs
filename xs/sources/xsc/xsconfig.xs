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
	<import href="xsTool.xs"/>
	<import href="xsconfigPackage.xs"/>
	<import href="xsconfigMakefile.xs"/>
	
	<patch prototype="xsTool">
		<null name="binPath"/>
		<boolean name="debug"/>
		<null name="grammarPath"/>
		<null name="inputPaths"/>
		<null name="outputPath"/>
		<null name="packages"/>
		<null name="platform"/>
		<null name="subPlatform"/>
		<null name="programName"/>
		<null name="scriptPaths"/>
		<null name="tmpPath"/>
		<null name="fhomePath"/>
		<null name="fskTmpPath"/>
		<null name="wrapPaths"/>
		<null name="productTarget"/>
		<null name="destdir"/>
		<boolean name="cmake" value="false"/>
		<boolean name="embed" value="false"/>
		<string name="application"/>
		<boolean name="verbose"/>

		<function name="initialize" params="theArguments">
			var anIndex, aName, aPath, aSplit;
			
			this.platform = this.getPlatform();
			for (anIndex = 1; anIndex < theArguments.length; anIndex++) {
				switch (theArguments[anIndex]) {
				case "-d":
					this.debug = true;
					break;
					
				case "-p":
					anIndex++;	
					var platforms = theArguments[anIndex].split(":");
					this.platform = platforms[0];
					if (platforms.length > 1)
						this.subPlatform = platforms[1];
					break;
					
				case "-v":
					this.verbose = true;
					break;
				
				case "-t":
					anIndex++;
					this.productTarget = theArguments[anIndex];
					break;
				
				case "-g":
					anIndex++;
					this.destdir = theArguments[anIndex];
					break;

				case "-x":
					anIndex++;
					this.cmake = true;
					break;

				case "-e":
					this.embed = true;
					break;

				case "-a":
					anIndex++;
					this.application = theArguments[anIndex];
					break;

				case "-f":
					anIndex++;
					this.fhomePath = theArguments[anIndex];
					break;

				default:
					aName = theArguments[anIndex];
					if (this.grammarPath)
						throw new Error("'" + aName + "': too many files!");
					aPath = this.resolvePath(aName, false);
					if (!aPath)
						throw new Error("'" + aName + "': file not found!");
					this.grammarPath = aPath;
					break;
				}
			}
			if (!this.grammarPath)
				throw new Error("Usage: xsconfig (-d) (-v) file");
		</function>

		<function name="run">
			var aSplit, aPath, aMakefile, aPackage, aCodeList, aStream;
			
			aSplit = this.splitPath(this.grammarPath);
			
			this.inputPaths = [aSplit[0]];
			this.outputPath = "";
			this.packages = [];
			this.programName = aSplit[1];
			this.scriptPaths = [];
			this.wrapPaths = [];
			
			aPath = this.makePath(aSplit[0], aSplit[1], ".mk");
			aPath = this.resolvePath(aPath, false);
			if (!aPath)
				throw new Error("-m '" + aSplit[1] + ".mk': makefile not found!");
			if (this.verbose)
				this.report("# Parsing '" + aPath + "'...");
			aMakefile = this.load(aPath);
			if (aMakefile == undefined)
				throw new Error("'" + aPath + "': invalid file!");
			if (this.cmake)
				aMakefile.crossReference(this.platform + "-cmake");
			else
				aMakefile.crossReference(this.platform);
			if (this.errorCount)
				throw new Error("" + this.errorCount + " error(s)!");
				
			aPackage = this.loadPackage(this.grammarPath);
			aPackage.target(null, true, "");
			if (this.errorCount)
				throw new Error("" + this.errorCount + " error(s)!");
				
			aPackage.makeReference();
			if (this.errorCount)
				throw new Error("" + this.errorCount + " error(s)!");

			if (this.destdir != null)
				this.outputPath = this.destdir;

			if (this.outputPath == "")
				this.outputPath = aSplit[0];
			aPath = this.makePath(this.outputPath, "bin", null);
			this.createDirectory(aPath);

			if (this.productTarget != null) {
				aPath = this.makePath(aPath, this.productTarget, null);
				this.createDirectory(aPath);
			}

			if (this.platform == "Windows") {
				aPath = this.makePath(aPath, "win32", null);
	  			this.createDirectory(aPath);
			}
			else if (this.platform == "MacOSX" || this.platform == "mac") {
				aPath = this.makePath(aPath, "mac", null);
				this.createDirectory(aPath);
			}
			else if (this.platform == "MacOSX64") {
				aPath = this.makePath(aPath, "mac64", null);
				this.createDirectory(aPath);
			}
			else if (this.platform == "iPhone") {
				aPath = this.makePath(aPath, (this.subPlatform == "simulator") ? "iphonesimulator" : "iphoneos", null);
				this.createDirectory(aPath);
			}

			if (this.subPlatform && (this.platform != "iPhone")) {
				aPath = this.makePath(aPath, this.subPlatform, null);
				this.createDirectory(aPath);
			}

			this.binPath = this.makePath(aPath, (this.debug) ? "debug" : "release", null);
			this.createDirectory(this.binPath);

			aPath = this.makePath(this.outputPath, "tmp", null);
			this.createDirectory(aPath);

			if (this.productTarget != null) {
				aPath = this.makePath(aPath, this.productTarget, null);
				this.createDirectory(aPath);
			}

			if (this.platform == "Windows") {
				aPath = this.makePath(aPath, "win32", null);
	  			this.createDirectory(aPath);
			}
			else if (this.platform == "MacOSX" || this.platform == "mac") {
				aPath = this.makePath(aPath, "mac", null);
				this.createDirectory(aPath);
			}
			else if (this.platform == "MacOSX64") {
				aPath = this.makePath(aPath, "mac64", null);
				this.createDirectory(aPath);
			}
			else if (this.platform == "iPhone") {
				aPath = this.makePath(aPath, (this.subPlatform == "simulator") ? "iphonesimulator" : "iphoneos", null);
				this.createDirectory(aPath);
			}

			if (this.subPlatform && (this.platform != "iPhone")) {
				aPath = this.makePath(aPath, this.subPlatform, null);
				this.createDirectory(aPath);
			}

			if (this.cmake) {
				this.tmpPath = this.makePath(aPath, (this.debug) ? "Debug" : "Release", null)
				this.createDirectory(this.tmpPath)
			}
			else {
				aPath = this.makePath(aPath, aSplit[1], null);
				this.createDirectory(aPath);
				this.tmpPath = this.makePath(aPath, (this.debug) ? "debug" : "release", null);
				this.createDirectory(this.tmpPath);
			}

			if (this.fhomePath) {

				if (this.platform == "android" || this.platform == "Linux" || this.platform == "MacOSX" || this.platform == "MacOSX64" || this.platform == "iPhone" || this.cmake) {
					if (this.productTarget != null)
						this.fskTmpPath = this.makePath(this.fhomePath, "tmp/" + this.productTarget, null);
					else if (this.platform == "MacOSX" || this.platform == "mac")
						this.fskTmpPath = this.makePath(this.fhomePath, "tmp/mac", null);
					else if (this.platform == "MacOSX64")
						this.fskTmpPath = this.makePath(this.fhomePath, "tmp/mac64", null);
					else if (this.platform == "iPhone")
						this.fskTmpPath = this.makePath(this.fhomePath, (this.subPlatform == "simulator") ? "tmp/iphonesimulator" : "tmp/iphoneos", null);
					if (this.subPlatform && (this.platform != "iPhone"))
						this.fskTmpPath += "/" + this.subPlatform;

					if (this.cmake) {
						if (this.debug)
							this.fskTmpPath += "/Debug";
						else
							this.fskTmpPath += "/Release";
					}
					else {
						this.fskTmpPath += "/fsk";
						if (this.debug)
							this.fskTmpPath += "/debug";
						else
							this.fskTmpPath += "/release";
					}

				}
				else {
					if (this.platform == "Windows")
						this.fskTmpPath = this.makePath(this.fhomePath, "tmp\\win32", null);

					if (this.subPlatform)
						this.fskTmpPath += "\\" + this.subPlatform;

					this.fskTmpPath += "\\fsk";

					if (this.debug)
						this.fskTmpPath += "\\debug";
					else
						this.fskTmpPath += "\\release";
				}
			}

			aMakefile.generate(this.platform, this.cmake);
		</function>
	</patch>
</package>