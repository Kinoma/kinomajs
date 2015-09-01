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
import * as FS from "fs";
import Grammar from "grammar";
import infoset from "markup";

var xsMakefileItem = {
	configure(tool, manifest, tree) {},
	push(tree, property, string, buildType) {
		if (property in tree) {
			var array = tree[property];
			if (array.indexOf(string) < 0)
				tree[property].push(string);
		}
		else
			tree[property] = [ string ];
	}
};
var xsMakefileNamedItem = {
	__proto__: xsMakefileItem,
	name: "",
	property: "",
	configure(tool, manifest, tree, buildType) {
		var property = buildType ? this.property + buildType : this.property;
		if (!this.name)
			tool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
		else
			this.push(tree, property, this.name);
	},
};
var xsMakefileList = {
	__proto__: xsMakefileItem,
	items: [],
	configureItems(tool, manifest, tree, buildType) {
		for (var item of this.items)
			item.configure(tool, manifest, tree, buildType);
	},
};
var xsMakefile = {
	__proto__: xsMakefileList,
	items: [],
	configure(tool, manifest, tree) {
		this.push(tree, "cIncludes", tool.currentDirectory);
		this.push(tree, "cIncludes", tool.currentDirectory + "/sources");
		this.configureItems(tool, manifest, tree);
	},
};
var xsInput = {
	__proto__: xsMakefileNamedItem,
	property: "cIncludes",
	configure(tool, manifest, tree) {
		if (!this.name)
			tool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
		else {
			var path = tool.resolveDirectoryPath(this.name);
			if (path) {
				if (path == tool.xsIncludes)
					path = tool.xs6Includes;
				tool.inputDirectories.push(path);
				this.push(tree, this.property, path);
			}
			else
				tool.reportError(this.__xs__path, this.__xs__line, this.name + ": directory not found");
		}
	},
};
var xsConfiguration = {
	__proto__: xsMakefileList,
	configure(tool, manifest, tree) {
		if (!this.name)
			xs2js.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
		if (this.name == "debug") {
			if (tool.cmake)
				this.configureItems(tool, manifest, tree, "Debug");
			else
				if (tool.debug)
					this.configureItems(tool, manifest, tree);
		}
		else if (this.name == "release") {
			if (tool.cmake)
					this.configureItems(tool, manifest, tree, "Release");
			else
				if (!tool.debug)
					this.configureItems(tool, manifest, tree);
		}
		else
			this.configureItems(tool, manifest, tree);
	},
};
var xsPlatform = {
	__proto__: xsMakefileList,
	configure(tool, manifest, tree) {
		if (!this.name)
			xs2js.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
		else if (tool.checkPlatform(this.name))
			this.configureItems(tool, manifest, tree);
	},
};
var xsCOption = {
	__proto__: xsMakefileNamedItem,
	property: "cOptions",
};
var xsASMOption = {
	__proto__: xsMakefileNamedItem,
	property: "asmOptions",
};
var cmakeOptions = {
	__proto__: xsMakefileItem,
	type: "",
	depends: "",
	links: "",
	configure(tool, manifest, tree) {
		if (!this.name)
			tool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
		else {
			if (!("cmakeOptions" in tree))
				tree.cmake = {};
			tree.cmake[this.name] = {
				type: this.type,
				depends: this.depends,
				links: this.links
			};
		}
	},
};
var xsLibrary = {
	__proto__: xsMakefileItem,
	configure(tool, manifest, tree) {
		if (!this.name)
			tool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
		else {
			tool.insertUnique(manifest.libraries, this.name);
		}
	},
};
var xsSource = {
	__proto__: xsMakefileNamedItem,
	property: "sources",
	configure(tool, manifest, tree) {
		if (!this.name)
			tool.reportError(this.__xs__path, this.__xs__line, "missing name attribute");
		else {
			var path = tool.getPath(this.name);
			if (path)
				this.push(tree, this.property, path);
			else
				tool.reportError(this.__xs__path, this.__xs__line, this.name + ": file not found");
		}
	},
};
var xsHeader = {
	__proto__: xsSource,
	property: "headers",
};
var xsBuildStyle = {
	__proto__: xsMakefileList,
};
var xsCommon = {
	__proto__: xsMakefileItem,
	text: "",
	concatCOptions(tool, manifest, tree, text, buildType) {
		var options = "";
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
		options = options.replace("$(DEFAULT_C_OPTIONS)", "");
		options = options.replace("$(COMMON_C_OPTIONS)", "");
		options = options.replace(/\s+/g, " ");
		var split = options.split(" ");
		var c = split.length;
		var property = "cOptions";
		if (buildType)
			property += buildType;
		for (var i = 0; i < c; i++) {
			var name = split[i];
			if (name == "/D") {
				i++;
				this.push(tree, property, name + " " + split[i]);
			}
			else if (name)
				this.push(tree, property, name);
		}
	},
	concatLibraries(tool, manifest, tree, text) {
		var libraries = "";
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
		libraries = libraries.replace("$(LIBRARIES)", "");
		libraries = libraries.replace("$(DEFAULT_LIBRARIES)", "");
		libraries = libraries.replace("$(COMMON_LIBRARIES)", "");
		libraries = libraries.replace(/\s+/g, " ");
		var split = libraries.split(" ");
		var c = split.length;
		for (var i = 0; i < c; i++) {
			var name = split[i];
			if (name == "-framework") {
				i++;
				tool.insertUnique(manifest.libraries, name + " " + split[i]);
			} else if (name.substring(0) == "#") {
				continue;
			} else if (name) {
				if (tree.separate)
					this.push(tree, "libraries", name);
				else
					tool.insertUnique(manifest.libraries, name);
			}
		} 
	},
	concatObjects(tool, manifest, tree, text) {
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
		for (var i = 0; i < c; i++) {
			var item = results[i];
			var index = item.lastIndexOf("$(F_HOME)");
			if (index >= 0) {
				var name = item.slice(index);
				var path = tool.resolveFilePath(name);
				if (path)
					this.push(tree, "sources", path);
				else
					tool.reportError(this.__xs__path, this.__xs__line, name + ": file not found");
			}
		}
	},
	concatXSCOptions(tool, manifest, tree, text) {
		var options = "";
		var regexp = /^XSC_OPTIONS\s+\+?=\s+(.+)$/gm;
		var results = regexp.exec(text);
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
		options = options.replace("$(XSC_OPTIONS)", "");
		options = options.replace("$(DEFAULT_XSC_OPTIONS)", "");
		options = options.replace("$(COMMON_XSC_OPTIONS)", "");
		options = options.replace(/\s+/g, " ");
		var split = options.split("-");
		var c = split.length;
		for (var i = 0; i < c; i++) {
			var option = split[i].trim();
			if (option)
				tool.insertUnique(manifest.xsOptions, "-" + option);
		}
	},
	configure(tool, manifest, tree, buildType) {
		if (this.text) {
			var text = this.filter(this.text);
			this.concatCOptions(tool, manifest, tree, text, buildType);
			this.concatLibraries(tool, manifest, tree, text);
			this.concatObjects(tool, manifest, tree, text);
			this.concatXSCOptions(tool, manifest, tree, text);
		}
	},
	filter(text) {
		text = text.replace(/#.*\\[\n\r]?/gm, "");
		text = text.replace(/\s+\\\n\s+/gm, " ");
		text = text.replace(/\s+\\\r\s+/gm, " ");
		text = text.replace(/\s+\\\r\n\s+/gm, " ");
		text = text.replace(/\\\s+/gm, " ");
		return text;
	},
};
var xsDebug = {
	__proto__: xsCommon,
	configure(tool, manifest, tree) {
		if (tool.cmake)
			super.configure(tool, manifest, tree, "Debug")
		else
			if (tool.debug)
				super.configure(tool, manifest, tree)
	}	
};
var xsRelease = {
	__proto__: xsCommon,
	configure(tool, manifest, tree) {
		if (tool.cmake)
			super.configure(tool, manifest, tree, "Release")
		else
			if (!tool.debug)
				super.configure(tool, manifest, tree)
	}	
};
var xsInclude = {
	__proto__: xsMakefileItem,
};
var xsImportMakefile = {
	__proto__: xsMakefileItem,
	configure(tool, manifest, tree) {
		var path = tool.getPath(this.name);
		if (path) {
			if (this.verbose)
				this.report("Loading " + path + "...");
			var buffer = FS.readFileSync(path);
			var makefile = g.parse(buffer, path);
			makefile.configure(tool, manifest, tree);
		}
	},
};
var xsOutput = {
	__proto__: xsMakefileItem,
};
var xsVersion = {
	__proto__: xsConfiguration,
};
var xsWrap = {
	__proto__: xsSource,
};


var g = new Grammar(infoset);
g.object(xsMakefileItem, "", {
	name: g.string("@name")
});
g.object(xsMakefileList, "", {
	items: g.array(".", xsMakefileItem)
});
g.object(xsMakefile, "/makefile", {
});
g.object(xsInput, "input", {});
g.object(xsConfiguration, "configuration", {});
g.object(xsPlatform, "platform", {});
g.object(xsCOption, "c", {
	name: g.string("@option")
});
g.object(xsASMOption, "asm", {
	name: g.string("@option")
});
g.object(cmakeOptions, "cmake", {
	type: g.string("@type"),
	depends: g.string("@depends"),
	links: g.string("@links")
});
g.object(xsLibrary, "library", {});
g.object(xsSource, "source", {});
g.object(xsHeader, "header", {});
g.object(xsBuildStyle, "buildstyle", {});
g.object(xsCommon, "common", {
	text: g.string(".")
});
g.object(xsDebug, "debug", {});
g.object(xsRelease, "release", {});
g.object(xsInclude, "include", {});
g.object(xsImportMakefile, "import", {});
g.object(xsOutput, "output", {});
g.object(xsVersion, "version", {});
g.object(xsWrap, "wrap", {});
g.link();
export default g;
