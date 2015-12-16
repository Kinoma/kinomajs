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
import Grammar from "grammar";
import infoset from "markup";

const ATTRIBUTE_FLAG = 1;
const DEFAULT_FLAG = 2;
const PI_FLAG = 4;
const ROOT_FLAG = 8;
const SKIP_FLAG = 16;

class PackageGrammar extends Grammar {
	constructor(it) {
		super(it);
		this.properties = new Map;
		xsPrototype.initialize(this);
		xsObjectPrototype.initialize(this);
		xsFunctionPrototype.initialize(this);
		xsArrayPrototype.initialize(this);
		xsStringPrototype.initialize(this);
		xsBooleanPrototype.initialize(this);
		xsNumberPrototype.initialize(this);
		xsDatePrototype.initialize(this);
		xsRegExpPrototype.initialize(this);
		xsErrorPrototype.initialize(this);
		xsChunkPrototype.initialize(this);
		xsNamespaces.initialize();
	}
	insertProperty(name, value, patchIt) {
		if (!patchIt) {
			var former = this.properties.get(name);
			if (former) {
				this.reportError(value.__xs__path, value.__xs__line, name + " already defined");
				this.reportError(former.__xs__path, former.__xs__line, "here");
			}
		}
		this.properties.set(name, value);	
	}
	printGrammar(file, _package) {
		file.CR(0);
		file.write("var g = new Grammar;");
		file.CR(0);
		xsNamespaces.printGrammar(file);
		_package.printGrammar(file);
		file.write("g.link();");
		file.CR(0);
	}
	searchProperty(name) {
		return this.properties.get(name);
	}
}

function trimString(theString) {
	var aString = theString; 
	var aRegExp = /^(\s*)([\W\w]*)(\b\s*$)/; 
	if (aRegExp.test(aString)) 
		aString = aString.replace(aRegExp, '$2');
	return aString; 
}

var xsDontPatchFlag = 2;
var xsDontDeleteFlag = 2;
var xsDontEnumFlag = 4;
var xsDontScriptFlag = 8;
var xsDontSetFlag = 16;
var xsGetterFlag = 32;
var xsSetterFlag = 64;

var xsPackageItem = {
	crossReference(tool, thePackage, theQualifiedName, patchIt) {},
	isAccessor(theFlag, theName) {},
	patch(tool) {},
	print() {},
	printGrammar() {},
	target(tool) {},
};

var xsPackage = {
	deleteFlag: "",
	enumFlag: "",
	qualifiedName: "this",
	scriptFlag: "",
	setFlag: "",
	items: [],
	name: null,
	crossReference(tool, thePackage, theQualifiedName, patchIt) {
		this.defaultNamespace = undefined;
		this.prefixedNamespaces = {};
		for (var item of this.items) 
			item.crossReference(tool, this, "", patchIt);
	},
	getAccessor(theFlag, theName) {
		var c = this.items.length;
		for (var i = 0; i< c; i++) {
			var anItem = this.items[i];
			if (anItem.isAccessor(theFlag, theName))
				return anItem;
		}
	},
	patch(tool) {
		var c = this.items.length;
		for (var i = 0; i< c; i++)
			this.items[i].patch(tool);
	},
	print(theFile) {
		var c = this.items.length;
		for (var i = 0; i< c; i++)
			this.items[i].print(theFile, this);
		theFile.CR(0);
	},
	printGrammar(theFile) {
		var c = this.items.length;
		for (var i = 0; i< c; i++)
			this.items[i].printGrammar(theFile);
	},
	target(tool) {
		for (var item of this.items) 
			item.target(tool);
	},
};

var xsNamespace = {
	__proto__: xsPackageItem,
	prefix: "",
	uri: "",
	crossReference(tool, thePackage, theQualifiedName, patchIt) {
		if (!this.uri)
			tool.reportError(this.__xs__path, this.__xs__line, "missing uri attribute");
		xsNamespaces.add(this);
		if (this.prefix)
			thePackage.prefixedNamespaces[this.prefix] = this;
		else
			thePackage.defaultNamespace = this;
	},
};

var xsImport = {
	__proto__: xsPackageItem,
	href: "",
	link: "static",
	package: null,
	crossReference(tool, thePackage, theQualifiedName, patchIt) {
		if (this.package)
			this.package.crossReference(tool, this.package, theQualifiedName, patchIt);
	},
	patch(tool) {
		if (this.package)
			this.package.patch(tool);
	},
	print(theFile) {
		if ((this.link == "static") && this.package)
			this.package.print(theFile);
	},
	printGrammar(theFile) {
		if ((this.link == "static") && this.package)
			this.package.printGrammar(theFile);
	},
	target(tool) {
		if (!this.href) {
			tool.reportError(this.__xs__path, this.__xs__line, "missing href attribute");
			return;
		}
		var path = tool.getPath(this.href);
		if (!path)
			tool.reportError(this.__xs__path, this.__xs__line, this.href + ": file not found");

		if ((this.link != "dynamic") && (this.link != "static"))
			tool.reportError(this.__xs__path, this.__xs__line, "the link attribute must be dynamic or static");
			
		var aPackage = tool.loadPackage(path);
		if (aPackage) {
			aPackage.target(tool);
			this.package = aPackage;
		}
	},
};

