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
	<function name="main" params="theArguments">
		xsTool.initialize(theArguments);
		xsTool.run();
	</function>

	<object name="xsTool">
		<number name="errorCount" value="0"/>
		<object name="targets"/>
		<number name="warningCount" value="0"/>
		
		<function name="compareFile" params="a, b" c="xsToolCompareFile"/>
		
		<function name="compareName" params="a, b" c="xsToolCompareName"/>
		
		<function name="createDirectory" params="theDirectory" c="xsToolCreateDirectory"/>

		<function name="fileExists" params="theFile" c="xsToolFileExists"/>

		<function name="ensureDirectory" params="path" script="true">
			var result = this.resolvePath(path, true);
			if (!result) {
				offset = 0
				for (;;) {
					var offset = path.indexOf("/", offset + 1)
					if (-1 == offset)
						break;
					var directory = path.substring(0, offset)
					if (this.resolvePath(directory, true))
						continue;
					this.createDirectory(directory)
				}
				result = this.createDirectory(path);
			}
			return result;
		</function>
		
		<function name="execute" c="xsToolExecute"/>
		
		<function name="getPath" params="theName">
			var c, i, aPath;
			
			c = this.inputPaths.length;
			for (i = 0; i < c; i++) {
				aPath = this.joinPath(this.inputPaths[i], theName, null);
				aPath =	this.resolvePath(aPath, false);
				if (aPath)
					return aPath;
			
			}
		</function>
		
		<function name="getEnvironmentValue" params="theName" c="xsToolGetEnvironmentValue"/>

		<function name="getIPAddress" c="xsToolGetIPAddress"/>
		
		<function name="getPlatform" c="xsToolGetPlatform"/>
		
		<function name="hasTarget" params="theTarget">
			with (this.targets)
				return eval(theTarget);
		</function>

		<function name="insertProperty" params="theQualifiedName, theProperty, patchIt" c="xsToolInsertProperty"/>
		
		<function name="isKPR" params="path" c="xsToolIsKPR"/>

		<function name="joinPath" params="theDirectory, theName, theExtension" c="xsToolJoinPath"/>
		
		<function name="listDirectory" params="theDirectory" c="xsToolListDirectory"/>
		
		<function name="load" params="thePath" c="xsToolLoad"/>
		
		<function name="loadPackage" params="thePath">
			var c, i, aPackage;
			
			c = this.packages.length;
			for (i = 0; i < c; i++) {
				if (this.compareFile(this.packages[i].__xs__path, thePath))
					return null;
			}
			if (this.verbose)
				this.report("Loading " + thePath + "...");
			aPackage = this.load(thePath);
			if (aPackage == undefined)
				throw new Error("'" + thePath + "': invalid file!");
			
			aPackage.__xs__path = thePath;
			this.packages[c] = aPackage;
			return aPackage;
		</function>
		
		<function name="readString" params="thePath" c="xsToolReadString"/>
		
		<function name="replacePath" params="theOffset, theString" c="xsToolReplacePath"/>
		
		<function name="report" params="theString" c="xsToolReport"/>

		<function name="reportError" params="thePath, theLine, theError" c="xsToolReportError"/>

		<function name="reportWarning" params="thePath, theLine, theWarning" c="xsToolReportWarning"/>
		
		<function name="resolvePath" params="thePath, theFlag" c="xsToolResolvePath"/>
		
		<function name="searchPath" params="thePath, theFlag">
			if (thePath.charAt(0) == "/")
				thePath = "$(XS_HOME)" + thePath;
			thePath = thePath.replace(/\$\(([\w]+)\)/g, this.replacePath);
			if (this.windows) {
				if (thePath.charAt(1) != ":")
					thePath = this.currentPath + "/" + thePath;
			}
			else {
				if (thePath.charAt(0) != "/")
					thePath = this.currentPath + "/" + thePath;
			}
			return this.resolvePath(thePath, theFlag);
		</function>		

		<function name="searchProperty" params="theQualifiedName" c="xsToolSearchProperty"/>
		
		<function name="splitPath" params="thePath" c="xsToolSplitPath"/>
	
		<function name="trimString" params="theString">
			var aString = theString; 
			var aRegExp = /^(\s*)([\W\w]*)(\b\s*$)/; 
			if (aRegExp.test(aString)) 
				aString = aString.replace(aRegExp, '$2');
			return aString; 
		</function>		
		
		<function name="writeString" params="thePath, theString" c="xsToolWriteString"/>
	</object>
	
	<object name="xsFile" c="xsToolDestroyFile">
		<function name="close" c="xsToolCloseFile"/>
		<function name="include" params="thePath" c="xsToolIncludeFile"/>
		<function name="open" params="thePath" c="xsToolOpenFile"/>
		<function name="write" params="theString" c="xsToolWriteFile"/>
	</object>
	<function name="XSFile" params="thePath, mode" prototype="xsFile">
		if (mode)
			this.open(thePath, mode);
		else
			this.open(thePath);
	</function>

	<function name="XSFileFixUpPath" params="path" c="xsToolFileFixUpPath"/>

</package>