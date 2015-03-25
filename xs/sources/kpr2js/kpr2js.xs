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
<package script="true">
	<import href="kprTemplatesGrammar.xs"/>

	<function name="main" params="theArguments">
		kpr2js.initialize(theArguments);
		kpr2js.run();
	</function>
	
	<object name="kpr2js">
		<boolean name="debug" value=""/>
		<number name="errorCount" value="0"/>
		<undefined name="grammarPaths"/>
		<undefined name="outputPath"/>
		<boolean name="verbose"/>
		<number name="warningCount" value="0"/>
		
		<function name="compareFile" params="a, b" c="kpr2jsCompareFile"/>
		
		<function name="compareName" params="a, b" c="kpr2jsCompareName"/>
		
		<function name="createDirectory" params="theDirectory" c="kpr2jsCreateDirectory"/>

		<function name="getPlatform" c="kpr2jsGetPlatform"/>
		
		<function name="initialize" params="theArguments">
			var anIndex, aName, aPath, aSplit;
			
			this.grammarPaths = [];
			this.outputPath = "";
			
			for (anIndex = 1; anIndex < theArguments.length; anIndex++) {
				switch (theArguments[anIndex]) {

				case "-o":
					anIndex++;	
					aName = theArguments[anIndex];
					if (this.outputPath != "")
						throw new Error("-o '" + aName + "': too many -o directory!");
					aPath = this.resolvePath(aName, true);
					if (aPath)
						this.outputPath = aPath;
					else
						throw new Error("-o '" + aName + "': directory not found!");
					break;

				case "-v":
					this.verbose = true;
					break;
			
				default:
					aName = theArguments[anIndex];
					aPath = this.resolvePath(aName, false);
					if (aPath)
						this.grammarPaths[this.grammarPaths.length] = aPath;
					else
						throw new Error("'" + aName + "': file not found!");
					break;
				}
			}
			if (!this.grammarPaths.length)
				throw new Error("Usage: kpr2js (-o directory) (-v) file+");
			if (this.outputPath == "")
				this.outputPath = this.resolvePath(".", true);
		</function>
		
		<function name="load" c="kpr2jsLoad"/>
		
		<function name="makePath" params="theDirectory, theName, theExtension" c="kpr2jsMakePath"/>

		<function name="makeRelativeURL" params="fromURL, toURL">
			var fromSplit = fromURL.split("/");
			var toSplit = toURL.split("/");
			var fromLength = fromSplit.length - 1;
			var toLength = toSplit.length - 1;
			var i, j, aURL;
			j = Math.min(fromLength, toLength);
			for (i = 0; i < j; i++) {
				if (fromSplit[i] != toSplit[i])
					break;
			}
			aURL = "";
			for (j = i; j < fromLength; j++)
				aURL += "../";
			for (j = i; j < toLength; j++)
				aURL += toSplit[j] + "/";
			aURL += toSplit[toLength];
			return aURL;
 		</function>
		
		<function name="report" params="theString" c="kpr2jsReport"/>

		<function name="reportError" params="thePath, theLine, theError" c="kpr2jsReportError"/>

		<function name="reportWarning" params="thePath, theLine, theWarning" c="kpr2jsReportWarning"/>
		
		<function name="resolvePath" params="thePath, theFlag" c="kpr2jsResolvePath"/>
		
		<function name="run"><![CDATA[
			var paths = this.grammarPaths;
			var c = paths.length;
			for (var i = 0; i < c; i++) {
				var aPath = paths[i];
				var aGrammar = this.load(aPath);
				var aCode = aGrammar.generate();
				var aSplit = this.splitPath(aPath);
				var aPath = this.makePath(this.outputPath, aSplit[1], ".js");
				var aFile = new XSFile(aPath);
				aFile.write(aCode);
				aFile.close();
			}
		]]></function>
		
		<function name="splitPath" params="thePath" c="kpr2jsSplitPath"/>		
	</object>

	<object name="xsFile" c="kpr2jsDestroyFile">
		<number name="charCount"/>
		<function name="close" c="kpr2jsCloseFile"/>
		<function name="include" params="thePath" c="kpr2jsIncludeFile"/>
		<function name="open" params="thePath" c="kpr2jsOpenFile"/>
		<function name="write" params="theString" c="kpr2jsWriteFile"/>
	</object>
	<function name="XSFile" params="thePath" prototype="xsFile">
		this.open(thePath);
	</function>
</package>