var xsObjectItem = {
	__proto__: xsPackageItem,
	print(theFile, theObject) {},
	printItems(theFile, theObject) {},
};

var xsPatch = {
	__proto__: xsObjectItem,
	prototype: "",
	items: [],
	crossReference(tool, thePackage, theQualifiedName, patchIt) {
		var c, i;
		theQualifiedName = this.prototype + ".";
		patchIt = false;
		if (this.prototype) {
			this.prototype = g.searchProperty(this.prototype);
			if (!this.prototype)
				tool.reportError(this.__xs__path, this.__xs__line, "prototype does not exist");
			else if (!(xsObject.isPrototypeOf(this.prototype)))
				tool.reportError(this.__xs__path, this.__xs__line, "prototype is no object");
			else if (!this.prototype.patchFlag)
				tool.reportError(this.__xs__path, this.__xs__line, "cannot patch prototype");
			else
				patchIt = true;
		}
		else
			tool.reportError(this.__xs__path, this.__xs__line, "missing prototype attribute");
		for (var item of this.items) 
			item.crossReference(tool, thePackage, theQualifiedName, true);
	},
	getAccessor(theFlag, theName) {
		var c = this.items.length;
		for (var i = 0; i< c; i++) {
			var anItem = this.items[i];
			if (anItem.isAccessor(theFlag, theName)) {
				anItem.object = this;
				return anItem;
			}
		}
		return this.prototype.getAccessor(theFlag, theName);
	},
	patch(tool) {
		var c = this.items.length;
		for (var i = 0; i < c; i++)
			this.items[i].patch(tool, this);
	},
	print(theFile) {
		var c = this.items.length;
		for (var i = 0; i< c; i++)
			this.items[i].print(theFile, this.prototype);
	},
	printGrammar(theFile) {
		var c = this.items.length;
		for (var i = 0; i< c; i++)
			this.items[i].printGrammar(theFile);
	},
	target(tool) {
		for (var item of this.items) 
			item.target(tool);
	},
};

var xsProgram = {
	__proto__: xsObjectItem,
	c: "",
	href: "",
	value: "",
	crossReference(tool, thePackage, theQualifiedName, patchIt) {
		if (this.hasOwnProperty("c")) {
			if (this.c == "")
				tool.reportError(this.__xs__path, this.__xs__line, 
						"the c attribute cannot be empty");
			if (this.hasOwnProperty("value"))
				tool.reportError(this.__xs__path, this.__xs__line, 
						"a program element with a c attribute must be empty");
			else if (this.hasOwnProperty("href"))
				tool.reportError(this.__xs__path, this.__xs__line, 
						"a program element cannot have both c and href attributes");
		}
		else if (this.hasOwnProperty("href")) {
			if (this.hasOwnProperty("value"))
				tool.reportError(this.__xs__path, this.__xs__line, 
						"a program element with an href attribute must be empty");
			else {
				var aPath = tool.getPath(this.href);
				if (!aPath)
					tool.reportError(this.__xs__path, this.__xs__line, this.href + ": file not found");
				this.href = aPath;
			}
		}
	},
	print(theFile) {
		if (this.hasOwnProperty("c")) {
			theFile.CR(0);
			theFile.write("(function () @ \"");
			theFile.write(this.c);
			theFile.write("\")();");
		}
		else if (this.hasOwnProperty("href")) {
			theFile.writePathLine(this.href, 1);
			theFile.CR(0);
			theFile.include(aPath);
		}
		else {
			theFile.writePathLine(this.__xs__path, this.__xs__line);
			theFile.CR(0);
			theFile.write("// program");
			theFile.CR(0);
			theFile.write(this.value);
		}
	},
};

var xsTarget = {
	__proto__: xsObjectItem,
	name: "",
	items: [],
	flag: false,
	crossReference(tool, thePackage, theQualifiedName, patchIt) {
		if (this.flag) {
			for (var item of this.items) 
				item.crossReference(tool, thePackage, theQualifiedName, patchIt);
		}
	},
	patch(tool) {
		if (this.flag) {
			var c = this.items.length;
			for (var i = 0; i< c; i++)
				this.items[i].patch(tool);
		}
	},
	print(theFile, theObject) {
		if (this.flag) {
			var c = this.items.length;
			for (var i = 0; i< c; i++)
				this.items[i].print(theFile, theObject);
		}
	},
	printGrammar(theFile, theObject) {
		if (this.flag) {
			var c = this.items.length;
			for (var i = 0; i< c; i++)
				this.items[i].printGrammar(theFile, theObject);
		}
	},
	target(tool) {
		if (!this.name)
			tool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
		this.flag = tool.hasTarget(this.name);
		if (this.flag) {
			for (var item of this.items) 
				item.target(tool);
		}
	},
};

