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
	<patch prototype="xsPackageItem">
		<function name="makeReference"/>
	</patch>
	
	<patch prototype="xsPackage">
		<function name="makeReference">
			var c, i;
				
			c = this.items.length;
			for (i = 0; i< c; i++)
				this.items[i].makeReference();
		</function>
	</patch>
	
	<patch prototype="xsProgram">
		<function name="makeReference">
			if (this.hasOwnProperty("href")) {
				var aPath = xsTool.getPath(this.href);
				if (!aPath)
					xsTool.reportError(this.__xs__path, this.__xs__line, this.href + ": file not found");
				else
					xsTool.scriptPaths[xsTool.scriptPaths.length] = aPath;
			}
		</function>
	</patch>

	<patch prototype="xsObject">
		<function name="makeReference">
			var c, i;
				
			c = this.items.length;
			for (i = 0; i< c; i++)
				this.items[i].makeReference();
		</function>
	</patch>
</package>