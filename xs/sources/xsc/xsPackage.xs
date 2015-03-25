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
	<object name="ioDocumentation">
		<function name="parse" params="theText">
			if (typeof(XSDocumentation) != "undefined")
				return new XSDocumentation(theText);
		</function>
	</object>
	<custom name="piDocumentation" io="ioDocumentation" pattern="?xsdoc"/>
	
	<object name="xsPackageItem">
		<function name="target" params="theItems, theLink, theDepth"/>
	</object>

	<object name="xsPackage" pattern="/package">
		<string name="cFlag" pattern="@c" value=""/>
		<string name="deleteFlag" pattern="@delete" value=""/>
		<string name="enumFlag" pattern="@enum" value=""/>
		<string name="scriptFlag" pattern="@script" value=""/>
		<string name="setFlag" pattern="@set" value=""/>
		<array name="items" pattern="." contents="xsPackageItem, piDocumentation"/>
		
		<boolean name="linked" value="true"/>

		<function name="target" params="theItems, theLink, theDepth">
			var c, i;
			var items = [];
			if (this.items) {
				theDepth += "  ";
				c = this.items.length;
				for (i = 0; i< c; i++)
					this.items[i].target(items, theLink, theDepth);
			}
			this.items = items;
			if (theItems)
				theItems[theItems.length] = this;
			this.linked = theLink;
		</function>
	</object>

	<object name="xsNamespace" pattern="namespace" prototype="xsPackageItem">
		<string name="prefix" pattern="@prefix"/>
		<string name="uri" pattern="@uri"/>

		<function name="target" params="theItems, theLink, theDepth">
			theItems[theItems.length] = this;
		</function>
	</object>
	
	<object name="xsImport" pattern="import" prototype="xsPackageItem">
		<string name="href" pattern="@href"/>
		<string name="link" pattern="@link" value="static"/>

		<function name="target" params="theItems, theLink, theDepth">
			var aPackage;
			
			if (this.link == "dynamic")
				theLink = false;
			else if (this.link != "static")
				xsTool.reportError(this.__xs__path, this.__xs__line, "the link attribute must be dynamic or static");
				
			if (!this.href)
				xsTool.reportError(this.__xs__path, this.__xs__line, "missing href attribute");
			else {
				var aPath = xsTool.getPath(this.href);
				if (!aPath)
					xsTool.reportError(this.__xs__path, this.__xs__line, this.href + ": file not found");
				else {
					aPackage = xsTool.loadPackage(aPath);
					if (aPackage)
						aPackage.target(theItems, theLink, theDepth);
				}
			}
		</function>
	</object>

	<object name="xsObjectItem" prototype="xsPackageItem">
		<boolean name="linked" value="true"/>

		<function name="target" params="theItems, theLink, theDepth">
			theItems[theItems.length] = this;
			this.linked = theLink;
		</function>
	</object>
	
	<object name="xsDocumentation" prototype="xsObjectItem"/>
	<function name="XSDocumentation" params="theText" prototype="xsDocumentation">
		debugger
	</function>
	
	<object name="xsPatch" pattern="patch" prototype="xsObjectItem">
		<string name="prototype" pattern="@prototype"/>
		<array name="items" pattern="." contents="xsObjectItem, piDocumentation" />
		
		<function name="target" params="theItems, theLink, theDepth">
			var c, i;
			var items = [];
			if (this.items) {
				c = this.items.length;
				for (i = 0; i< c; i++)
					this.items[i].target(items, theLink, theDepth);
			}
			this.items = items;
			theItems[theItems.length] = this;
			this.linked = theLink;
		</function>
	</object>
	
	<object name="xsProgram" pattern="program" prototype="xsObjectItem">
		<string name="c" pattern="@c"/>
		<string name="href" pattern="@href"/>
		<string name="value" pattern="."/>
	</object>
	
	<object name="xsProperty" prototype="xsObjectItem">
		<string name="name" pattern="@name"/>
		<string name="pattern" pattern="@pattern" value=""/>

		<string name="deleteFlag" pattern="@delete" value=""/>
		<string name="enumFlag" pattern="@enum" value=""/>
		<string name="scriptFlag" pattern="@script" value=""/>
		<string name="setFlag" pattern="@set" value=""/>
			
		<function name="insertProperty" params="theQualifiedName, theProperty, patchIt">
			if (this.qualifiedName == theQualifiedName) {
				if (patchIt) {
					theProperty.qualifiedName = theQualifiedName;
				}
				else {
					xsTool.reportError(theProperty.__xs__path, theProperty.__xs__line, "name already defined");
					xsTool.reportError(this.__xs__path, this.__xs__line, "here");
				}
			}
			else if (this.nextProperty)
				return this.nextProperty.insertProperty(theQualifiedName, theProperty, patchIt);
			else {
				this.nextProperty = theProperty;
				theProperty.qualifiedName = theQualifiedName;
			}
		</function>
		
		<function name="searchProperty" params="theQualifiedName">
			if (this.qualifiedName == theQualifiedName)
				return this;
			else if (this.nextProperty)
				return this.nextProperty.searchProperty(theQualifiedName);
			else
				return undefined;
		</function>		
	</object>

	<object name="xsLiteral" prototype="xsProperty">
	</object>
	
	<object name="xsBoolean" pattern="boolean" prototype="xsLiteral">
		<string name="value" pattern="@value" value="false"/>
	</object>
	
	<object name="xsChunk" pattern="chunk" prototype="xsLiteral">
		<string name="value" pattern="@value" value="false"/>
	</object>
	
	<object name="xsCustom" pattern="custom" prototype="xsLiteral">
		<string name="io" pattern="@io"/>
		<string name="value" pattern="@value" value=""/>
	</object>
	
	<object name="xsDate" pattern="date" prototype="xsLiteral">
		<string name="value" pattern="@value" value="01 Jan 1970 00:00:00 GMT"/>
	</object>
	
	<object name="xsFunction" pattern="function" prototype="xsLiteral">
		<string name="c" pattern="@c"/>
		<string name="check" pattern="@check"/>
		<string name="params" pattern="@params" value=""/>
		<string name="prototype" pattern="@prototype"/>
		<string name="value" pattern="."/>
	</object>
	<function name="XSFunction" prototype="xsFunction"/>

	<object name="xsNull" pattern="null" prototype="xsLiteral">
		<string name="pattern" pattern=""/>
	</object>
	
	<object name="xsNumber" pattern="number" prototype="xsLiteral">
		<string name="value" pattern="@value" value="0"/>
	</object>

	<object name="xsRegExp" pattern="regexp" prototype="xsLiteral">
		<string name="value" pattern="@value" value="//"/>
	</object>

	<object name="xsString" pattern="string" prototype="xsLiteral">
		<string name="value" pattern="@value" value=""/>
	</object>

	<object name="xsUndefined" pattern="undefined" prototype="xsLiteral">
		<string name="pattern" pattern=""/>
	</object>

	<object name="xsReference" pattern="reference" prototype="xsProperty">
		<string name="contents" pattern="@contents"/>
	</object>
	<function name="XSReference" prototype="xsReference"/>
	
	<object name="xsArray" pattern="array" prototype="xsReference">
	</object>
	<function name="XSArray" prototype="xsArray"/>

	<object name="xsObject" pattern="object" prototype="xsProperty">
		<string name="c" pattern="@c"/>
		<string name="patchFlag" pattern="@patch" value="true"/>
		<string name="prototype" pattern="@prototype"/>
		<array name="items" pattern="." contents="xsObjectItem, piDocumentation" />
		
		<function name="target" params="theItems, theLink, theDepth">
			var c, i;
			var items = [];
			if (this.items) {
				c = this.items.length;
				for (i = 0; i< c; i++)
					this.items[i].target(items, theLink, theDepth);
			}
			this.items = items;
			theItems[theItems.length] = this;
			this.linked = theLink;
		</function>
		</object>
	<function name="XSObject" prototype="xsObject"/>

	<object name="xsTarget" pattern="target" prototype="xsObjectItem">
		<string name="name" pattern="@name"/>
		<array name="items" pattern="." contents="xsObjectItem"/>
		
		<function name="target" params="theItems, theLink, theDepth">
			var c, i;
			if (!this.name)
				xsTool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
			else if (xsTool.hasTarget(this.name)) {
				c = this.items.length;
				for (i = 0; i < c; i++)
					this.items[i].target(theItems, theLink, theDepth);
			}
		</function>
	</object>

</package>