var xsProperty = {
	__proto__: xsObjectItem,
	name: "",
	pattern: "",
	patternFlags: 0,
	deleteFlag: "",
	enumFlag: "",
	scriptFlag: "",
	setFlag: "",
	masks: 0,
	flags: 0,
	nextProperty: null,
	references: null,
	addReference(theReference) {
		if (this.references)
			this.references[this.references.length] = theReference;
		else
			this.references = [ theReference ];
	},
	checkPattern() {
	},
	crossReference(tool, thePackage, theQualifiedName, patchIt) {
		this.qualifiedName = theQualifiedName + this.name;
		if (this.name)
			g.insertProperty(this.qualifiedName, this, patchIt);
		else
			tool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
		
		if (this.pattern) {
			tool.grammar = true;
			this.parsePattern(thePackage);
			this.checkPattern();
		}

		if (!this.hasOwnProperty("deleteFlag"))
			this.deleteFlag = thePackage.deleteFlag;
		if (!this.hasOwnProperty("enumFlag"))
			this.enumFlag = thePackage.enumFlag;
		if (!this.hasOwnProperty("scriptFlag"))
			this.scriptFlag = thePackage.scriptFlag;
		if (!this.hasOwnProperty("setFlag"))
			this.setFlag = thePackage.setFlag;
			
		if (this.deleteFlag) {
			this.masks += xsDontDeleteFlag;
			if (this.deleteFlag == "false")
				this.flags += xsDontDeleteFlag;
		}
		if (this.enumFlag) {
			this.masks += xsDontEnumFlag;
			if (this.enumFlag == "false")
				this.flags += xsDontEnumFlag;
		}
		if (this.scriptFlag) {
			this.masks += xsDontScriptFlag;
			if (this.scriptFlag == "false")
				this.flags += xsDontScriptFlag;
		}
		if (this.setFlag) {
			this.masks += xsDontSetFlag;
			if (this.setFlag == "false")
				this.flags += xsDontSetFlag;
		}
	},
	fixName(thePackage, name, flag) {
		var split = name.split(":");
		if (split.length == 2) {
			var prefix = split[0];
			if (prefix in thePackage.prefixedNamespaces) {
				var namespace = thePackage.prefixedNamespaces[prefix];
				if (prefix != namespace.commonPrefix) {
					prefix = namespace.commonPrefix;
					return prefix + ":" + split[1];
				}
			}
			else
				tool.reportError(this.__xs__path, this.__xs__line, "pattern namespace not found");
		}
		else if (flag && thePackage.defaultNamespace) {
			return thePackage.defaultNamespace.commonPrefix + ":" + name;
		}
		return name;
	},
	parsePattern(thePackage) {
		var aSlash, aColon, aPrefix, aNamespace, 
		var pattern = this.pattern;
		var flags = 0;
		if (pattern == "")
			flags |= SKIP_FLAG;
		else if (pattern == ".")
			flags |= DEFAULT_FLAG;
		else {
			var names;
			if (pattern.charAt(0) == "/") {
				flags |= ROOT_FLAG;
				names = pattern.split("/");
				names.shift();
			}
			else
				names = pattern.split("/");
			var c = names.length;
			for (var i = 0; i < c; i++) {
				var name = names[i];
				if (name == ".") {
					if (i < (c - 1))
						tool.reportError(this.__xs__path, this.__xs__line, "invalid pattern");
					flags |= DEFAULT_FLAG;
				}
				else if (name == "..") 
					tool.reportError(this.__xs__path, this.__xs__line, "invalid pattern");
				else if (name.charAt(0) == "@") {
					if (i < (c - 1))
						tool.reportError(this.__xs__path, this.__xs__line, "invalid pattern");
					flags |= ATTRIBUTE_FLAG;
					names[i] = "@" + this.fixName(thePackage, name.substring(1, name.length));
				}
				else if (name.charAt(0) == "?") {
					if (i < (c - 1))
						tool.reportError(this.__xs__path, this.__xs__line, "invalid pattern");
					flags |= PI_FLAG;
					names[i] = "?" + this.fixName(thePackage, name.substring(1, name.length), true);
				}
				else {
					names[i] = this.fixName(thePackage, name, true);
				}
			}
			pattern = names.join("/");
			if (flags & ROOT_FLAG)
				pattern = "/" + pattern;
		}
		this.pattern = pattern;
		this.patternFlags = flags;
	},
	print(theFile, theObject) {
		theFile.CR(0);
		if (theObject.qualifiedName != "this") {
			theFile.write(theObject.qualifiedName);
			theFile.write(".");
			theFile.writeName(this.name);
			theFile.write(" = ");
			this.printValue(theFile);
			theFile.write(";");
			/*
			theFile.write("Object.defineProperty(");
			theFile.write(theObject.qualifiedName);
			theFile.write(", \"");
			theFile.writeName(this.name);
			theFile.write("\", {");
			if (!(this.flags & xsDontDeleteFlag))
				theFile.write(" configurable: true,");
			if (!(this.flags & xsDontEnumFlag))
				theFile.write(" enumerable: true,");
			if (!(this.flags & xsDontSetFlag))
				theFile.write(" writable: true,");
			theFile.write(" value: ");
			if (!this.printValue(theFile))
				theFile.write(" ");
			theFile.write("});");
			*/
		}
		else {
			theFile.write("var ");
			theFile.write(this.name);
			theFile.write(" = ");
			this.printValue(theFile);
			theFile.write(";");
		}
		this.printItems(theFile, theObject);
	},
	printValue(theFile) {
		debugger;
	},
};

