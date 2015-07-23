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
	<namespace uri="http://www.kinoma.com/Fsk/1"/>
	<namespace prefix="build" uri="http://www.kinoma.com/Fsk/manifest/1"/>

	<import href="xsTool.xs"/>
	<import href="xsMakefile.xs"/>
	<import href="xsPackage.xs"/>

	<object name="kconfig">
		<object name="variable" pattern="variable">
			<string name="name" pattern="@name"/>
			<string name="value" pattern="@value"/>
			<string name="platform" pattern="@platform"/>
			<string name="application" pattern="@application"/>
			<string name="style" pattern="@build:style"/>
			<string name="resolved"/>
		</object>
		<object name="buildVariable" pattern="build:variable" prototype="kconfig.variable"/>
		<function name="Variable" params="name, value" prototype="kconfig.variable">
			this.name = name
			this.value = value
		</function>

		<object name="loadableType">
			<number name="unknown" value="0"/>
			<number name="extension" value="1"/>
			<number name="bytecode" value="2"/>
			<number name="document" value="3"/>
		</object>

		<object name="loadable">
			<string name="href" pattern="@href"/>
			<string name="from" pattern="@build:from"/>
			<boolean name="required" pattern="@required"/>
			<string name="platform" pattern="@platform"/>
			<string name="application" pattern="@build:application"/>
			<string name="embed" pattern="@build:embed"/>
			<string name="style" pattern="@build:style"/>
			<string name="name"/>
			<string name="vmName"/>
			<number name="type"/>
			<boolean name="needsBuild" value="true"/>
		</object>
		<object name="extension" prototype="kconfig.loadable" pattern="extension"/>
		<object name="bytecode" prototype="kconfig.loadable" pattern="bytecode"/>
		<object name="document" prototype="kconfig.loadable" pattern="document">
			<boolean name="needsBuild" value="false"/>
		</object>
		
		<object name="kind" pattern="kind">
			<string name="type" pattern="@name"/>
			<string name="messages" pattern="@messages" value="normal"/>
			<number name="flags" pattern="@flags"/>
		</object>
		<object name="instrument" pattern="instrument">
			<string name="log" pattern="@log"/>
			<string name="syslog" pattern="@syslog"/>
			<boolean name="trace" value="false" pattern="@trace"/>
			<boolean name="threads" value="false" pattern="@threads"/>
			<boolean name="times" value="false" pattern="@times"/>
			<boolean name="androidlog" value="false" pattern="@androidlog"/>
			<array name="kinds" contents="kconfig.kind" pattern="."/>
		</object>
		
		<object name="ssl" pattern="ssl">
			<string name="CA_list" pattern="CA_list/@href"/>
		</object>
		
		<object name="family" pattern="family">
			<string name="name" pattern="@name"/>
			<string name="uses" pattern="@uses"/>
		</object>
		<object name="font" pattern="font">
			<string name="name" pattern="@face"/>
			<number name="textSize" pattern="@size"/>		<!-- unused -->
			<string name="path" pattern="@href"/>
			<string name="engine" pattern="@engine"/>
			<string name="os" pattern="@os"/>
			<array name="family" contents="kconfig.family" pattern="."/>
		</object>
		<object name="ui" pattern="ui">
			<string name="platform" pattern="@platform"/>
			<array name="fonts" contents="kconfig.font" pattern="."/>
		</object>

		<object name="cOption" pattern="c">
			<string name="name" pattern="@option"/>
			<string name="platform" pattern="@platform"/>
		</object>

		<object name="asmOption" pattern="asm">
			<string name="name" pattern="@option" />
			<string name="platform" pattern="@platform"/>
		</object>

		<object name="xsdebug" pattern="xsdebug">
			<boolean name="enabled" value="false" pattern="@enabled"/>
		</object>

		<object name="feature" pattern="feature">
			<string name="name" pattern="@name"/>
			<string name="type" pattern="@type"/>
			<string name="value" pattern="@value"/>
			<string name="platform" pattern="@platform"/>
		</object>
		<object name="permission" pattern="permission">
			<string name="name" pattern="@name"/>
			<string name="platform" pattern="@platform"/>
		</object>
		<object name="version" pattern="versions">
			<string name="minimum" pattern="@minimum"/>
			<string name="target"  pattern="@target"/>
			<string name="platform" pattern="@platform"/>
		</object>
		<object name="options" pattern="options">
			<array name="features" contents="kconfig.feature" pattern="."/>
			<array name="permissions" contents="kconfig.permission" pattern="."/>
			<array name="versions" contents="kconfig.version" pattern="."/>
		</object>

		<object name="vm" pattern="vm">
			<string name="name" pattern="@name"/>
			<string name="alloc" pattern="@alloc"/>
			<array name="items" contents="kconfig.loadable" pattern="."/>
			<array name="uis" contents="kconfig.ui" pattern="."/>
			<array name="variables" contents="kconfig.variable" pattern="environment"/>
			<array name="options" contents="kconfig.options" pattern="."/>
			<reference name="instrument" contents="kconfig.instrument" pattern="."/>
			<reference name="ssl" contents="kconfig.ssl" pattern="."/>
		</object>
		<object name="rootVM" pattern="rootvm" prototype="kconfig.vm">
		</object>

		<object name="exclude" pattern="build:exclude">
			<string name="path" pattern="@path"/>
			<string name="pattern" pattern="@pattern"/>
			<string name="platform" pattern="@platform"/>
			<string name="application" pattern="@application"/>
		</object>

		<object name="install" pattern="build:install">
			<string name="sourcePath" pattern="@sourcePath"/>
			<string name="destinationPath" pattern="@destinationPath"/>
			<boolean name="selfRegister" value="false" pattern="@selfRegister"/>
			<string name="platform" pattern="@platform"/>
			<string name="application" pattern="@application"/>
		</object>

		<object name="uninstall" pattern="build:uninstall">
			<string name="destinationPath" pattern="@destinationPath"/>
			<string name="platform" pattern="@platform"/>
			<string name="application" pattern="@application"/>
		</object>

		<object name="copy" pattern="build:copy">
			<string name="sourcePath" pattern="@sourcePath"/>
			<string name="destinationPath" pattern="@destinationPath" value="."/>
			<string name="platform" pattern="@platform"/>
			<string name="application" pattern="@application"/>
		</object>

		<object name="registry" pattern="build:registry">
			<string name="entry" pattern="@entry"/>
			<string name="platform" pattern="@platform"/>
			<string name="application" pattern="@application"/>
		</object>

		<object name="cabsetup" pattern="build:cabsetup">
			<string name="sourcePath" pattern="@sourcePath"/>
			<string name="platform" pattern="@platform"/>
			<string name="application" pattern="@application"/>
		</object>

		<object name="root" pattern="/fsk">
			<array name="vm" contents="kconfig.vm" pattern="."/>
			<array name="exclude" contents="kconfig.exclude" pattern="."/>
			<array name="install" contents="kconfig.install" pattern="."/>
			<array name="uninstall" contents="kconfig.uninstall" pattern="."/>
			<array name="copy" contents="kconfig.copy" pattern="."/>
			<array name="registry" contents="kconfig.registry" pattern="."/>
			<array name="cabsetup" contents="kconfig.cabsetup" pattern="."/>
			<array name="cOptions" contents="kconfig.cOption" pattern="."/>
			<array name="asmOptions" contents="kconfig.asmOption" pattern="."/>
			<reference name="xsdebug" contents="kconfig.xsdebug" pattern="."/>
		</object>
		
	</object>
	
	<patch prototype="xsTool">
		<string name="application"/>
		<boolean name="debug"/>
		<boolean name="cmake"/>
		<string name="home"/>
		<boolean name="instrument"/>
		<boolean name="leak"/>
		<boolean name="make"/>
		<string name="makePath"/>
		<string name="manifestPath"/>
		<string name="outputPath"/>
		<string name="platform"/>
		<string name="resourcePath"/>
		<string name="slash" value="/"/>
		<null name="subplatform"/>
		<null name="variables"/>
		<boolean name="verbose"/>
		<boolean name="windows"/>
		<string name="xscOptions"/>
				
		<object name="configurationNames">
			<boolean name="debug" value="true"/>
			<boolean name="Debug" value="true"/>
			<boolean name="release" value="false"/>
			<boolean name="Release" value="false"/>
		</object>
		<object name="platformNames">
			<string name="android" value="android"/>
			<string name="Android" value="android"/>
			<string name="iphone" value="iphone"/>
			<string name="iPhone" value="iphone"/>
			<string name="iphoneos" value="iphone/device"/>
			<string name="iphonesimulator" value="iphone/simulator"/>
			<string name="linux" value="linux"/>
			<string name="Linux" value="linux"/>
			<string name="mac" value="mac"/>
			<string name="macosx" value="mac"/>
			<string name="MacOSX" value="mac"/>
			<string name="solaris" value="solaris"/>
			<string name="Solaris" value="solaris"/>
			<string name="win" value="win"/>
			<string name="Windows" value="win"/>
		</object>
		
		<function name="checkPlatform" params="string">
			if (!string)
				return true;
			var platforms = string.split(",");
			var c = platforms.length;
			for (var i = 0; i < c; i++) {
				var platform = platforms[i];
				var parts = platform.split("/");
				if (parts[0] != this.platform)
					continue;
				if (parts.length > 1) {
					if (parts[1] != this.subplatform)
						continue;
				}
				return true;
			}
			return false;
		</function>
		
		<function name="createDirectories" params="path, name, flag">
			this.createDirectory(path);
			path += this.slash + name;
			this.createDirectory(path);
			path += this.slash + this.platform;
			this.createDirectory(path);
			if (this.subplatform) {
				path += this.slash + this.subplatform;
				this.createDirectory(path);
			}
			if (!this.both) {
				if (this.debug) 
					path += this.slash + "debug";
				else
					path += this.slash + "release";
			}
			this.createDirectory(path);
			if (flag) {
				path += this.slash + this.application;
				this.createDirectory(path);
			}
			return path;
		</function>

		<function name="initialize" params="theArguments">
			var aName, aPath, outputPath;
			
			this.action = "all";
			this.application = "";
			this.binPath = "";
			this.both = false;
			this.cmake = false;
			this.debug = false;
			this.home = this.resolvePath(this.getEnvironmentValue("F_HOME"), true);
			this.instrument = false;
			this.jobs = 1;
			this.leak = false;
			this.make = false;
			this.makePath = "";
			this.manifestPath = "";
			this.platform = "";
			this.subplatform = "";
			this.tmpPath = "";
			this.variables = [];
			this.verbose = false;
			this.xsdebug = false;
			this.windows = "Windows" == this.getPlatform();
			outputPath="";
			if (!this.home)
				throw new Error("F_HOME undefined!");
			if (this.windows) {
				this.slash = "\\";
				this.home = this.home.replace("\\", "/");
			}
			for (var anIndex = 1; anIndex < theArguments.length; anIndex++) {
				switch (theArguments[anIndex]) {
				case "-a":
					anIndex++;	
					aName = theArguments[anIndex];
					if (this.application != "")
						throw new Error("-a '" + aName + "': too many -a application!");
					this.application = aName;
					break;

				case "-b":
					anIndex++;	
					aName = theArguments[anIndex];
					if (this.binPath != "")
						throw new Error("-b '" + aName + "': too many -b directory!");
					aPath = this.ensureDirectory(aName, true);
					if (aPath)
						this.binPath = aPath;
					else {
						throw new Error("-b '" + aName + "': directory not found!");
					}
					break;

				case "-c":
					anIndex++;	
					aName = theArguments[anIndex];
					if (aName in this.configurationNames)
						this.debug = this.configurationNames[aName];
					else
						throw new Error("-c '" + aName + "': unknown configuration!");
					break;

				case "-d":
					this.debug = true;
					break;
					
				case "-i":
					this.instrument = true;
					break;
					
				case "-l":
					this.leak = true;
					break;

				case "-m":
					this.make = true;
					if ((anIndex + 1) < theArguments.length) {
						aName = theArguments[anIndex + 1];
						aPath = this.resolvePath(aName, false);
						//if ((aName == "all") || (aName == "clean")) {
						if(!aPath && (aName.charAt(0) != '-')) {
							this.action = aName;
							anIndex++;	
						}
					}
					break;
					
				case "-o":
					anIndex++;	
					aName = theArguments[anIndex];
					if (outputPath != "")
						throw new Error("-o '" + aName + "': too many -o directory!");
					aPath = this.resolvePath(aName, true);
					if (aPath)
						outputPath = aPath;
					else
						throw new Error("-o '" + aName + "': directory not found!");
					break;
				
				case "-p":
					anIndex++;	
					aName = theArguments[anIndex].toLowerCase();;
					if (this.makePath != "")
						throw new Error("-p '" + aName + "': too many -p platform!");
					if (aName in this.platformNames)
						aName = this.platformNames[aName];
					aPath = this.resolvePath(this.home + "/kinoma/kpr/cmake/" + aName, true);
					if (aPath)
						this.makePath = aPath;
					else
						throw new Error("-p '$(F_HOME)/kinoma/kpr/cmake/" + aName + "': directory not found!");
					aSplit = aName.split("/");
					this.platform = aSplit[0];
					if (aSplit.length > 1)
						this.subplatform = aSplit[1];
					break;
					
				case "-t":
					anIndex++;	
					aName = theArguments[anIndex];
					if (this.tmpPath != "")
						throw new Error("-t '" + aName + "': too many -t directory!");
					aPath = this.ensureDirectory(aName, true);
					if (aPath)
						this.tmpPath = aPath;
					else {
						throw new Error("-t '" + aName + "': directory not found!");
					}
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
					
				case "-x2":
					this.cmake = true;
					this.both = true;
					break;

				case "-j":
					anIndex++;	
					aName = theArguments[anIndex];
					if (this.jobs != 1) 
						throw new Error("-j '" + aName + "': too many -j thread!");
					this.jobs = aName;
					break;

				default:
					aName = theArguments[anIndex];
					if (this.manifestPath)
						throw new Error("'" + aName + "': too many files!");
					aPath = this.resolvePath(aName, false);
					if (aPath)
						this.manifestPath = aPath;
					else
						throw new Error("'" + aName + "': file not found!");
					break;
				}
			}
			if (!this.makePath) {
				aName = this.getPlatform();
				if (aName in this.platformNames)
					aName = this.platformNames[aName];
				aPath = this.resolvePath(this.home + "/kinoma/kpr/cmake/" + aName, true);
				if (aPath)
					this.makePath = aPath;
				else
					throw new Error("-p '$(F_HOME)/kinoma/kpr/cmake/" + aName + "': directory not found!");
				this.platform = aName;
			}
			if (!this.manifestPath) {
				aName = "manifest.xml";
				aPath = this.resolvePath(aName, false);
				if (aPath)
					this.manifestPath = aPath;
				else
					throw new Error("'" + aName + "': file not found!");
			}
			if (!this.application) {
				var parts = this.splitPath(this.manifestPath);
				this.resourcePath = parts[0] + this.slash + this.platform;
				parts = this.splitPath(parts[0]);
				this.application = parts[1];
			}
			if (!this.binPath || !this.tmpPath) {
				if (!outputPath)
					outputPath = this.home;
				if (!this.binPath)
					this.binPath = this.createDirectories(outputPath, "bin", this.platform != "mac");
				if (!this.tmpPath)
					this.tmpPath = this.createDirectories(outputPath, "tmp", true);
			}
			this.platformNames[this.platform] = this.platform;
		</function>
		
		<function name="recurse" params="sourcePath, destinationPath, flag"><![CDATA[
			if (sourcePath in this.excludes)
				return;
			this.excludes[sourcePath] = true;
			if (flag) {
				if (destinationPath)
					this.directoryPaths.push(destinationPath);
				var items = this.listDirectory(sourcePath);
				var c = items.length;
				for (var i = 0; i < c; i++) {
					var item = items[i];
					this.recurse(this.joinPath(sourcePath, item.name), destinationPath + this.slash + item.name, item.directory);
				}
			}
			else {
				var parts = this.splitPath(sourcePath);
				switch (parts[2]) {
				case ".xml":
					if (xsTool.isKPR(sourcePath))
						this.xmlPaths.push({
							sourcePath: sourcePath,
							destinationPath: destinationPath.slice(0, -4),
						});
					else
						this.otherPaths.push({
							sourcePath: sourcePath,
							destinationPath: destinationPath,
						});
					break;
				case ".js":
					this.jsPaths.push({
						sourcePath: sourcePath,
						destinationPath: destinationPath.slice(0, -3),
					});
					break;
				default:
					this.otherPaths.push({
						sourcePath: sourcePath,
						destinationPath: destinationPath,
					});
					break;
				}
			}
		]]></function>
		
		<function name="run"><![CDATA[
			if (this.verbose)
				this.report("Configuring " + this.application  + " for platform " + this.platform + "...");
			var config = this.load(this.manifestPath, 1 | 2 | 4 | 128)
			
			var variableTable = {
				applicationPath: "[applicationPath]",
				F_HOME: this.home,
				KDT_HOME: this.getEnvironmentValue("KDT_HOME"),
				F_HOST: this.getIPAddress(),
			};
			var environmentTable = {
				grammarName: ((this.platform == "android") ? "res/raw/kinoma.jet/" : "") + this.application + ".xsb",
			};
			var instrument;
			var fonts = [];
			var features = [];
			var permissions = [];
			var minimumVersion;
			var targetVersion;
			var certificate;

			var vms = config.vm;
			var c = vms.length;
			for (var i = 0; i < c; i++) {
				var vm = vms[i]
				var variables = vm.variables;
				var d = variables.length;
				for (var j = 0; j < d; j++) {
					var variable = variables[j];
					if (variable.platform == "build")
						variableTable[variable.name] = this.applyVariables(variable.value, variableTable);
					else if (this.checkPlatform(variable.platform)) {
						variableTable[variable.name] = this.applyVariables(variable.value, variableTable);
						environmentTable[variable.name] = variableTable[variable.name];
					}
				}
				if (vm.instrument != kconfig.instrument) 
					instrument = vm.instrument;
				var uis = vm.uis;
				var d = uis.length;
				for (var j = 0; j < d; j++) {
					var ui = uis[j]
					if (this.checkPlatform(ui.platform))
						fonts = fonts.concat(ui.fonts);
				}
				// Handle Android/permissions/features/version
				var defaultVersionsPath = this.resolvePath(this.home + "/kinoma/kpr/cmake/" + this.platform + "/versions.properties", false);
				if (defaultVersionsPath) {
					var defaultVersions = this.readString(defaultVersionsPath).trim();
					var versions = defaultVersions.split('\n');
					var d = versions.length;
					for (var j = 0; j < d; j++) {
						version = versions[j].split('=');
						switch(version[0]) {
							case "version.minimum":
								minimumVersion = version[1];
								break;
							case "version.target":
								targetVersion = version[1];
								break;
						}
					}
				}
				var options = vm.options;
				var d = options.length;
				for (var j = 0; j < d; j++) {
					var option = options[j];
					var e = option.features.length;
					for (var k = 0; k < e; k++) {
						var feature = option.features[k];
						if (this.checkPlatform(feature.platform))
							features = features.concat(feature);
					}
					var e = option.permissions.length;
					for (var k = 0; k < e; k++) {
						var permission = option.permissions[k];
						if (this.checkPlatform(permission.platform))
							permissions = permissions.concat(permission);
					}
					var e = option.versions.length;
					for (var k = 0; k < e; k++) {
						var version = option.versions[k];
						if (this.checkPlatform(version.platform)) {
							// Ensure that we don't override the default values
							if (version.minimum)
								minimumVersion = version.minimum;
							if (version.target)
								targetVersion = version.target;
						}
					}
				}
				if (vm.ssl.CA_list) 
					certificate = environmentTable.CA_list = vm.ssl.CA_list;
			}

			var defaultPermissionsPath = this.resolvePath(this.home + "/kinoma/kpr/cmake/" + this.platform + "/permissions.cfg", false);
			if (defaultPermissionsPath) {
				var defaultPermissions = this.readString(defaultPermissionsPath).trim();
				var tempPermissions = defaultPermissions.match(/^[^=]+/gm);
				var d = tempPermissions.length;
				for (var j = 0; j < d; j++) {
					var tempPermission = xs.newInstanceOf(kconfig.permission);
					tempPermission.name = tempPermissions[j];
					tempPermission.platform = this.platform;
					permissions = permissions.concat(tempPermission);
				}
			}
			var defaultFeaturesPath = this.resolvePath(this.home + "/kinoma/kpr/cmake/" + this.platform + "/features.cfg", false);
			if (defaultFeaturesPath) {
				var defaultFeatures = this.readString(defaultFeaturesPath).trim();
				var tempFeatures = defaultFeatures.split(/\n/);
				var d = tempFeatures.length;
				for (var j = 0; j < d; j++) {
					var tempFeature = xs.newInstanceOf(kconfig.feature);
					var featureItems = tempFeatures[j];
					var items = featureItems.split(',');
					var e = items.length;
					for (var k = 0; k < e; k++) {
						var item = items[k].split('=');
						switch(item[0]) {
							case 'name':
								tempFeature.name = item[1];
								break;
							case 'value':
								tempFeature.value = item[1];
								break;
							case 'type':
								tempFeature.type = item[1];
								break;
							default:
								break;
						}
					}
					features = features.concat(tempFeature);
				}
			}
			// not mandatory for all KPR apps
			// if (!("modulePath" in environmentTable))
			// 	this.reportError(config.__xs__path, config.__xs__line, "no modulePath variable!");
			if (!("shellPath" in environmentTable))
				this.reportError(config.__xs__path, config.__xs__line, "no shellPath variable!");

			var FskPlatform = xs.newInstanceOf(kconfig.extension);
			FskPlatform.href = "FskPlatform";
			FskPlatform.from = this.joinPath(this.makePath, "FskPlatform.mk");
			var FskCore = xs.newInstanceOf(kconfig.extension);
			FskCore.href = "FskCore";
			FskCore.from = this.resolvePath(this.home + "/kinoma/kpr/patches/FskCore.mk", false);
			var blocks = [ FskPlatform, FskCore ];

			var vms = config.vm;
			var c = vms.length;
			for (var i = 0; i < c; i++) {
				var vm = vms[i];
				var items = vm.items;
				var d = items.length;
				for (var j = 0; j < d; j++) {
					var item = items[j]
					if (this.checkPlatform(item.platform)) {
						if (item.needsBuild) {
							item.from = this.applyVariables(item.from, variableTable)
							// don't add duplicates
							for (var k = 0; k < blocks.length; k++) {
								if (item.from == blocks[k].from) {
									item = null
									break
								}
							}
							if (item)
								blocks.push(item);
						}
					}
				}
			}
			
			this.excludes = {};
			this.directoryPaths = [];
			this.xmlPaths = [];
			this.jsPaths = [];
			this.otherPaths = [];
			this.excludes[this.manifestPath] = true;
			var items = config.exclude;
			var c = items.length;
			for (var i = 0; i < c; i++) {
				var item = items[i];
				if (this.checkPlatform(item.platform)) {
					var path = this.applyVariables(items[i].path, variableTable);
					this.excludes[this.resolvePath(path)] = true;
				}
			}
			var offset = this.home.length;
			var items = config.copy;
			var c = items.length;
			for (var i = 0; i < c; i++) {
				var item = items[i];
				if (this.checkPlatform(item.platform)) {
					var sourcePath = this.applyVariables(item.sourcePath, variableTable);
					var destinationPath = this.applyVariables(item.destinationPath, variableTable);
					var path = this.resolvePath(sourcePath, true);
					var flag = true;
					if (!path) {
						path = this.resolvePath(sourcePath, false);
						if (path) {
							// @@ sourcePath is a not fold
							flag = false;
							var parts = this.splitPath(destinationPath);
							if (!parts[2]) { // @@ no extension, it is a directory!
								parts = this.splitPath(path);
								destinationPath = this.joinPath(destinationPath, parts[1], parts[2]);
							}
						}
						else
							throw new Error(item.__xs__path + ": line "+ item.__xs__line + " '" + item.sourcePath + "': not found!");
					}
					if (path in this.excludes) {
						var excludeItem;
						for (var j = 0; j < config.exclude.length; j++) {
							var exclude = config.exclude[j];
							var excludePath = this.applyVariables(exclude["path"], variableTable);
							if (path == excludePath)
								excludeItem = exclude;
						}
						throw new Error(item.__xs__path + ": line "+ item.__xs__line + " '" + item.sourcePath + "': this has been excluded already on line " + excludeItem.__xs__line + "!");
					}

					// ensure destination directory
					var parts = this.splitPath(destinationPath);
					var directory = parts[0];
					if( (parts[2]) && (flag)) //destination is a file, but source is a fold
						throw new Error(item.__xs__path + ": line "+ item.__xs__line + " '" + item.destinationPath + "': should be a fold!");
					if (directory) {
						var paths = this.directoryPaths;
						var j, d = paths.length;
						if (d) {
							for (j = 0; j < d; j++) {
								if (paths[j] == directory)
									break;
							}
							if (j == d)
								this.directoryPaths.push(parts[0]);
						}
					}
					if (path)
						this.recurse(path, destinationPath, flag);
					else
						this.reportError(item.__xs__path, item.__xs__line, "'" + item.sourcePath + "': not found!");
				}
			}
			this.libraries = [];
			this.packages = [];
			this.sources = {};
			
			var makefiles = [];
			var patchesPath = this.resolvePath(this.home + "/kinoma/kpr/patches", true);
			var xscIncludePaths = [];
			
			var c = blocks.length;
			for (var i = 0; i < c; i++) {
				var block = blocks[i];
				var path = this.resolvePath(block.from, false);
				if (path) {
					if (this.verbose)
						this.report("Loading " + block.from  + "...");
					var split = this.splitPath(block.from);
					this.currentPath = split[0];
					this.inputPaths = [ 
						this.tmpPath,
						patchesPath,
						this.currentPath,
						this.joinPath(this.currentPath, "sources"),
					];
					var makefile = this.load(block.from);
					makefile.separate = ((block.platform == "android") && (block.embed != "true"));
					makefile.crossReference(this.platform, this.currentPath, split[1]);
					makefile.mkPath = path;
					
					path = this.getPath(makefile.name + ".xs");
					if (path) {
						var j = this.packages.length;		
						var package = this.loadPackage(path);
						if (package.items.length) {
							makefile.xsPath = path;
							package.target(null, true, "");
							var packages = this.packages;
							var d = packages.length;
							for (; j < d; j++) {
								var package = packages[j];
								split = xsTool.splitPath(package.__xs__path);
								this.insertUnique(xscIncludePaths, split[0]);
								path = xsTool.resolvePath(xsTool.joinPath(split[0], split[1], "c"), false);
								if (path) {
									if ((path in this.sources) && !makefile.separate) {
										if (this.verbose)
											this.report("Ignoring " + path + "...");
									}
									else {
										this.sources[path] = true;
										makefile.sources.push(new XSSource(split[1], ".c", path));
									}
								}
							}
						}
						else {
							split = xsTool.splitPath(package.__xs__path);
							path = xsTool.resolvePath(xsTool.joinPath(split[0], split[1], "c"), false);
							if (path) {
								if ((path in this.sources) && !makefile.separate) {
									if (this.verbose)
										this.report("Ignoring " + path + "...");
								}
								else {
									this.sources[path] = true;
									makefile.sources.push(new XSSource(split[1], ".c", path));
								}
							}
							this.packages.length = j;
							makefile.xsPath = null;
							if (xsTool.verbose)
								xsTool.report("Ignoring " + path + "...");
						}
					}
					
					makefiles.push(makefile);
				}
				else
					this.reportError(block.__xs__path, block.__xs__line, "'" + block.from + "': makefile not found!");
			}
			
			if (this.errorCount)
				throw new Error("" + this.errorCount + " error(s)!");

			/* C */
			this.generateFskPlatform();

			aPath = this.joinPath(this.tmpPath, "FskManifest", "c");
			aFile = new XSFile(aPath);
			aFile.write('/* WARNING: This file is automatically generated by kprconfig. Do not edit. */\n');
			aFile.write('#include "Fsk.h"\n');
			aFile.write('#include "FskECMAScript.h"\n');
			aFile.write('#include "FskEnvironment.h"\n');
			aFile.write('#include "FskExtensions.h"\n');
			aFile.write('#include "FskFiles.h"\n');
			aFile.write('#include "FskHardware.h"\n');
			aFile.write('#include "FskInstrumentation.h"\n');
			aFile.write('#include "FskMemory.h"\n');
			aFile.write('#include "FskString.h"\n');
			aFile.write('#define __FSKTEXT_PRIV__ 1\n');
			aFile.write('#include "FskText.h"\n');
			aFile.write('#include "xs.h"\n');
			aFile.write('\n');
			aFile.write('extern FskErr FskExtensionLoad(const char *name);\n');
			aFile.write('extern FskErr FskExtensionUnload(const char *name);\n');
			var c = makefiles.length;
			for (var i = 0; i < c; i++) {
				var makefile = makefiles[i];
				if (!makefile.separate) {
					var aName = makefile.name;
					aFile.write('extern FskErr ');
					aFile.write(aName);
					aFile.write('_fskLoad(FskLibrary library);\n');
					aFile.write('extern FskErr ');
					aFile.write(aName);
					aFile.write('_fskUnload(FskLibrary library);\n');
				}
			}
			aFile.write('\n');
			aFile.write('void FskExtensionsEmbedLoad(char *vmName)\n');
			aFile.write('{\n');
			aFile.write('\tchar* value;\n');
//			aFile.write('\tFskTextEngine fte;\n');
			
			if (instrument) {
				aFile.write('#if SUPPORT_INSTRUMENTATION\n');
				aFile.write('\tFskInstrumentationSimpleClientConfigure(');
				aFile.write(instrument.trace ? '1, ' : '0, ');
				aFile.write(instrument.threads ? '1, ' : '0, ');
				aFile.write(instrument.times ? '1, "' : '0, "');
				aFile.write(instrument.log);
				aFile.write('", "');
				aFile.write(instrument.syslog);
				aFile.write(instrument.androidlog ? '", 1);\n' : '", 0);\n');
				var kinds = instrument.kinds;
				var c = kinds.length;
				for (var i = 0; i < c; i++) {
					var kind = kinds[i];
					aFile.write('\tFskInstrumentationSimpleClientAddType("');
					aFile.write(kind.type);
					aFile.write('", (');
					aFile.write(kind.flags);
					switch (kind.messages) {
					case "normal": aFile.write('<< 16) | kFskInstrumentationLevelUpToNormal);\n'); break;
					case "minimal": aFile.write('<< 16) | kFskInstrumentationLevelUpToMinimal);\n'); break;
					case "verbose": aFile.write('<< 16) | kFskInstrumentationLevelUpToVerbose);\n'); break;
					case "debug": aFile.write('<< 16) | kFskInstrumentationLevelUpToDebug);\n'); break;
					default:  aFile.write('<< 16));\n'); break;
					}
				}
				aFile.write('#endif\n');
			}

			/* Grab core version from build.properties */
			var buildPropertiesPath = this.home + "/kinoma/kpr/cmake/build.properties";
			var buildProperties = this.readString(buildPropertiesPath);
			if (!buildProperties)
				xsTool.reportError(null, 0, "Unable to read " + buildPropertiesPath);
			var coreVersion = buildProperties.match(/core\.version=([0-9.]*)/);
			if (coreVersion[1])
				environmentTable["CORE_VERSION"] = coreVersion[1];

			/* Set a default version if none specified in manifest */
			if (!environmentTable["VERSION"])
				environmentTable["VERSION"] = "1.00";

			for (var i in environmentTable) {
				aFile.write('\tvalue = FskEnvironmentDoApply(FskStrDoCopy("');
				aFile.write(environmentTable[i]);
				aFile.write('"));\n');
				aFile.write('\tFskEnvironmentSet("');
				aFile.write(i);
				aFile.write('", value);\n');
				aFile.write('\tFskMemPtrDispose(value);\n');
			}
			
			if ((this.platform == "android") || (this.platform == "linux") || (this.platform == "threadx") || (this.platform == "win")) {
				var c = fonts.length;
				var fontName;	
				for (var i = 0; i < c; i++) {
					var font = fonts[i];
					if (!font.os) {
						aFile.write('\tFskTextFreeTypeInstallFonts("');
						aFile.write(font.path);
						if (font.name) {
							if (fontName)
								this.reportError(null, 0, "'" + font.name + "': too many default fonts!");
							else
								fontName = font.name;
							aFile.write('", "');
							aFile.write(font.name);
							aFile.write('");\n');
						}
						else
							aFile.write('", NULL);\n');
					}
				}
				if ((this.platform == "linux") || (this.platform == "threadx")) {
					if (!fontName)
						this.reportError(null, 0, "No default font found!");
				}
				else if (this.platform == "android") {
					aFile.write('\tFskTextFreeTypeInstallFonts("/system/fonts/", NULL);\n');
				}
			}
	
			var c = makefiles.length;
			for (var i = 0; i < c; i++) {
				var makefile = makefiles[i];
				var aName = makefile.name;
				if (!makefile.separate) {
					aFile.write('\t');
					aFile.write(aName);
					aFile.write('_fskLoad(NULL);\n');
				}
				else {
					aFile.write('\tFskExtensionLoad("');
					aFile.write(aName);
					aFile.write('");\n');
				}
			}
			
			aFile.write('}\n');
			aFile.write('\n');
			aFile.write('void FskExtensionsEmbedUnload(char *vmName)\n');
			aFile.write('{\n');
			var c = makefiles.length;
			for (var i = 0; i < c; i++) {
				var makefile = makefiles[i];
				var aName = makefile.name;
				if (!makefile.separate) {
					aFile.write('\t');
					aFile.write(aName);
					aFile.write('_fskUnload(NULL);\n');
				}
				else {
					aFile.write('\tFskExtensionUnload("');
					aFile.write(aName);
					aFile.write('");\n');
				}
			}
			aFile.write('}\n');
			aFile.close();

			/* version - to set minimum and maximum versions for Android/iOS */
			if (this.platform == "android" || this.platform == "iphone" || this.platform == "mac") {
				if (minimumVersion || targetVersion) {
					aPath = this.joinPath(this.tmpPath, "versions", "properties");
					aFile = new XSFile(aPath);
					if (minimumVersion) {
						aFile.write("version.minimum="); aFile.write(minimumVersion); aFile.write("\n");
					}
					if (targetVersion) {
						aFile.write("version.target="); aFile.write(targetVersion); aFile.write("\n");
					}
					aFile.close();
				}
			}

			/* features  - only for Android/iOS currently */
			var c = features.length;
			if (c > 0) {
				aPath = this.joinPath(this.tmpPath, "features", "txt");
				aFile = new XSFile(aPath);
				for (var i = 0; i < c; i++) {
					feature = features[i];
					if (this.platform == "android") {
						aFile.write('\t<uses-feature android:name="');
						aFile.write(feature.name);
						if (feature.value) {
							aFile.write('" android:required="');
							aFile.write(feature.value);
						}
						aFile.write('"/>\n');
					}
					else if (this.platform == "iphone") {
						aFile.write("\t<key>" + feature.name + "</key>\n");
						aFile.write("\t<" + feature.type + ">");
						if (feature.type == "array") {
							values = feature.value.split(",");
							var d = values.length;
							for (var j = 0; j < d; j++)
								aFile.write("\n\t\t<string>" + values[j] + "</string>");
							aFile.write("\n\t");
						}
						else
							aFile.write(feature.value);
						aFile.write("</" + feature.type + ">\n");
					}
				}
				aFile.close();
			}
			
			/* permissions - only for android currently */
			if (this.platform == "android") {
				var useGCM = ("remoteNotification" in environmentTable) && (environmentTable["remoteNotification"] != "0");
				if (useGCM) {
					/* INTERNET and WAKE_LOCK are defined by default */
					var tempPermission = xs.newInstanceOf(kconfig.permission);
					tempPermission.name = "GET_ACCOUNTS";
					tempPermission.platform = this.platform;
					permissions = permissions.concat(tempPermission);
				}
				var c = permissions.length;
				if (c > 0) {
					aPath = this.joinPath(this.tmpPath, "permissions", "txt");
					aFile = new XSFile(aPath);
					for (var i = 0; i < c; i++) {
						permission = permissions[i];
						aFile.write('\t<uses-permission android:name="android.permission.');
						aFile.write(permission.name);
						aFile.write('"/>\n');
					}
					if (useGCM) {
						/* additional permissions */
						aFile.write('\t<uses-permission android:name="com.google.android.c2dm.permission.RECEIVE" />\n');
						aFile.write('\t<uses-permission android:name="com.kinoma.kinomaplay.permission.C2D_MESSAGE" />\n');
						aFile.write('\t<permission\n');
						aFile.write('\t\tandroid:name="com.kinoma.kinomaplay.permission.C2D_MESSAGE"\n');
						aFile.write('\t\tandroid:protectionLevel="signature" />\n');
					}
					aFile.close();
					aPath = this.joinPath(this.tmpPath, "permissions", "cfg");
					aFile = new XSFile(aPath);
					for (var i = 0; i < c; i++) {
						permission = permissions[i];
						aFile.write(permission.name);
						aFile.write('=true\n');
					}
					if (useGCM) {
						aFile.write("C2D_MESSAGE=true\n");
					}
					aFile.close();
				}

				aPath = this.joinPath(this.tmpPath, "modules", "txt");
				aFile = new XSFile(aPath);
				if (useGCM) {
					aFile.write('\t\t<receiver\n');
					aFile.write('\t\t\tandroid:name=".GcmBroadcastReceiver"\n');
					aFile.write('\t\t\tandroid:permission="com.google.android.c2dm.permission.SEND" >\n');
					aFile.write('\t\t\t<intent-filter>\n');
					aFile.write('\t\t\t\t<action android:name="com.google.android.c2dm.intent.RECEIVE" />\n');
					aFile.write('\t\t\t\t<category android:name="com.kinoma.kinomaplay" />\n');
					aFile.write('\t\t\t</intent-filter>\n');
					aFile.write('\t\t</receiver>\n');
					aFile.write('\t\t<service android:name=".GcmIntentService" />\n');
					aFile.write('\t\t<meta-data android:name="com.google.android.gms.version"\n');
					aFile.write('\t\t\tandroid:value="@integer/google_play_services_version" />\n');
				} else {
					// not to be empty
					aFile.write('\n');
				}
				aFile.close();
			}
			
			/* XS */
			aPath = this.joinPath(this.tmpPath, "FskManifest", "xs");
			aFile = new XSFile(aPath);
			aFile.write('<!-- WARNING: This file is automatically generated by kprconfig. Do not edit. -->\n');
			aFile.write('<?xml version="1.0" encoding="UTF-8"?>\n');
			aFile.write("<package>\n");
			var c = makefiles.length;
			for (var i = 0; i < c; i++) {
				var aMakefile = makefiles[i];
				if (aMakefile.xsPath) {
					aFile.write('\t<import href="');
					aFile.write(aMakefile.name);
					aFile.write('.xs"/>\n');
				}
			}
			aFile.write("</package>\n");
			aFile.close();

			if (this.cmake) {
				this.generateCMake(xscIncludePaths, this.packages, makefiles, config, permissions);
				return;
			}
			/* MAKE */
			aPath = this.joinPath(this.tmpPath, "makefile", null);
			aFile = new XSFile(aPath);
			aFile.write('# WARNING: This file is automatically generated by kprconfig. Do not edit. #\n');
			
			for (var i in variableTable) {
				var value = variableTable[i];
				if (value) {
					if (value.indexOf("[applicationPath]") >= 0)
						continue;
					if (value.indexOf(this.home) >= 0)
						continue;
					aFile.write(i);
					aFile.write(' = ');
					aFile.write(variableTable[i]);
					aFile.write('\n');
				}
			}

			aFile.write("APP = " + this.application + "\n");
			aFile.write("KPR_APPLICATION = " + this.application + "\n");
			aFile.write("KPR_MAKE_PATH = " + this.makePath + "\n");
			aFile.write("KPR_RESOURCE_PATH = " + this.resourcePath + "\n");
			aFile.write("KPR_CONFIGURATION = " + (this.debug ? "debug\n" : "release\n"));
			aFile.write("KPR_PLATFORM = " + this.platform + "\n");
			aFile.write("KPR_SUBPLATFORM = " + this.subplatform + "\n");
			aFile.write("KPR_BIN_DIR = " + this.joinPath(null, this.binPath, null) + "\n");
			aFile.write("BIN_DIR = ");
			aFile.write(this.joinPath(null, this.binPath, null));
			if (this.platform == "mac") {
				aFile.write("/");
				aFile.write(this.application)
				aFile.write(".app/Contents/MacOS");
				aFile.write("\n");
				aFile.write("BUNDLE = ");
				aFile.write(this.joinPath(null, this.binPath, null));
				aFile.write("/");
				aFile.write(this.application)
				aFile.write(".app");
			}
			aFile.write("\n");
			
			aFile.write("TMP_DIR = ");
			aFile.write(this.joinPath(null, this.tmpPath, null));
			aFile.write("\n");
			aFile.write("\n");
			
			aFile.include(this.joinPath(this.makePath, "prefix.make", false));

			aFile.write("\nSUPPORT_XS_DEBUG=" + (this.xsdebug || this.debug || config.xsdebug["enabled"] ? 1 : 0) + "\n");

			aFile.write("\nMANIFEST_PATH=" + this.manifestPath + "\n")
			
			aFile.write("\n");
			aFile.write("\n");
			
			var c = makefiles.length;
			for (var i = 0; i < c; i++) {
				aFile.write("\n#Generate variables according to file: ");
				aFile.write(makefiles[i].mkPath);
				aFile.write("\n");
				makefiles[i].generateVariables(aFile);
			}

			aFile.write("\n#Overall Flags\n");
			if (this.windows)
				aFile.write("C_INCLUDES = /I$(TMP_DIR) $(FskPlatform_C_INCLUDES)\n");
			else
				aFile.write("C_INCLUDES += -I$(TMP_DIR) $(FskPlatform_C_INCLUDES)\n");
			
			if (this.windows)
				aFile.write("C_OPTIONS =");
			else
				aFile.write("C_OPTIONS +=");
			var options = config.cOptions;
			var c = options.length;
			for (var i = 0; i < c; i++) {
				var option = options[i];
				if (this.checkPlatform(option.platform)) {
					aFile.write(" ");
					aFile.write(option.name);
				}
			}
			aFile.write(" $(FskPlatform_C_OPTIONS)\n");
			
			if (this.windows)
				aFile.write("HEADERS = $(FskPlatform_HEADERS)\n");
			else
				aFile.write("HEADERS += $(FskPlatform_HEADERS)\n");
			
			if (this.windows)
				aFile.write("LIBRARIES =");
			else
				aFile.write("LIBRARIES +=");
			var c = this.libraries.length;
			for (var i = 0; i < c; i++) {
				aFile.write(" \\\n\t");
				aFile.write(this.libraries[i]);
			}
			aFile.write("\n");
			
			if (this.windows) {
				aFile.write("ARCHIVES =");
				aFile.write(" $(TMP_DIR)\\FskManifest.xs.o $(TMP_DIR)\\FskManifest.o")
			}
			else {
				aFile.write("ARCHIVES +=");
				aFile.write(" $(TMP_DIR)/FskManifest.xs.o $(TMP_DIR)/FskManifest.o")
			}
			var c = makefiles.length;
			for (var i = 0; i < c; i++) {
				var makefile = makefiles[i];
				if (makefile.sources.length)
					aFile.write(" $(" + makefiles[i].name + "_ARCHIVE)");
			}
			aFile.write("\n");

			if (this.windows) {
				aFile.write("OBJECTS =");
				aFile.write(" $(TMP_DIR)\\FskManifest.xs.o $(TMP_DIR)\\FskManifest.o")
			}
			else {
				aFile.write("OBJECTS +=");
				aFile.write(" $(TMP_DIR)/FskManifest.xs.o $(TMP_DIR)/FskManifest.o")
			}
			for (var i = 0; i < c; i++) {
				var makefile = makefiles[i];
				if (makefile.sources.length && !makefile.separate)
					aFile.write(" $(" + makefile.name + "_OBJECTS)");
			}
			aFile.write("\n");
			
			if (this.windows) {
				aFile.write("WILDCARDS =");
				for (var i = 0; i < c; i++) {
					var makefile = makefiles[i];
					if (makefile.sources.length && !makefile.separate) {
						aFile.write(" $(TMP_DIR)\\");
						aFile.write(makefile.name);
						aFile.write("\\*.*");
					}
				}
				aFile.write("\n");
			}
			
			aFile.write("\n#XSC Flags\n");
			aFile.write("XSC_OPTIONS =");
			aFile.write(" \\\n\t-b");
			if (this.debug)
				aFile.write(" \\\n\t-d");
			var c = xscIncludePaths.length;
			for (var i = 0; i < c; i++) {
				aFile.write(" \\\n\t-i ");
				aFile.write(xscIncludePaths[i]);
			}
			if (this.debug)
				aFile.write(" \\\n\t-t debug");
			aFile.write(" \\\n\t-t KPR_CONFIG");
			aFile.write(" \\\n\t-xsID");
			if (this.xscOptions) {
				aFile.write(" \\\n\t");
				aFile.write(this.xscOptions);
			}
			aFile.write("\n");
			
			aFile.write("XSC_PACKAGES =");
			var packages = this.packages;
			var c = packages.length;
			for (var i = 0; i < c; i++) {
				aFile.write(" \\\n\t");
				aFile.write(packages[i].__xs__path);
			}
			aFile.write("\n");
			
			aFile.write("\n#Fold and files to make packages\n");
			if (this.windows) {
				this.directoryPaths.forEach(this.fixDirectoryPath);
				this.xmlPaths.forEach(this.fixItemPaths);
				this.jsPaths.forEach(this.fixItemPaths);
				this.otherPaths.forEach(this.fixItemPaths);
			}
			aFile.write("FOLDERS =");
			this.directoryPaths.forEach(this.writeDirectoryPathVariable, aFile);
			aFile.write("\n");
			aFile.write("FILES =");
			aFile.write(" \\\n\t$(BIN_DIR)");
			aFile.write(this.slash);
			aFile.write(this.application);
			aFile.write(".xsb");
			if (certificate) {
				aFile.write(" \\\n\t$(BIN_DIR)");
				aFile.write(this.slash);
				aFile.write(certificate);
			}
			this.xmlPaths.forEach(this.writeScriptPathVariable, aFile);
			this.jsPaths.forEach(this.writeScriptPathVariable, aFile);
			this.otherPaths.forEach(this.writeFilePathVariable, aFile);
			aFile.write("\n");
			
			aFile.write("\n\n#Include file ");
			var parts = this.splitPath(this.manifestPath);
			var localSuffix;
			if (this.windows) {
				localSuffix = this.resolvePath(this.joinPath(parts[0], "suffix.win.make", false));
			}
			else
				localSuffix = this.resolvePath(this.joinPath(parts[0], "suffix.make", false));
			if(!localSuffix)
				localSuffix = this.resolvePath(this.joinPath(this.makePath, "suffix.make", false)); // Use default suffix.make
			if(!localSuffix)
				throw new Error("Can not find file: " + this.joinPath(this.makePath, "suffix.make", false));
			aFile.write(localSuffix);
			aFile.write(" \n");
			aFile.include(localSuffix);
			aFile.write("\n");

			var c = makefiles.length;
			for (var i = 0; i < c; i++) {
				var makefile = makefiles[i];
				if (makefile.separate) {
					aFile.write("$(SEPARATE_LIBRARIES_DIR)/lib" + makefile.name + ".so: " + "$(" + makefile.name + "_OBJECTS)\n");
					aFile.write("\t$(LINK) ");
					aFile.write("-Wl,-soname,lib" + makefile.name + ".so $(SEPARATE_LINK_OPTIONS) ");
					aFile.write("$(SEPARATE_LIBRARIES) $(" + makefile.name + "_LIBRARIES) ");
					aFile.write("$(" + makefile.name + "_OBJECTS) ");
					aFile.write("-o $@\n");
				}
			}

			aFile.write("\n");
			aFile.write("separate:");
			var c = makefiles.length;
			for (var i = 0; i < c; i++) {
				var makefile = makefiles[i];
				if (makefile.separate) {
					aFile.write("\\\n\t$(SEPARATE_LIBRARIES_DIR)/lib" + makefile.name + ".so ");
				}
			}
			aFile.write("\n\n");

			aFile.write("\n#Generate rules for all files.\n");
			if (this.windows) {
				aFile.write("$(TMP_DIR)\\FskManifest.xs.o: $(HEADERS) $(TMP_DIR)\\FskManifest.xs.c");
				aFile.write("\n\tcl $(TMP_DIR)\\FskManifest.xs.c $(C_OPTIONS) $(C_INCLUDES) /Fo$@\n");
				aFile.write("$(TMP_DIR)\\FskManifest.xs.c: $(XSC_PACKAGES) ");
				aFile.write(this.manifestPath);
				aFile.write("\n\t$(XSC) $(TMP_DIR)\\FskManifest.xs $(XSC_OPTIONS) -o $(TMP_DIR)\n");
				
				aFile.write("$(TMP_DIR)\\FskManifest.o: $(HEADERS) ");
				aFile.write(this.manifestPath);
				aFile.write("\n\tcl $(TMP_DIR)\\FskManifest.c $(C_OPTIONS) $(C_INCLUDES) /Fo$@\n");
				
			}
			else {
				aFile.write("$(TMP_DIR)/FskManifest.xs.o: $(HEADERS) $(TMP_DIR)/FskManifest.xs.c");
				aFile.write("\n\t$(CC) $(TMP_DIR)/FskManifest.xs.c $(C_OPTIONS) $(C_INCLUDES) -c -o $@\n");
				aFile.write("$(TMP_DIR)/FskManifest.xs.c: $(XSC_PACKAGES) ");
				aFile.write(this.manifestPath);
				aFile.write("\n\t$(XSC) $(TMP_DIR)/FskManifest.xs $(XSC_OPTIONS) -o $(TMP_DIR)\n");
					
				aFile.write("$(TMP_DIR)/FskManifest.o: $(HEADERS) ");
				aFile.write(this.manifestPath);
				aFile.write("\n\t$(CC) $(TMP_DIR)/FskManifest.c $(C_OPTIONS) $(C_INCLUDES) -c -o $@\n");
			}	
			
			var c = makefiles.length;
			for (var i = 0; i < c; i++)
				makefiles[i].generateRules(aFile);
				
			if (this.windows) {
				aFile.write("$(BIN_DIR)\\");
				aFile.write(this.application);
				aFile.write(".xsb : $(TMP_DIR)\\FskManifest.xsb");
				aFile.write("\n\tcopy /Y $(TMP_DIR)\\FskManifest.xsb $(BIN_DIR)\\");
				aFile.write(this.application);
				aFile.write(".xsb\n");
				if (certificate) {
					aFile.write("$(BIN_DIR)\\");
					aFile.write(certificate);
					aFile.write(" : $(F_HOME)\\data\\sslcert\\");
					aFile.write(certificate);
					aFile.write("\n\tcopy /Y ");
					aFile.write("$(F_HOME)\\data\\sslcert\\");
					aFile.write(certificate);
					aFile.write(" $(BIN_DIR)\\");
					aFile.write(certificate);
					aFile.write("\n");
				}
			}
			else {
				aFile.write("$(BIN_DIR)/");
				aFile.write(this.application);
				aFile.write(".xsb : $(TMP_DIR)/FskManifest.xsb");
				aFile.write("\n\tcp -p $(TMP_DIR)/FskManifest.xsb $(BIN_DIR)/");
				aFile.write(this.application);
				aFile.write(".xsb\n");
				if (certificate) {
					aFile.write("$(BIN_DIR)/");
					aFile.write(certificate);
					aFile.write(" : $(F_HOME)/data/sslcert/");
					aFile.write(certificate);
					aFile.write("\n\tcp -p ");
					aFile.write("$(F_HOME)/data/sslcert/");
					aFile.write(certificate);
					aFile.write(" $(BIN_DIR)/");
					aFile.write(certificate);
					aFile.write("\n");
				}
			}
			
			aFile.write("\n#Rules to create target folds, compilging xml/js files and copy resource files.\n");
			this.directoryPaths.forEach(this.writeDirectoryPathRule, aFile);
			this.xmlPaths.forEach(this.writeXMLPathRule, aFile);
			this.jsPaths.forEach(this.writeScriptPathRule, aFile);
			this.otherPaths.forEach(this.writeFilePathRule, aFile);
			aFile.close();
		
			if (this.make) {
				if (this.verbose)
					this.report("Making " + this.application  + "...");
				if (this.windows) {
					var result = this.execute("nmake.exe", this.action, "/C", "/F", this.tmpPath + "\\makefile");
					if (result != true)
						throw new Error("namke failed!");
				} else {
					this.execute("make", this.action, "-j", this.jobs, "-f", this.tmpPath + "/makefile");
				}
			}
		]]></function>
		
		<function name="fixDirectoryPath" params="path">
			XSFileFixUpPath(path);
		</function>
		<function name="fixItemPaths" params="item">
			if (item.destinationPath)
				XSFileFixUpPath(item.destinationPath);
			if (item.sourcePath)
				XSFileFixUpPath(item.sourcePath);
		</function>
		
		<function name="writeDirectoryPathVariable" params="path">
			this.write(" \\\n\t$(BIN_DIR)");
			this.write(xsTool.slash);
			this.write(path);
			this.write(" \\\n\t$(TMP_DIR)");
			this.write(xsTool.slash);
			this.write(path);
		</function>
		<function name="writeFilePathVariable" params="item">
			this.write(" \\\n\t$(BIN_DIR)");
			this.write(xsTool.slash);
			this.write(item.destinationPath);
		</function>
		<function name="writeScriptPathVariable" params="item">
			this.write(" \\\n\t$(BIN_DIR)");
			this.write(xsTool.slash);
			this.write(item.destinationPath);
			this.write(".jsb");
		</function>
		
		<function name="writeDirectoryPathRule" params="path">
			this.write("$(BIN_DIR)");
			this.write(xsTool.slash);
			this.write(path);
			if (xsTool.windows) {
				this.write(" :\n\tif not exist $(BIN_DIR)");
				this.write(xsTool.slash);
				this.write(path);
				this.write("/$(NULL) mkdir $(BIN_DIR)");
			}
			else
				this.write(" :\n\tmkdir -p $(BIN_DIR)");
			this.write(xsTool.slash);
			this.write(path);
			this.write("\n");
			
			this.write("$(TMP_DIR)");
			this.write(xsTool.slash);
			this.write(path);
			if (xsTool.windows) {
				this.write(" :\n\tif not exist $(TMP_DIR)");
				this.write(xsTool.slash);
				this.write(path);
				this.write("/$(NULL) mkdir $(TMP_DIR)");
			}
			else
				this.write(" :\n\tmkdir -p $(TMP_DIR)");
			this.write(xsTool.slash);
			this.write(path);
			this.write("\n");
		</function>
		<function name="writeFilePathRule" params="item">
			this.write("$(BIN_DIR)");
			this.write(xsTool.slash);
			this.write(item.destinationPath);
			this.write(" : ");
			this.write(item.sourcePath);
			if (xsTool.windows)
				this.write("\n\tcopy /Y ");
			else
				this.write("\n\tcp -p ");
			this.write(item.sourcePath);
			this.write(" $(BIN_DIR)");
			this.write(xsTool.slash);
			this.write(item.destinationPath);
			this.write("\n");
		</function>
		<function name="writeScriptPathRule" params="item">
			var parts = xsTool.splitPath(item.destinationPath);
			this.write("$(BIN_DIR)");
			this.write(xsTool.slash);
			this.write(item.destinationPath);
			this.write(".jsb : ");
			this.write(item.sourcePath);
			this.write("\n\t$(XSC) -b ");
			if (xsTool.debug)
				this.write("-d ");
			this.write(item.sourcePath);
			this.write(" -o $(BIN_DIR)");
			this.write(xsTool.slash);
			this.write(parts[0]);
			this.write("\n");
		</function>
		<function name="writeXMLPathRule" params="item">
			var parts = xsTool.splitPath(item.destinationPath);
			
			this.write("$(TMP_DIR)");
			this.write(xsTool.slash);
			this.write(item.destinationPath);
			this.write(".js : ");
			this.write(item.sourcePath);
			this.write("\n\t$(KPR2JS) ");
			this.write(item.sourcePath);
			this.write(" -o $(TMP_DIR)");
			this.write(xsTool.slash);
			this.write(parts[0]);
			this.write("\n");
			
			this.write("$(BIN_DIR)");
			this.write(xsTool.slash);
			this.write(item.destinationPath);
			this.write(".jsb : $(TMP_DIR)");
			this.write(xsTool.slash);
			this.write(item.destinationPath);
			this.write(".js\n\t$(XSC) -b ");
			if (xsTool.debug)
				this.write("-d ");
			this.write("$(TMP_DIR)");
			this.write(xsTool.slash);
			this.write(item.destinationPath);
			this.write(".js -o $(BIN_DIR)");
			this.write(xsTool.slash);
			this.write(parts[0]);
			this.write("\n");
		</function>
	
		<function name="insertUnique" params="array, string">
			if (!string)
				return
    		var c = array.length;
    		for (var i = 0; i < c; i++)
        		if (array[i] == string)
            		return;
            array.push(string);		
		</function>
		<function name="getVariable" params="name, table">
			if (name in table)
				return table[name];
			var value = this.getEnvironmentValue(name);
			if (value)
				return table[name] = value;
			return table[name] = "[" + name + "]";
		</function>
		<function name="applyVariables" params="value, table">
			var j = 0;
			while (true) {
				var k = value.indexOf("[", j)
				if (k < 0)
					break
				var l = value.indexOf("]", k)
				if (l < 0)
					break
				var name = value.substring(k + 1, l)
				var replacement = this.getVariable(name, table);
				if (name == "F_HOME") {
					if (value.charAt(l + 1) != '/')
						replacement = replacement + "/";
				}
				l = value.length - (l + 1);
				value = value.replace("[" + name + "]", replacement);
				j = value.length - l;
			}
			return value
		</function>
		
		<function name="segregateCOptions" params="name, copts, defs, incs">
			var results = name.match(/"?[-\/][^ ]*(\s*[^-\/]*)/g);
			if (results) {
				for (var j = 0; j < results.length; j++) {
					var option = results[j].trim();
					var regexpVar = /\$\(([^)]*)\)/;
					var regexpStr = /\$([^)]*\))/;
					var matches = option.match(regexpVar);
					if (matches)
						option = option.replace(regexpStr, "${" + matches[1] + "}");
					switch (option.match(/"?([-\/].)/)[1]) {
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
							incs.push(option.match(/"?-I(.*)/)[1]);
							break;
						case "/I":
							incs.push(option.match(/"?\/I(.*)/)[1]);
							break;
						default:
							copts.push(option);
					}
				}
			}
		</function>

		<function name="toCMakePath" params="path">
			if (this.platform == "win")
				path = path.replace('\\', '/');
			return path.replace(this.home, "${F_HOME}");
		</function>

		<function name="toCMakeResource" params="item">
			item.destinationPath = this.toCMakePath(item.destinationPath);
			item.sourcePath = this.toCMakePath(item.sourcePath);
			return item;
		</function>
		
		<function name="generateCMake" params="xscIncludePaths, xscPackages, makefiles, config, permissions ">
			var path = xsTool.joinPath(xsTool.tmpPath, "CMakeLists", "txt");
			var parts = this.splitPath(this.manifestPath);
			var localCMake = this.resolvePath(this.joinPath(parts[0], "CMakeLists.txt", false));

			var aFile = new XSFile(path);
			aFile.write('cmake_minimum_required(VERSION 2.8.8)\n');
			aFile.write('# WARNING: This file is automatically generated by kprconfig. Do not edit.\n');
			aFile.write('# SOURCE: ' + makefiles[0].mkPath + '\n');
			aFile.write('# SOURCE: ' + this.manifestPath + '\n\n');

			if (this.platform == "win") {
				aFile.write("set(CMAKE_USER_MAKE_RULES_OVERRIDE ");
				aFile.write("\t${CONFIG_BASEDIR}/${TARGET_PLATFORM}/CMake-Overides/c_flag_overrides.cmake)\n");
				aFile.write("set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ");
				aFile.write("\t${CONFIG_BASEDIR}/${TARGET_PLATFORM}/CMake-Overides/cxx_flag_overrides.cmake)\n\n");
			}

			aFile.write('if (${CMAKE_MAJOR_VERSION} GREATER 2)\n');
			aFile.write('\tcmake_policy(SET CMP0037 OLD)\n');
			aFile.write('endif()\n\n');

			aFile.write('project("'); aFile.write(this.application); aFile.write('")\n\n');

			aFile.write("set(SUPPORT_XS_DEBUG " + (this.xsdebug || this.debug || config.xsdebug["enabled"] ? 1 : 0) + ")\n\n");

			aFile.write('include(${CONFIG_BASEDIR}/CMakeLists.txt)\n\n');
			if (xsTool.platform == "android") {
				aFile.write('set(BASE_C_FLAGS ${CMAKE_C_FLAGS})\n');
				aFile.write('set(BASE_CXX_FLAGS ${CMAKE_CXX_FLAGS})\n\n');
			}

			aFile.write('set(XSC_OPTIONS -t KPR_CONFIG -b ' + this.xscOptions);
			if (this.debug)
				aFile.write(' -d');
			aFile.write(')\n');
			var c = xscIncludePaths.length;
			for (var i = 0; i < c; i++) {
				path = this.toCMakePath(xscIncludePaths[i]);
				aFile.write("list(APPEND XSC_OPTIONS -i " + path + ")\n");
			}
			aFile.write('\n');

			var c = this.xmlPaths.length;
			for (i = 0; i < c; i++) {
				var item = this.toCMakeResource(this.xmlPaths[i]);
				var targetName = item.destinationPath.replace('/', '_');
				aFile.write('xml2jsb(SOURCE ');
				aFile.write(item.sourcePath);
				aFile.write(' DESTINATION ');
				aFile.write(item.destinationPath);
				aFile.write('.jsb)\n');
			}
			if (c > 0)
				aFile.write('\n');

			var c = this.jsPaths.length;
			for (i = 0; i < c; i++) {
				var item = this.toCMakeResource(this.jsPaths[i]);
				var targetName = item.destinationPath.replace('/', '_');
				aFile.write('js2jsb(');
				aFile.write('SOURCE ');
				aFile.write(item.sourcePath);
				aFile.write(' DESTINATION ');
				aFile.write(item.destinationPath);
				aFile.write('.jsb)\n');
			}
			if (c > 0)
				aFile.write('\n');

			var c = this.otherPaths.length;
			for (i = 0; i < c; i++) {
				var item = this.toCMakeResource(this.otherPaths[i]);
				var parts = xsTool.splitPath(item.destinationPath);
				var targetName = item.destinationPath.replace('/', '_');
				if (parts[2] == ".psd")
					continue;
				aFile.write('copy_file(');
				aFile.write('SOURCE ');
				aFile.write(item.sourcePath);
				aFile.write(' DESTINATION ')
				if (parts[0] == "program/iphone") {
					if (this.platform == "iphone")
						aFile.write('.');
				} else {
					aFile.write(item.destinationPath)
				}
				aFile.write(')\n');
			}
			if (!localCMake) {
				aFile.write('copy_file(SOURCE ${F_HOME}/data/sslcert/ca-bundle.crt DESTINATION ca-bundle.crt)\n');
				aFile.write('copy_file(SOURCE ${BUILD_TMP}/FskManifest.xsb DESTINATION ' + this.application + '.xsb DEPENDS FskManifest)\n');
				aFile.write('\n');
			}
					
			var copts = [];
			var defs = [];
			var incs = [];

			var options = config.cOptions;
			debugger
			var c = options.length;
			for (var i = 0; i < c; i++) {
				var option = options[i];
				if (this.checkPlatform(option.platform))
					this.segregateCOptions(option.name, copts, defs, incs);
			}

			var makefile = makefiles[0];
			var c = makefile.cOptions.length;
			for (var i = 0; i < c; i++) {
				this.segregateCOptions(makefile.cOptions[i].name, copts, defs, incs);
			}

			var asmOptions = makefile.asmOptions;
			var c = asmOptions.length;
			for (var i = 0; i < c; i++) {
				var option = asmOptions[i];
				aFile.write('set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ' + option.name + '")\n');
			}
			if (c > 0)
				aFile.write("\n");

			var c = copts.length;
			for (var i = 0; i < c; i++) {
				aFile.write('add_cflags("')
				aFile.write(copts[i]);
				aFile.write('")\n'); 
			}
			aFile.write('\n');

			var c = defs.length;
			for (var i = 0; i < c; i++) {
				aFile.write('add_definitions(');
				// Corectly handle the IBOutlet options for iOS
				var matches = defs[i].match(/^"(.*)"/);
				if (matches)
					aFile.write('"\'' + matches[1] + '\'"');
				else
					aFile.write(defs[i]);
				aFile.write(')\n'); 
			}
			var c = permissions.length;
			for (var i = 0; i < c; i++) {
				aFile.write('add_definitions(-DANDROID_PERMISSION_');
				aFile.write(permissions[i].name);
				aFile.write(')\n');
			}
			aFile.write('\n');
			var c = incs.length;
			for (var i = 0; i < c; i++) {
				path = this.toCMakePath(incs[i]);
				aFile.write('include_directories('); aFile.write(path); aFile.write(')\n');
			}
			var c = makefile.cIncludes.length;
			for (var i = 0; i < c; i++) {
				path = this.toCMakePath(makefile.cIncludes[i]);
				aFile.write('include_directories('); aFile.write(path); aFile.write(')\n');
			}
			aFile.write('\n');

			var c = this.libraries.length;
			var linkIncs = [];
			var linkFlags = [];
			var frameworks = [];
			var frameworkPaths = [];
			var linkOther = [];
			for (i = 0; i < c; i++) {
				libraries = this.libraries[i].replace("(", "{").replace(")", "}");
				results = libraries.match(/.[^ ]*(\s*[^-\/]*)/g);
				if (results) {
					for (var j = 0; j < results.length; j++) {
						var option = results[j].trim();
						switch (option.substring(0,2)) {
							case "-L":
								linkIncs.push(option.substring(2));
								break;
							case "-l":
								linkFlags.push(option);
								break;
							case "-f":
								frameworks.push(option.split(' ')[1]);
								break;
							case "-F":
								frameworkPaths.push(option.substring(2));
								break;
							default:
								if (this.platform == "win") {
									if (/^\/.*/.test(option))
										linkFlags.push(this.toCMakePath(option));
									else if (/.*\.lib$/.test(option))
										linkOther.push(this.toCMakePath(option));
								} else {
									linkOther.push(option);
								}
						}
					}
				}
			}

			if (xsTool.platform == "android") {
				aFile.write('link_directories(${NDK_LIBS_PATH})\n');
				aFile.write('link_directories(${NDK_DIR}/platforms/android-${NDK_PLATFORM_VER}/arch-arm/usr/lib)\n');
			}
			var c = linkIncs.length;
			if (c > 0) {
				for (i = 0; i < c; i++)
					aFile.write('link_directories(' + this.toCMakePath(linkIncs[i]) + ')\n');
				aFile.write('\n');
			}

			var c = linkOther.length;
			if (c > 0)
				aFile.write('LIST(APPEND EXTRA_LIBS ' + linkOther.join(' ') + ')\n\n');

			var c = frameworkPaths.length;
			if (c > 0) {
				for (var i = 0; i < c; i++)
					aFile.write('list(APPEND CMAKE_PREFIX_PATH ' + this.toCMakePath(frameworkPaths[i]) + ')\n');
				aFile.write("\n");
			}

			var c = frameworks.length;
			if (c > 0) {
				for (i = 0; i < c; i++) {
					aFile.write("local_find_library(");
					aFile.write(frameworks[i]);
					aFile.write(")\n")
				}
				aFile.write("\n");
			}

			aFile.write('file(GLOB sources ${BUILD_TMP}/*.c)\n');
			var c = makefiles[0].sources.length;
			for (var i = 0; i < makefiles[0].sources.length; i++) {
				aFile.write("add_source(LIST sources SOURCE " + makefiles[0].sources[i].path + ")\n");
			}
			aFile.write('xscc(SOURCE ${BUILD_TMP}/FskManifest.xs)\n');

			if (xsTool.platform == "iphone")
				aFile.write('list(APPEND sources ${KPR_PATCH_PATH}/main.m)\n');
			aFile.write('\n');

			if (this.platform == "win") {
				aFile.write('set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ');
				aFile.write(linkFlags.join(' '));
				aFile.write('")\n\n');
			}

			var deps = [];
			var c = makefiles.length;
			if (localCMake) {
				aFile.write("list(APPEND PROJECTS");
			} else {
				aFile.write("process_subdirectories(");
				aFile.write("\n\tOBJECTS");
			}
			for (var i = 1; i < c; i++) {
				var makefile = makefiles[i];
				if (makefile.cmakeOutputType == "object") {
					if (makefile.sources.length > 0) {
						aFile.write('\n\t');
						aFile.write(makefile.name);
					}
				} else if (makefile.cmakeOutputType == "shared") {
					deps.push(makefile.name);
				}
			}
			var c = deps.length;
			if (c > 0) {
				aFile.write('\n\tSHARED');
				for (var i = 0; i < c; i++) {
					aFile.write('\n\t')
					aFile.write(deps[i]);
				}
			}
			aFile.write("\n\t)\n\n");
		
			if (localCMake) {
				aFile.write("include(" + this.toCMakePath(localCMake) + ")\n");
			} else {
				if (this.platform == "android") {
					aFile.write('add_library(${KPR_BINARY_NAME}');
				} else {
					aFile.write('add_executable(${KPR_BINARY_NAME}');
					if (this.platform == "iphone" || this.platform == "mac")
						aFile.write(' MACOSX_BUNDLE')
					else if (this.platform == "win")
						aFile.write(' WIN32');
				}
				aFile.write(' ${PLATFORM_OBJECTS} ${sources})\n\n');
			}

			aFile.write('target_link_libraries(${KPR_BINARY_NAME} ${EXTRA_LIBS}');
			if (this.platform != "win") {
				if (linkFlags.length > 0)
					aFile.write(' ' + linkFlags.join(' '));
				if (makefile.cmakeLinks)
					aFile.write(' ' + makefile.cmakeLinks);
			}
			aFile.write(')\n');

			aFile.write('add_dependencies(${KPR_BINARY_NAME} ${PROJECT_DEPENDS})\n');

			if (!localCMake)
				aFile.write('include(${CONFIG_BASEDIR}/${TARGET_PLATFORM}/suffix.cmake OPTIONAL)\n');

			if (deps.length > 0) {
				if (this.platform == "android")
					aFile.write('\nadd_dependencies(${DEPENDS_TARGET} ${DEPENDS_TARGET_SHARED})\n');
			}

			aFile.close();
			
			var c = makefiles.length;
			for (var i = 1; i < c; i++) {
				var aMakefile = makefiles[i];
				aMakefile.generateCMake();
			}
		</function>
		
		<function name="generateFskPlatform">
			var srcPath, dstPath, srcText, dstText;
			srcPath = this.home + "/kinoma/kpr/cmake/" + this.platform + "/FskPlatform.h";
			if (this.subplatform) {
				aPath = this.home + "/kinoma/kpr/cmake/" + this.platform + "/" + this.subplatform + "/FskPlatform.h";
				if (this.fileExists(aPath))
					srcPath = aPath;
			}
			srcText = this.readString(srcPath);
			srcText = srcText.replace(/#define[\s]+SUPPORT_INSTRUMENTATION[\s]+0/, "#define SUPPORT_INSTRUMENTATION " + (this.instrument ? 1 : 0));
			srcText = srcText.replace(/#define[\s]+SUPPORT_MEMORY_DEBUG[\s]+0/, "#define SUPPORT_MEMORY_DEBUG " + (this.leak ? 1 : 0));
			dstPath = this.joinPath(this.tmpPath, "FskPlatform", "h");
			if (this.resolvePath(dstPath, false))
				dstText = this.readString(dstPath);
			else
				dstText = "";
			if (srcText != dstText) 
				this.writeString(dstPath, srcText);
		</function>

	</patch>
</package>