var xsLiteral = {
	__proto__: xsProperty,
	checkPattern() {
		if (this.patternFlags & ROOT_FLAG)
			tool.reportError(this.__xs__path, this.__xs__line, "invalid pattern");
	},
	printGrammar(theFile, theObject) {
		if (!this.pattern) return;
		if (theObject) {
			theFile.writeName(this.name);
			theFile.write(": ");
			this.printGrammarCall(theFile, this.pattern);
		}
		else if (this.references) {
			theFile.write("g.data(");
			theFile.write(this.qualifiedName);
			theFile.write(", \"");
			theFile.write(this.pattern);
			theFile.write("\", ");
			this.printGrammarCall(theFile, ".");
			theFile.write(");");
			theFile.CR(0);
		}
	},
};

var xsBoolean = {
	__proto__: xsLiteral,
	value: "false",
	printValue(theFile) {
		theFile.write(this.value);
	},
	printGrammarCall(theFile, thePattern) {
		theFile.write("g.boolean(\"");
		theFile.write(thePattern);
		theFile.write("\")");
	},
};

var xsChunk = {
	__proto__: xsLiteral,
	value: "new Chunk",
	printValue(theFile) {
		theFile.write(this.value);
	},
	printGrammarCall(theFile, thePattern) {
		theFile.write("g.chunk(\"");
		theFile.write(thePattern);
		theFile.write("\")");
	},
};

var xsCustom = {
	__proto__: xsLiteral,
	io: "",
	value: "",
	crossReference(tool, thePackage, theQualifiedName, patchIt) {
		xsProperty.crossReference.call(this, tool, thePackage, theQualifiedName, patchIt);
		if (this.hasOwnProperty("io")) {
			this.io = g.searchProperty(this.io);
			if (!this.io)
				tool.reportError(this.__xs__path, this.__xs__line, "io does not exist");
			else if (!(xsObject.isPrototypeOf(this.io)))
				tool.reportError(this.__xs__path, this.__xs__line, "io is no object");
		}
		else
			tool.reportError(this.__xs__path, this.__xs__line, "missing io attribute");
	},
	printValue(theFile) {
		theFile.write(this.io.qualifiedName);
		theFile.write(".parse(\"");
		theFile.write(this.value);
		theFile.write("\")");
	},
	printGrammarCall(theFile, thePattern) {
		theFile.write("g.custom(\"");
		theFile.write(thePattern);
		theFile.write("\", ");
		theFile.write(this.io.qualifiedName);
		theFile.write(")");
	},
};

var xsDate = {
	__proto__: xsLiteral,
	value: "01 Jan 1970 00:00:00 GMT",
	printValue(theFile) {
		theFile.write("Date.parse(\"")
		theFile.write(this.value);
		theFile.write("\")");
	},
	printGrammar(theFile, theObject) {
		if (!theObject) return;
		if (!this.pattern) return;
		theFile.writeName(this.name);
		theFile.write(": g.date(\"");
		theFile.write(this.pattern);
		theFile.write("\")");
	},
	printGrammarCall(theFile, thePattern) {
		theFile.write("g.date(\"");
		theFile.write(thePattern);
		theFile.write("\")");
	},
};

var xsFunction = {
	__proto__: xsLiteral,
	c: "",
	check: "",
	params: "",
	pattern: "",
	prototype: "",
	value: "",
	other: null,
	crossReference(tool, thePackage, theQualifiedName, patchIt) {
		var result = xsProperty.crossReference.call(this, tool, thePackage, theQualifiedName, patchIt);
		if (this.name.match(/^set .+/)) {
			this.name = this.name.substring(4, this.name.length);
			this.masks += xsSetterFlag;
			this.flags += xsSetterFlag;
		}
		else if (this.name.match(/^get .+/)) {
			this.name = this.name.substring(4, this.name.length);
			this.masks += xsGetterFlag;
			this.flags += xsGetterFlag;
		}
		if (this.hasOwnProperty("c")) {
			if (this.c == "")
				tool.reportError(this.__xs__path, this.__xs__line, 
						"the c attribute cannot be empty");
			if (this.hasOwnProperty("value"))
				tool.reportError(this.__xs__path, this.__xs__line, 
						"a function element with a c attribute must be empty");
		}
		if (this.prototype) {
			if (this.flags & xsGetterFlag)
				tool.reportError(this.__xs__path, this.__xs__line, "a getter has no prototype");
			else if (this.flags & xsSetterFlag)
				tool.reportError(this.__xs__path, this.__xs__line, "a setter has no prototype");
			else {
				this.prototype = g.searchProperty(this.prototype);
				if (!this.prototype)
					tool.reportError(this.__xs__path, this.__xs__line, "prototype does not exist");
				else if (!(xsObject.isPrototypeOf(this.prototype)))
					tool.reportError(this.__xs__path, this.__xs__line, "prototype is no object");
			}
		}
		return result;
	},
	isAccessor(theFlag, theName) {
		return (this.flags & theFlag) && (this.name == theName);
	},
	patch(tool, theObject) {
		var other;
		if ((this.flags & xsGetterFlag) && (this.other == null) && theObject) {
			other = theObject.getAccessor(xsSetterFlag, this.name);
			if (other) {
				this.other = other
				if (other.object == theObject)
					other.other = this;
			}
		}
		else if ((this.flags & xsSetterFlag) && (this.other == null) && theObject) {
			other = theObject.getAccessor(xsGetterFlag, this.name);
			if (other) {
				this.other = other
				if (other.object == theObject)
					other.other = this;
			}
		}
	},
	print(theFile, theObject) {
		var getter, setter;
		if (this.flags & xsGetterFlag) {
			getter = this;
			setter = this.other;
		}
		else if (this.flags & xsSetterFlag) {
			getter = this.other;
			setter = this;
			if (getter && (getter.other == this))
				return;
		}
		if ((getter || setter)) {
			theFile.CR(0);
			theFile.write("Object.defineProperties(");
			theFile.write(theObject.qualifiedName);
			theFile.write(", { ");
			theFile.writeName(this.name);
			theFile.write(": { ");
			if ((getter && (!(getter.flags & xsDontDeleteFlag))) || (setter && (!(setter.flags & xsDontDeleteFlag))))
				theFile.write("configurable: true, ");
			if ((getter && (!(getter.flags & xsDontEnumFlag))) || (setter && (!(setter.flags & xsDontEnumFlag))))
				theFile.write("enumerable: true, ");
			if (getter) {
				theFile.CR(0);
				theFile.write("\tget: ");
				if (getter.c) {
					theFile.write("function() @ \"");
					theFile.write(getter.c);
					theFile.write("\"");
				}
				else {
					theFile.writePathLine(getter.__xs__path, getter.__xs__line);
					theFile.CR(0);
					theFile.write("\tfunction() {");
					theFile.CR(0);
					if (getter.value) {
						theFile.write(getter.value);
						theFile.write("}");
					}
					else {
						theFile.write("\t\}");
					}
				}
				theFile.write(",");
			}
			if (setter) {
				theFile.CR(0);
				theFile.write("\tset: ");
				if (setter.c) {
					theFile.write("function(");
					theFile.write(setter.params);
					theFile.write(") @ \"");
					theFile.write(setter.c);
					theFile.write("\"");
				}
				else {
					theFile.writePathLine(setter.__xs__path, setter.__xs__line);
					theFile.CR(0);
					theFile.write("\tfunction(");
					theFile.write(setter.params);
					theFile.write(") {");
					theFile.CR(0);
					if (setter.value) {
						theFile.write(setter.value);
						theFile.write("}");
					}
					else {
						theFile.write("\t\}");
					}
				}
				theFile.write(",");
			}
			theFile.CR(0);
			theFile.write("}});");
		}
		else if (theObject.qualifiedName != "this") {
			theFile.CR(0);
			theFile.write("Object.defineProperties(");
			theFile.write(theObject.qualifiedName);
			theFile.write(", { ");
			theFile.writeName(this.name);
			theFile.write(": { ");
			if (!(this.flags & xsDontDeleteFlag))
				theFile.write("configurable: true, ");
			if (!(this.flags & xsDontEnumFlag))
				theFile.write("enumerable: true, ");
			if (!(this.flags & xsDontSetFlag))
				theFile.write("writable: true, ");
			theFile.write("value: ");
			if (this.c) {
				theFile.write("function(");
				theFile.write(this.params);
				theFile.write(") @ \"");
				theFile.write(this.c);
				theFile.write("\"");
			}
			else {	
				theFile.writePathLine(this.__xs__path, this.__xs__line);
				theFile.CR(0);
				theFile.write("\tfunction(");
				theFile.write(this.params);
				theFile.write(") {");
				theFile.CR(0);
				if (this.value) {
					theFile.write(this.value);
					theFile.write("}");
				}
				else {
					theFile.write("\t\}");
				}
				theFile.CR(0);
			}
			theFile.write("}});");
			if (this.prototype)
				this.printConstructor(theFile, theObject);
		}
		else {
			xsLiteral.print.call(this, theFile, theObject);
			if (this.prototype)
				this.printConstructor(theFile, theObject);
		}
	},
	printConstructor(theFile) {
		theFile.CR(0);
		theFile.write("Object.defineProperties(");
		theFile.write(this.qualifiedName);
		theFile.write(", { prototype: { value: ");
		theFile.write(this.prototype.qualifiedName);
		theFile.write(", writable:true }});");
	},
	printValue(theFile) {
		if (this.c) {
			theFile.write("function(");
			theFile.write(this.params);
			theFile.write(") @ \"");
			theFile.write(this.c);
			theFile.write("\"");
		}
		else {	
			theFile.writePathLine(this.__xs__path, this.__xs__line);
			theFile.CR(0);
			theFile.write("\tfunction(");
			theFile.write(this.params);
			theFile.write(") {");
			theFile.CR(0);
			if (this.value) {
				theFile.write(this.value);
				theFile.write("}");
			}
			else {
				theFile.write("\t\}");
			}
			theFile.CR(0);
		}
		return true;
	},
};

var xsNull = {
	__proto__: xsLiteral,
	pattern: "",
	printValue(theFile) {
		theFile.write("null");
	},
};

var xsNumber = {
	__proto__: xsLiteral,
	value: "0",
	printValue(theFile) {
		theFile.write(this.value);
	},
	printGrammarCall(theFile, thePattern) {
		theFile.write("g.number(\"");
		theFile.write(thePattern);
		theFile.write("\")");
	},
};

var xsRegExp = {
	__proto__: xsLiteral,
	value: "//",
	printValue(theFile) {
		theFile.write(this.value);
	},
	printGrammarCall(theFile, thePattern) {
		theFile.write("g.regexp(\"");
		theFile.write(thePattern);
		theFile.write("\")");
	},
};

var xsScript = {
	__proto__: xsLiteral,
	pattern: "",
	print(theFile) {
	},
};

var xsString = {
	__proto__: xsLiteral,
	value: "",
	printValue(theFile) {
		theFile.write("\"");
		theFile.write(this.value);
		theFile.write("\"");
	},
	printGrammarCall(theFile, thePattern) {
		theFile.write("g.string(\"");
		theFile.write(thePattern);
		theFile.write("\")");
	},
};

var xsUndefined = {
	__proto__: xsLiteral,
	pattern: "",
	printValue(theFile) {
		theFile.write("undefined");
	},
};

var xsReference = {
	__proto__: xsProperty,
	contents: "",
	checkPattern() {
		if (this.patternFlags & ATTRIBUTE_FLAG)
			tool.reportError(this.__xs__path, this.__xs__line, "invalid @ pattern");
		if (this.patternFlags & PI_FLAG)
			tool.reportError(this.__xs__path, this.__xs__line, "invalid ? pattern");
		if (this.patternFlags & ROOT_FLAG)
			tool.reportError(this.__xs__path, this.__xs__line, "invalid / pattern");
	},
	crossReference(tool, thePackage, theQualifiedName, patchIt) {
		var c, i, aName, aPrototype;
	
		xsProperty.crossReference.call(this, tool, thePackage, theQualifiedName, patchIt);
		if (this.contents) {
			this.contents = this.contents.split(",");
			c = this.contents.length;
			for (i = 0; i < c; i++) {
				aName = trimString(this.contents[i]);
				aPrototype = g.searchProperty(aName);
				if (!aPrototype)
					tool.reportError(this.__xs__path, this.__xs__line, 
							aName + " does not exist");
				else if (xsReference.isPrototypeOf(aPrototype))
					tool.reportError(this.__xs__path, this.__xs__line, 
							aName + " is a reference");
				else if (xsArray.isPrototypeOf(aPrototype))
					tool.reportError(this.__xs__path, this.__xs__line, 
							aName + " is an array");
				else if (xsObject.isPrototypeOf(aPrototype)) {
					aPrototype.addReference(this);		
					this.contents[i] = aPrototype;
				}
				else if (aPrototype.hasOwnProperty("pattern")) {
					aPrototype.addReference(this);		
					this.contents[i] = aPrototype;
				}
				else
					tool.reportError(this.__xs__path, this.__xs__line, 
							aName + " has no pattern");
			}
		}
		else if (!xsArray.isPrototypeOf(this))
			tool.reportError(this.__xs__path, this.__xs__line, "missing contents attribute");
	},
	printValue(theFile) {
		theFile.write(this.contents[0].qualifiedName);
	},
	printGrammar(theFile, theObject) {
		if (!theObject) return;
		if (!this.pattern) return;
		theFile.writeName(this.name);
		theFile.write(": g.reference(\"");
		theFile.write(this.pattern);
		theFile.write("\"");
		var c = this.contents.length;
		for (var i = 0; i < c; i++) {
			theFile.write(", ");
			theFile.write(this.contents[i].qualifiedName);
		}
		theFile.write(")");
	},
};

var xsArray = {
	__proto__: xsReference,
	printValue(theFile) {
		theFile.write("[]");
	},
	printGrammar(theFile, theObject) {
		if (!theObject) return;
		if (!this.pattern) return;
		theFile.writeName(this.name);
		theFile.write(": g.array(\"");
		theFile.write(this.pattern);
		theFile.write("\"");
		var c = this.contents.length;
		for (var i = 0; i < c; i++) {
			theFile.write(", ");
			theFile.write(this.contents[i].qualifiedName);
		}
		theFile.write(")");
	},
};

var xsObject = {
	__proto__: xsProperty,
	c: "",
	prototype: "",
	items: [],
	patchFlag: true,
	grammarFlag: false,
	checkPattern() {
		if (this.patternFlags & ATTRIBUTE_FLAG)
			tool.reportError(this.__xs__path, this.__xs__line, "invalid @ pattern");
		if (this.patternFlags & PI_FLAG)
			tool.reportError(this.__xs__path, this.__xs__line, "invalid ? pattern");
	},
	crossReference(tool, thePackage, theQualifiedName, patchIt) {
		var c, i, items;
		
		xsProperty.crossReference.call(this, tool, thePackage, theQualifiedName, patchIt);
		if (this.c) {
			if (this.prototype)
				tool.reportError(this.__xs__path, this.__xs__line, "a host object has no prototype");
		}
		else if (this.prototype) {
			this.prototype = g.searchProperty(this.prototype);
			if (!this.prototype)
				tool.reportError(this.__xs__path, this.__xs__line, "prototype does not exist");
			else if (!(xsObject.isPrototypeOf(this.prototype)))
				tool.reportError(this.__xs__path, this.__xs__line, "prototype is no object");
		}
		theQualifiedName += this.name + ".";
		for (var item of this.items) 
			item.crossReference(tool, thePackage, theQualifiedName, patchIt);
	},
	findItemIndex(name) {
		var c = this.items.length;
		for (var i = 0; i < c; i++) {
			if (this.items[i].name == name)
				return i;
		}
		return -1;
	},
	getAccessor(theFlag, theName) {
		var c = this.items.length;
		for (var i = 0; i< c; i++) {
			var anItem = this.items[i];
			if (anItem.isAccessor(theFlag, theName)) {
				anItem.object = this;
				return anItem;
			}
		}
		if (this.prototype)
			return this.prototype.getAccessor(theFlag, theName);
	},
	patch(tool) {
		var c = this.items.length;
		for (var i = 0; i < c; i++)
			this.items[i].patch(tool, this);
	},
	printGrammar(theFile, theObject) {
		if (theObject) {
			if (!this.pattern) return;
			theFile.writeName(this.name);
			theFile.write(": g.reference(\"");
			theFile.write(this.pattern);
			theFile.write("\", ");
			theFile.write(this.qualifiedName);
			theFile.write(")");
			return;
		}
		var c = this.items.length;
		for (var i = 0; i< c; i++)
			this.items[i].printGrammar(theFile);
		if (this.pattern || this.references) {
			var flag = true;
			theFile.write("g.object(");
			theFile.write(this.qualifiedName);
			theFile.write(", \"");
			theFile.write(this.pattern);
			theFile.write("\", {");
			for (var i = 0; i< c; i++) {
				var item = this.items[i];
				if (item.pattern) {
					if (flag) {
						theFile.CR(1);
						flag = false;
					}
					else {
						theFile.write(",");
						theFile.CR(0);
					}
					item.printGrammar(theFile, this);
				}
			}
			if (!flag)
				theFile.CR(-1);
			theFile.write("});");
			theFile.CR(0);
		}
	},
	printItems(theFile, theObject) {
		theFile.tabCount++;
		var c = this.items.length;
		for (var i = 0; i< c; i++)
			this.items[i].print(theFile, this);
		theFile.tabCount--;
		//theFile.CR(0);
		return true;
	},
	printValue(theFile, theObject) {
		if (this.c) {
			theFile.write("@ \"");
			theFile.write(this.c);
			theFile.write("\"");
		}
		else if (this.prototype) {
			theFile.write("Object.create(");
			theFile.write(this.prototype.qualifiedName);
			theFile.write(")");
		}
		else {
			theFile.write("{}");
		}
	},
	target(tool) {
		for (var item of this.items) 
			item.target(tool);
	},
};

var xsPrototype = {
	__proto__: xsObject,
	name: "xs",
	initialize(g) {
		this.qualifiedName = this.name;
		g.insertProperty(this.qualifiedName, this, false);
	},
};

var xsECMAPrototype = {
	__proto__: xsObject,
	name: "",
	initialize(g) {
		this.qualifiedName = this.name + ".prototype";
		g.insertProperty(this.qualifiedName, this, false);
	},
};

var xsObjectPrototype = {
	__proto__: xsECMAPrototype,
	name: "Object",
};

var xsFunctionPrototype = {
	__proto__: xsECMAPrototype,
	name: "Function",
};

var xsArrayPrototype = {
	__proto__: xsECMAPrototype,
	name: "Array",
};

var xsStringPrototype = {
	__proto__: xsECMAPrototype,
	name: "String",
};

var xsBooleanPrototype = {
	__proto__: xsECMAPrototype,
	name: "Boolean",
};

var xsNumberPrototype = {
	__proto__: xsECMAPrototype,
	name: "Number",
};

var xsDatePrototype = {
	__proto__: xsECMAPrototype,
	name: "Date",
};

var xsRegExpPrototype = {
	__proto__: xsECMAPrototype,
	name: "RegExp",
};

var xsErrorPrototype = {
	__proto__: xsECMAPrototype,
	name: "Error",
};

var xsChunkPrototype = {
	__proto__: xsECMAPrototype,
	name: "Chunk",
};

var xsNamespaces = {
	index: 0,
	add(theNamespace) {
		if (theNamespace.uri in this.items) {
			theNamespace.commonPrefix = this.items[theNamespace.uri];
		}
		else {
			var aPrefix = theNamespace.prefix;
			if (!aPrefix) {
				aPrefix = "xs" + this.index;
				this.index++;
			}
			this.items[theNamespace.uri] = aPrefix;
			theNamespace.commonPrefix = aPrefix;
		}
	},
	initialize() {
		this.items = {};
	},
	printGrammar(theFile) {
		for (var i in this.items) {
			var prefix = this.items[i];
			theFile.write("g.namespace(\"");
			theFile.write(i);
			if (prefix) {
				theFile.write("\", \"");
				theFile.write(prefix);
			}
			theFile.write("\");");
			theFile.CR(0);
		}
	},
};

var xsPropertyList = {
	flags: undefined,
	add(theProperty) {
		var result = this.items.length;
		this.items[result] = theProperty;
		return result;
	},
	initialize() {
		this.items = [];
	},
	printExterns(theFile) {
		var c = this.items.length;
		for (var i = 0; i< c; i++)
			this.items[i].printExtern(theFile);
	},
	printHosts(theFile) {
		var c = this.items.length;
		for (var i = 0; i< c; i++)
			this.items[i].printHost(theFile);
	},
};

var g = new PackageGrammar(infoset);
g.object(xsPackageItem, "", {});
g.object(xsPackage, "/package", {
	cFlag: g.string("@c"),
	deleteFlag: g.string("@delete"),
	enumFlag: g.string("@enum"),
	scriptFlag: g.string("@script"),
	setFlag: g.string("@set"),
	items: g.array(".", xsPackageItem)
});
g.object(xsNamespace, "namespace", {
	prefix: g.string("@prefix"),
	uri: g.string("@uri")
});
g.object(xsImport, "import", {
	href: g.string("@href"),
	link: g.string("@link")
});
g.object(xsObjectItem, "", {});
g.object(xsPatch, "patch", {
	prototype: g.string("@prototype"),
	items: g.array(".", xsObjectItem)
});
g.object(xsProgram, "program", {
	c: g.string("@c"),
	href: g.string("@href"),
	value: g.string(".")
});
g.object(xsProperty, "", {
	name: g.string("@name"),
	pattern: g.string("@pattern"),
	deleteFlag: g.string("@delete"),
	enumFlag: g.string("@enum"),
	scriptFlag: g.string("@script"),
	setFlag: g.string("@set"),
});
g.object(xsBoolean, "boolean", {
	value: g.string("@value")
});
g.object(xsChunk, "chunk", {
	value: g.string("@value")
});
g.object(xsCustom, "custom", {
	io: g.string("@io"),
	value: g.string("@value")
});
g.object(xsDate, "date", {
	value: g.string("@value")
});
g.object(xsFunction, "function", {
	c: g.string("@c"),
	check: g.string("@check"),
	params: g.string("@params"),
	prototype: g.string("@prototype"),
	value: g.string(".")
});
g.object(xsNull, "null", {});
g.object(xsNumber, "number", {
	value: g.string("@value")
});
g.object(xsRegExp, "regexp", {
	value: g.string("@value")
});
g.object(xsString, "string", {
	value: g.string("@value")
});
g.object(xsUndefined, "undefined", {});
g.object(xsReference, "reference", {
	contents: g.string("@contents")
});
g.object(xsArray, "array", {});
g.object(xsObject, "object", {
	c: g.string("@c"),
	patchFlag: g.string("@patch"),
	prototype: g.string("@prototype"),
	items: g.array(".", xsObjectItem)
});
g.object(xsTarget, "target", {
	name: g.string("@name"),
	items: g.array(".", xsObjectItem)
});
g.link();
export default g;

