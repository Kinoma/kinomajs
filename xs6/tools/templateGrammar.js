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
import infoset from "infoset";
import * as FS from "fs";

function countLines(text) @ "kprGrammar_countLines";

var _item = {
	isExpression(value) {
		return value.indexOf("$") >= 0;
	},
	prepare(out) {
	},
	print(out) {
	},
};
var _include = {
	__proto__: _item,
	path: "",
	prepare(out) {
		if (!this.path)
			out.reportError(this, "missing path attribute");
		var path = out.tool.getPath(this.path);
		if (path) {	
			this.program = out.tool.load(path);
			this.script = null;
			this.program.items.forEach(this.program.prepareItem, out); 	
		}
		else {
			path = out.tool.getScript(this.path);
			if (path) {
				this.program = null;
				this.script = path;
			}
			else {
				this.program = null;
				this.script = null;
				out.reportError(this, this.path + ": file not found");
			}
		}
	},
	print(out) {
		if (this.program)
			this.program.items.forEach(this.program.printItem, out);
		else if (this.script) {
			out.at(this.script, 1);
			out.code += "\n";
			out.code += FS.readFileSync(this.script);
			out.code += "\n";
			out.at(this.__xs__path, this.__xs__line);
			out.code += "\n";
		}
	},
};
var _require = {
	__proto__: _item,
	id: "",
	path: "",
	prepare(out) {
		if (!this.id)
			out.reportError(this, "missing id attribute");
	},
	print(out) {
		out.at(this.__xs__path, this.__xs__line);
		if (!this.path)
			this.path = this.id;
		if (this.id == "THEME")
			out.line("THEME = require('", this.path, "');");
		else if (this.id == "KEYBOARD")
			out.line("KEYBOARD = require('", this.path, "');");
		else
			out.line("var ", this.id, " = require('", this.path, "');");
	},
};
var _variable = {
	__proto__: _item,
	id: "",
	value: "undefined",
	prepare(out) {
		if (!this.id)
			out.reportError(this, "missing id attribute");
	},
	print(out) {
		out.at(this.__xs__path, this.__xs__line);
		out.printVar(this.id);
		out.cat(this.value, ";");
	},
};
var _null = {
	__proto__: _item,
	id: "",
	prepare(out) {
		if (!this.id)
			out.reportError(this, "missing id attribute");
	},
	print(out) {
		out.at(this.__xs__path, this.__xs__line);
		out.printVar(this.id);
		out.cat("null;");
	},
};
var _script = {
	__proto__: _item,
	cdata: "",
	print(out) {
		var c = countLines(this.cdata);
		out.at(this.__xs__path, this.__xs__line + c);
		out.line(this.cdata.trim());
	},
};
var _function = {
	__proto__: _item,
	id: "",
	params: "",
	cdata: "",
	prepare(out) {
		if (!this.id)
			out.reportError(this, "missing id attribute");
	},
	print(out) {
		var c = countLines(this.cdata);
		out.at(this.__xs__path, this.__xs__line + c - 1);
		if (this.id == "getBehavior")
			out.line("getBehavior = ");
		else if (this.id == "getPreferences")
			out.line("getPreferences = ");
		else
			out.printVar(this.id);
		out.cat("function(", this.params, ") {");
		out.tab(1);
		out.line(this.cdata.trim());
		out.tab(-1);
		out.line("}");
	},
};
var _constructor = {
	__proto__: _item,
	params: "",
	cdata: "",
};
var _member = {
	__proto__: _item,
	id: "",
	bind(member, out) {
		out.reportError(member, "invalid id");
	},
	prepare(out, i, members) {
		if (!this.id)
			out.reportError(this, "missing id attribute");
		var c = members.length;
		for (i++; i < c; i++) {
			var member = members[i];
			if (member.id == this.id)
				member.bind(this, out);
		} 
	},
};
var _field = {
	__proto__: _member,
	value: "undefined",
	print(out) {
		out.line(this.id, ": { value: ", this.value, ", writable: true },");
	},
	printProperty(out) {
		out.line(this.id, ": ", this.value, ",");
	},
};
var _method = {
	__proto__: _member,
	params: "",
	cdata: "",
	print(out) {
		out.line(this.id, ": { value: ");
		var c = countLines(this.cdata);
		out.at(this.__xs__path, this.__xs__line + c - 1);
		out.tab(1);
		out.line("function(", this.params, ") {");
		out.tab(1);
		out.line(this.cdata.trim());
		out.tab(-1);
		out.line("},");
		out.tab(-1);
		out.line("},");
	},
	printProperty(out) {
		var c = countLines(this.cdata);
		out.at(this.__xs__path, this.__xs__line + c - 1);
		out.line(this.id, ": function(", this.params, ") {");
		out.tab(1);
		out.line(this.cdata.trim());
		out.tab(-1);
		out.line("},");
	},
};
var _getter = {
	__proto__: _member,
	cdata: "",
	setter: null,
	bind(member, out) {
		if (_setter.isPrototypeOf(member) && (this.setter == null))
			this.setter = member;
		else
			out.reportError(this, "invalid id");
	},
	print(out) {
		out.line(this.id, ": {");
		var c = countLines(this.cdata);
		out.at(this.__xs__path, this.__xs__line + c - 1);
		out.tab(1);
		out.line("get: function() {");
		out.tab(1);
		out.line(this.cdata.trim());
		out.tab(-1);
		out.line("},");
		if (this.setter) {
			var c = countLines(this.setter.cdata);
			out.at(this.setter.__xs__line + c - 1);
			out.line("set: function(", this.setter.params, ") {");
			out.tab(1);
			out.line(this.setter.cdata.trim());
			out.tab(-1);
			out.line("},");
		}
		out.tab(-1);
		out.line("},");
	},
	printProperty(out) {
		var c = countLines(this.cdata);
		out.at(this.__xs__path, this.__xs__line + c - 1);
		out.line("get ", this.id, "() {");
		out.tab(1);
		out.line(this.cdata.trim());
		out.tab(-1);
		out.line("},");
	},
};
var _setter = {
	__proto__: _member,
	params: "",
	cdata: "",
	bind(member, out) {
		if (_getter.isPrototypeOf(member) && (member.setter == null))
			member.setter = this;
		else
			out.reportError(this, "invalid id");
	},
	printProperty(out) {
		var c = countLines(this.cdata);
		out.at(this.__xs__path, this.__xs__line + c - 1);
		out.line("set ", this.id, "(", this.params, ") {");
		out.tab(1);
		out.line(this.cdata.trim());
		out.tab(-1);
		out.line("},");
	},
};
var _class = {
	__proto__: _item,
	_constructor: _constructor,
	id: "",
	like: "Object",
	members: [],
	prepare(out) {
		if (!this.id)
			out.reportError(this, "missing id attribute");
		this.members.forEach(this.prepareMember, out); 	
	},
	prepareMember(member, i, members) {
		member.prepare(this, i, members); 	
	},
	print(out) {
		var constructor = this._constructor
		if (constructor == _constructor) {
			out.printVar(this.id);
			out.cat("function() {");
			out.tab(1);
			out.line(this.like, ".call(this);");
			out.tab(-1);
			out.line("};");
		}
		else {
			var c = countLines(constructor.cdata);
			out.at(constructor.__xs__line + c - 1);
			out.printVar(this.id);
			out.cat("function(", constructor.params, ") {");
			out.tab(1);
			out.line(constructor.cdata.trim());
			out.tab(-1);
			out.line("};");
		}
		out.line(this.id, ".prototype = Object.create(", this.like, ".prototype, {");
		out.tab(1);
		this.members.forEach(this.printMember, out); 	
		out.tab(-1);
		out.line("});");
	},
	printMember(member) {
		member.print(this); 	
	},
};
var _effectPart = {
};
var _colorizePart = {
	__proto__: _effectPart,
	color: "gray",
	opacity: 1,
	print(out) {
		out.cat("colorize('", this.color, "', ", this.opacity, ");");
	},
};
var _innerGlowPart = {
	__proto__: _effectPart,
	blur: 1,
	color: "white",
	opacity: 1,
	radius: 1,
	print(out) {
		out.cat("innerGlow('", this.color, "', ", this.opacity, ", ", this.blur, ", ", this.radius, ");");
	},
};
var _innerHilitePart = {
	__proto__: _effectPart,
	blur: 1,
	color: "white",
	opacity: 1,
	x: 0,
	y: 0,
	print(out) {
		out.cat("innerHilite('", this.color, "', ", this.opacity, ", ", this.blur, ", ", this.x, ", ", this.y, ");");
	},
};
var _innerShadowPart = {
	__proto__: _effectPart,
	blur: 1,
	color: "black",
	opacity: 1,
	x: 0,
	y: 0,
	print(out) {
		out.cat("innerShadow('", this.color, "', ", this.opacity, ", ", this.blur, ", ", this.x, ", ", this.y, ");");
	},
};
var _maskPart = {
	__proto__: _effectPart,
	texture: "",
	print(out) {
		out.cat("mask(", this.texture, ");");
	},
};
var _outerGlowPart = {
	__proto__: _effectPart,
	blur: 1,
	color: "white",
	opacity: 1,
	radius: 1,
	print(out) {
		out.cat("outerGlow('", this.color, "', ", this.opacity, ", ", this.blur, ", ", this.radius, ");");
	},
};
var _outerHilitePart = {
	__proto__: _effectPart,
	blur: 1,
	color: "white",
	opacity: 1,
	x: 0,
	y: 0,
	print(out) {
		out.cat("outerHilite('", this.color, "', ", this.opacity, ", ", this.blur, ", ", this.x, ", ", this.y, ");");
	},
};
var _outerShadowPart = {
	__proto__: _effectPart,
	blur: 1,
	color: "black",
	opacity: 1,
	x: 0,
	y: 0,
	print(out) {
		out.cat("outerShadow('", this.color, "', ", this.opacity, ", ", this.blur, ", ", this.x, ", ", this.y, ");");
	},
};
var _shadePart = {
	__proto__: _effectPart,
	opacity: 1,
	texture: "",
	print(out) {
		out.cat("shade('", this.texture, "', ", this.opacity, ");");
	},
};
var _effect = {
	__proto__: _item,
	id: "",
	parts: [],
	prepare(out) {
		if (!this.id)
			out.reportError(this, "missing id attribute");
	},
	print(out) {
		out.at(this.__xs__path, this.__xs__line);
		out.printVar(this.id);
		out.cat("new Effect();");
		var parts = this.parts;
		var c = parts.length;
		for (var i = 0; i < c; i++) {
			out.line(this.id, ".");
			parts[i].print(out);
		}
	},
};
var _texture = {
	__proto__: _item,
	id: "",
	small: "",
	medium: "",
	large: "",
	prepare(out) {
		if (!this.id)
			out.reportError(this, "missing id attribute");
	},
	print(out) {
		out.at(this.__xs__path, this.__xs__line);
		out.printVar(this.id);
		out.cat("(screenScale == 2) ? ");
		if (this.large)
			out.printTexture(this.large, 2);
		else if (this.medium)
			out.printTexture(this.medium, 1.5);
		else
			out.printTexture(this.small, 1);
		out.cat(" : (screenScale == 1.5) ? ");
		if (this.medium)
			out.printTexture(this.medium, 1.5);
		else if (this.large)
			out.printTexture(this.large, 2);
		else
			out.printTexture(this.small, 1);
		out.cat(" : ");
		if (this.small)
			out.printTexture(this.small, 1);
		else if (this.medium)
			out.printTexture(this.medium, 1.5);
		else
			out.printTexture(this.large, 2);
		out.cat(";");
	},
};
var _variants = {
	offset: 0,
	names: "",
};
var _states = {
	offset: 0,
	colors: "",
	names: "",
};
var _tiles = {
	left: 0,
	right: 0,
	top: 0,
	bottom: 0,
};
var _margins = {
	left: 0,
	right: 0,
	top: 0,
	bottom: 0,
};
var _borders = {
	left: 0,
	right: 0,
	top: 0,
	bottom: 0,
	color: "",
	states: _states,
};
var _skin = {
	__proto__: _item,
	id: "",
	texture: "",
	x: 0,
	y: 0,
	width: 0,
	height: 0,
	variants: _variants,
	states: _states,
	tiles: _tiles,
	margins: _margins,
	aspect: "draw",
	color: "",
	borders: _borders,
	prepare(out) {
		if (!this.id)
			out.reportError(this, "missing id attribute");
		this.prepareConstructor(out);
	},
	prepareConstructor(out) {
		if (this.texture) {
			out.parseConstants(this.variants.names);
			out.parseConstants(this.states.names);
		}
		else {
			out.parseConstants(this.states.names);
			out.parseConstants(this.borders.states.names);
		}
	},
	print(out) {
		out.at(this.__xs__path, this.__xs__line);
		out.printVar(this.id);
		this.printConstructor(out);
		out.cat(";");
	},
	printConstructor(out) {
		out.cat("new Skin({ ");
		if (this.texture) {
			out.cat("texture: ", this.texture, ", ");
			if (this.x)
				out.cat("x: ", this.x, ", ");
			if (this.y)
				out.cat("y: ", this.y, ", ");
			if (this.width)
				out.cat("width: ", this.width, ", ");
			if (this.height)
				out.cat("height: ", this.height, ", ");
			if (this.variants.offset)
				out.cat("variants: ", this.variants.offset, ", ");
			if (this.states.offset)
				out.cat("states: ", this.states.offset, ", ");
			if (this.tiles != _tiles) {
				out.cat("tiles: ");
				this.printCoordinates(out, this.tiles);
				out.cat(", ");
			}
			if (this.margins != _margins) {
				out.cat("margins: ");
				this.printCoordinates(out, this.margins);
				out.cat(", ");
			}
			if (this.aspect != "draw")
				out.cat("aspect: '", this.aspect, "', ");
		}
		else {
			if (this.borders != _borders) {
				out.cat("borders: ");
				this.printCoordinates(out, this.borders);
				out.cat(", ");
			}
			if (this.color)
				out.cat("fill: '", this.color, "',");
			else if (this.states.colors) {
				out.cat("fill: ")
				out.printColors(this.states.colors);
				out.cat(", ");
			}
			if (this.borders.color)
				out.cat("stroke: '", this.borders.color, "',");
			else if (this.borders.states.colors) {
				out.cat("stroke: ")
				out.printColors(this.borders.states.colors);
				out.cat(", ");
			}
		}
		out.cat("})");
	},
	printCoordinate(out, instance, property, flag) {
		if (instance.hasOwnProperty(property)) {
			if (flag) 
				out.cat(", ");
			out.cat(property, ":", instance[property]);
			return true;
		}
	},
	printCoordinates(out, instance) {
		var flag = false;
		out.cat("{ ");
		flag |= this.printCoordinate(out, instance, "left", flag);
		flag |= this.printCoordinate(out, instance, "right", flag);
		flag |= this.printCoordinate(out, instance, "top", flag);
		flag |= this.printCoordinate(out, instance, "bottom", flag);
		out.cat(" }");
	},
};
var _style = {
	__proto__: _item,
	id: "",
	font: "",
	color: "",
	states: _states,
	align: "",
	horizontal: "null",
	indentation: 0,
	leading: 0,
	lines: 0,
	margins: _margins,
	vertical: "null",
	prepare(out) {
		if (!this.id)
			out.reportError(this, "missing id attribute");
		this.prepareConstructor(out);
	},
	prepareConstructor(out) {
		out.parseConstants(this.states.names);
		if (this.align) {
			var a = this.align.split(",");
			var c = a.length;
			for (var i = 0; i < c; i++) {
				var s = a[i];
				switch (s) {
				case "left":
				case "center":
				case "right":
				case "justify":
					this.horizontal = s;
					break;
				case "top":
				case "middle":
				case "bottom":
					this.vertical = s;
					break;
				}
			}
		}
	},
	print(out) {
		out.at(this.__xs__path, this.__xs__line);
		out.printVar(this.id);
		this.printConstructor(out);
		out.cat(";");
	},
	printConstructor(out) {
		out.cat("new Style({ ");
		if (this.color)
			out.cat("color: '", this.color, "',");
		else if (this.states.colors) {
			out.cat("color: ")
			out.printColors(this.states.colors);
			out.cat(", ");
		}
		if (this.font)
			out.cat(" font: '", this.font, "', ");
		if (this.horizontal)
			out.cat("horizontal: '", this.horizontal, "', ");
		if (this.indentation)
			out.cat("indentation: ", this.indentation, ", ");
		if (this.vertical)
			out.cat("vertical: '", this.vertical, "', ");
		if (this.leading)
			out.cat("leading: ", this.leading, ", ");
		if (this.margins.left)
			out.cat("left: ", this.margins.left, ", ");
		if (this.margins.right)
			out.cat("right: ", this.margins.right, ", ");
		if (this.margins.top)
			out.cat("top: ", this.margins.top, ", ");
		if (this.margins.bottom)
			out.cat("bottom: ", this.margins.bottom, ", ");
		if (this.lines)
			out.cat("lines: ", this.lines, ", ");
		out.cat("})");
	},
};
var _behavior = {
	__proto__: _item,
	id: "",
	like: "Behavior",
	members: [],
	script: "",
	top: /^(\s+)function\s+([^\(]+)/,
	middle: /}(\s+)function\s+([^\(]+)/g,
	bottom: /}\s+$/,
	embed: false,
	prepare(out) {
		if (!this.embed && !this.id)
			out.reportError(this, "missing id attribute");
		if (this.id == this.like)
			out.reportError(this, "invalid id attribute");
		this.members.forEach(this.prepareMember, out); 	
	},
	prepareMember(member, i, members) {
		member.prepare(this, i, members); 	
	},
	print(out) {
		/*
		if (this.embed)
			out.line(this.id, " = ");
		else
			out.printVar(this.id);
		out.cat("function(content, data, dictionary) {");
		out.tab(1);
		out.line(this.like, ".call(this, content, data, dictionary);");
		out.tab(-1);
		out.line("}");
		out.line(this.id, ".prototype = ");
		this.printObject(out);
		out.cat(";");
		*/
		if (this.embed)
			out.line(this.id, " = ");
		else
			out.printVar(this.id);
		out.cat(this.like, ".template({");
		out.tab(1);
		if (this.members.length)
			this.members.forEach(this.printMemberProperty, out); 	
		else
			this.printScriptProperty(out);
		out.tab(-1);
		out.line("})");
	},
	printMember(member) {
		member.print(this); 	
	},
	printMemberProperty(member) {
		member.printProperty(this); 	
	},
	printObject(out) {
		out.cat("Object.create(", this.like, ".prototype, {");
		out.tab(1);
		if (this.members.length)
			this.members.forEach(this.printMember, out); 	
		else
			this.printScript(out);
		out.tab(-1);
		out.line("})");
	},
	printScript(out) {
		var c = countLines(this.script);
		out.at(this.__xs__path, this.__xs__line + c);
		var text = this.script;
		text = text.replace(this.top, "$2: { value: function");
		text = text.replace(this.middle, "}},$1$2: { value: function");
		text = text.replace(this.bottom, "}}");
		out.line(text);
	},
	printScriptProperty(out) {
		var c = countLines(this.script);
		out.at(this.__xs__path, this.__xs__line + c);
		var text = this.script;
		text = text.replace(this.top, "$2: function");
		text = text.replace(this.middle, "},$1$2: function");
		text = text.replace(this.bottom, "}");
		out.line(text);
	},
};
var _transition = {
	__proto__: _behavior,
	duration: 0,
	like: "Transition",
	print(out) {
		out.printVar(this.id);
		out.cat("function(duration) {");
		out.tab(1);
		out.line("if (!duration) duration = ", this.duration, ";");
		out.line(this.like, ".call(this, duration);");
		out.tab(-1);
		out.line("}");
		out.line(this.id, ".prototype = ");
		this.printObject(out);
		out.cat(";");
	},
};
var _handler = {
	__proto__: _item,
	path: "",
	behaviorID: "",
	behavior: _behavior,
	prepare(out) {
		if (this.behavior != _behavior) {
			if (this.behaviorID)
				out.reportError(this, "a handler element cannot have a behavior attribute and a behavior element");
			else if (this.behavior.id)
				out.reportError(this.behavior, "nested behavior element cannot have an id attribute");
			else {
				this.behavior.embed = true;
				this.behavior.prepare(out);
			}
		}
	},
	print(out) {
		out.line("Handler.bind(\"", this.path, "\", ");
		if (this.behaviorID) {
			out.cat("Object.create(", this.behaviorID, ".prototype)");
		}
		else {
			this.behavior.printObject(out);
		}
		out.cat(");");
	},
};
var _any = {
	__proto__: _item,
	prepareContent(out) {
	},
	printContent(out) {
	},
	
};
var _insert = {
	__proto__: _any,
	printContent(out) {
		out.line("builder.call(", out.container, ";");
	},
};
var _iterate = {
	__proto__: _any,
	contents: [],
	on: "",
	prepareContent(out) {
		this.contents.forEach(out.prepareContent, out); 	
	},
	printContent(out) {
		out.at(this.__xs__path, this.__xs__line);
		out.line("(", this.on, ") ? ", this.on, ".map(function($) { var $$ = this; return [");
		out.tab(1);
		this.contents.forEach(out.printContent, out); 	
		out.tab(-1);
		out.line("]}, $) : null, ");
	},
};
var _scope = {
	__proto__: _any,
	contents: [],
	_with: "",
	prepareContent(out) {
		this.contents.forEach(out.prepareContent, out); 	
	},
	printContent(out) {
		out.at(this.__xs__path, this.__xs__line);
		out.line("(function($, $$) { return [");
		out.tab(1);
		this.contents.forEach(out.printContent, out); 	
		out.tab(-1);
		out.line("]})(", this._with, ", $), ");
	},
};
var _select = {
	__proto__: _any,
	contents: [],
	on: "",
	prepareContent(out) {
		this.contents.forEach(out.prepareContent, out); 	
	},
	printContent(out) {
		out.at(this.__xs__path, this.__xs__line);
		out.line("(", this.on, ") ? [");
		out.tab(1);
		this.contents.forEach(out.printContent, out); 	
		out.tab(-1);
		out.line("] : null, ");
	},
};
var _content = {
	__proto__: _any,
	_constructor: "Content",
	id: "",
	like: "",
	_with: "$",
	keys: "",
	anchor: "",
	name: "",
	behaviorID: "",
	behavior: _behavior,
	behaviors: null,
	horizontal: "",
	left: "",
	width: "",
	right: "",
	vertical: "",
	top: "",
	height: "",
	bottom: "",
	skinID: "",
	skin: _skin,
	styleID: "",
	style: _style,
	active: "",
	backgroundTouch: "",
	exclusiveTouch: "",
	mutipleTouch: "",
	visible: "",
	state: "",
	variant: "",
	prepare(out) {
		if (!this.id)
			out.reportError(this, "missing id attribute");
		out.current = this;
		this.behaviors = [];
		this.prepareContent(out); 	
	},
	prepareContent(out) {
		if (this.id && !this.behaviors)
			out.reportError(this, "nested content element cannot have an id attribute");
		out.prepareBehavior(this);
		if (this.skin != _skin) {
			if (this.skinID)
				out.reportError(this, "a content element cannot have a skin attribute and a skin element");
			else if (this.skin.id)
				out.reportError(this.skin, "nested skin element cannot have an id attribute");
			else
				this.skin.prepareConstructor(out);
		}
		if (this.style != _style) {
			if (this.styleID) 
				out.reportError(this, "a content element cannot have a style attribute and a style element");
			else if (this.style.id)
				out.reportError(this.style, "nested style element cannot have an id attribute");
			else
				this.style.prepareConstructor(out);
		}
	},
	print(out) {
		out.current = this;
		out.at(this.__xs__path, this.__xs__line);
		out.printVar(this.id);
		if (this.like)
			out.cat(this.like);
		else
			out.cat(this._constructor);
		out.cat(".template(function($) { return { ");
		this.printProperties(out);
		out.cat("}});");
		if (this.behaviors.length) {
			out.line(this.id, ".behaviors = new Array(", this.behaviors.length, ");");
			this.behaviors.forEach(out.printItem, out);
		}
	},
	printConstructor(out) {
		if (this.like)
			out.line(this.like);
		else
			out.line(this._constructor);
		out.cat("(", this._with, ", { ");
		this.printProperties(out);
		out.cat("})");
	},
	printContent(out) {
		out.at(this.__xs__path, this.__xs__line);
		this.printConstructor(out);
		out.cat(",");
	},
	printProperties(out) {
		this.printProperty(out, "left");
		this.printProperty(out, "width");
		this.printProperty(out, "right");
		this.printProperty(out, "top");
		this.printProperty(out, "height");
		this.printProperty(out, "bottom");
		this.printProperty(out, "active");
		this.printProperty(out, "backgroundTouch");
		this.printProperty(out, "exclusiveTouch");
		this.printProperty(out, "mutipleTouch");
		this.printProperty(out, "visible");
		
		if (this.skinID)
			out.cat("skin: ", this.skinID, ", ");
		else if (this.skin != _skin) {
			out.cat("skin: ");
			this.skin.printConstructor(out);
			out.cat(", ");
		}
		if (this.styleID)
			out.cat("style: ", this.styleID, ", ");
		else if (this.style != _style) {
			out.line("style: ");
			this.style.printConstructor(out);
			out.cat(", ");
		}
		this.printProperty(out, "state");
		this.printProperty(out, "variant");
		this.printProperty(out, "keys");
		
		if (this.hasOwnProperty("anchor"))
			out.cat("anchor: '", this.anchor, "', ");
		if (this.hasOwnProperty("name")) {
			if (this.isExpression(this.name))
				out.cat("name: ", this.name, ", ");
			else
				out.cat("name: '", this.name, "', ");
		}
		
		if (this.behaviorID)
			out.cat("behavior: Object.create((", this.behaviorID, ").prototype), ");
	},
	printProperty(out, property) {
		if (this.hasOwnProperty(property)) {
			out.cat(property, ": ", this[property], ", ");
		}
	},
};
var _container = {
	__proto__: _content,
	_constructor: "Container",
	contents: [],
	clip: "",
	prepareContent(out) {
		_content.prepareContent.call(this, out);
		this.contents.forEach(this.prepareContentAux, out); 	
	},
	prepareContentAux(content, i) {
		content.prepareContent(this);
	},
	printProperties(out) {
		_content.printProperties.call(this, out);
		this.printProperty(out, "clip");
		if (this.contents.length) {
			out.cat("contents: [");
			out.tab(1);
			this.contents.forEach(out.printContent, out); 	
			out.tab(-1);
			out.line("], ");
		}
	},
};
var _layout = {
	__proto__: _container,
	_constructor: "Layout",
};
var _port = {
	__proto__: _content,
	_constructor: "Port",
};
var _canvas = {
	__proto__: _content,
	_constructor: "Canvas",
};
var _scroller = {
	__proto__: _container,
	_constructor: "Scroller",
	loop: "",
	printProperties(out) {
		_container.printProperties.call(this, out);
		if (this.loop)
			out.cat("loop: ", this.loop, ", ");
	},
};
var _column = {
	__proto__: _container,
	_constructor: "Column",
};
var _line = {
	__proto__: _container,
	_constructor: "Line",
};
var _label = {
	__proto__: _content,
	_constructor: "Label",
	editable: "",
	hidden: "",
	selectable: "",
	string: "",
	printProperties(out) {
		_content.printProperties.call(this, out);
		this.printProperty(out, "editable");
		this.printProperty(out, "hidden");
		this.printProperty(out, "selectable");
		if (this.string)
			out.cat("string: ", this.string, ", ");
	},
};
var _span = {
	behaviorID: "",
	behavior: _behavior,
	string: "",
	style: "",
	_with: "$",
	prepareContent(out) {
		out.prepareBehavior(this);
	},
	print(out) {
		out.at(this.__xs__path, this.__xs__line);
		out.line("{ ");
		if (this.behaviorID) {
			out.cat("behavior: Object.create((", this.behaviorID, ").prototype), ");
			out.cat("$: ", this._with, ", ");
		}
		if (this.style)
			out.cat("style: ", this.style, ", ");
		if (this.string)
			out.cat("string: ", this.string, ", ");
		out.cat("},");
	},
};
var _wrap = {
	alignment: "middle",
	content: _content,
	prepareContent(out) {
		this.content.prepareContent(out),
	},
	print(out) {
		out.at(this.__xs__path, this.__xs__line);
		out.line("{ align: '", this.alignment, "', content: ");
		this.content.printConstructor(out);
		out.cat("},");
	},
};
var _block = {
	behaviorID: "",
	behavior: _behavior,
	spans: [],
	string: "",
	style: "",
	_with: "$",
	prepareSpan(span) {
		span.prepareContent(this);
	},
	prepareContent(out) {
		out.prepareBehavior(this);
		this.spans.forEach(this.prepareSpan, out); 	
	},
	print(out) {
		out.at(this.__xs__path, this.__xs__line);
		out.line("{ ");
		if (this.behaviorID) {
			out.cat("behavior: Object.create((", this.behaviorID, ").prototype), ");
			out.cat("$: ", this._with, ", ");
		}
		if (this.style)
			out.cat("style: ", this.style, ", ");
		if (this.spans.length) {
			out.cat("spans: [");
			out.tab(1);
			this.spans.forEach(this.printSpan, out); 	
			out.tab(-1);
			out.line("], ");
		}
		else if (this.string)
			out.cat("string: ", this.string, ", ");
		out.cat("},");
	},
	printSpan(span) {
		span.print(this);
	},
};
var _text = {
	__proto__: _content,
	_constructor: "Text",
	blocks: [],
	editable: "",
	selectable: "",
	string: "",
	prepareBlock(block) {
		block.prepareContent(this);
	},
	prepareContent(out) {
		_content.prepareContent.call(this, out);
		this.blocks.forEach(this.prepareBlock, out); 	
	},
	printBlock(block) {
		block.print(this);
	},
	printProperties(out) {
		_content.printProperties.call(this, out);
		this.printProperty(out, "editable");
		this.printProperty(out, "selectable");
		if (this.blocks.length) {
			out.line("blocks: [");
			out.tab(1);
			this.blocks.forEach(this.printBlock, out); 	
			out.tab(-1);
			out.line("], ");
		}
		else if (this.string)
			out.cat("string: ", this.string, ", ");
	},
};
var resource = {
	__proto__: _content,
	url: "null",
	aspect: "",
	mime: "null",
	printProperties(out) {
		_content.printProperties.call(this, out);
		if (this.aspect) {
			switch (this.aspect) {
			case "draw":
			case "fill":
			case "fit":
			case "stretch":
				out.cat("aspect: '", this.aspect, "', ");
				break;	
			default:
				out.cat("aspect: ", this.aspect, ", ");
				break;	
			}
		}
		if (this.hasOwnProperty("url")) {
			if (this.hasOwnProperty("mime"))
				out.cat("mime: ", this.mime, ", ");
			out.printURL(this.url);
		}
	},
};
var _picture = {
	__proto__: resource,
	_constructor: "Picture",
};
var _thumbnail = {
	__proto__: resource,
	_constructor: "Thumbnail",
};
var _media = {
	__proto__: resource,
	_constructor: "Media",
};
var _browser = {
	__proto__: _content,
	_constructor: "Browser",
	url: "null",
	prepare(out) {
		out.reportError(this, "the browser element must be nested");
		_content.prototype.prepare.call(this, out);
	},
	printConstructor(out) {
		out.line("(function() {");
		out.tab(1);
		out.line("var browser = ");
		if (this.anchor)
			out.cat("$.", this.anchor, " = ");
		out.cat("new Browser({ ");
		this.printProperty(out, "left");
		this.printProperty(out, "width");
		this.printProperty(out, "right");
		this.printProperty(out, "top");
		this.printProperty(out, "height");
		this.printProperty(out, "bottom");
		out.cat(" }");
		if (this.url)
			out.cat(", ", this.url);
		out.cat(");");
		if (this.behaviorID)
			out.line("browser.behavior = new (", this.behaviorID, ")(browser, $);");
		out.line("return browser;");
		out.tab(-1);
		out.line("})()");
	},
};
var _program = {
	currentPath: null,
	id: "",
	items: [],
	code: null,
	constants: null,
	contentDepth: 0,
	_private: false,
	scopeDepth: 0,
	tabDepth: 0,
	origin: 0,
	at(path, line) {
		if (this.currentPath != path) {
			this.currentPath = path;
			this.cat("\n//@line ", line, " \"", path, "\"");
		}
		else {
			this.cat("\n//@line ", line);
		}
	},
	cat() {
		var c = arguments.length;
		for (var i = 0; i < c; i++)
			this.code += arguments[i];
	},
	line() {
		var c;
		this.origin++;
		this.code += "\n";
		c = this.tabDepth;
		while (c > 0) {
			this.code += "\t";
			c--;
		}
		c = arguments.length;
		for (var i = 0; i < c; i++)
			this.code += arguments[i];
	},
	generate(tool) {
		this.tool = tool;
		this.constants = {};
		this.items.forEach(this.prepareItem, this); 	
		this.code = "";
		this.printKind();
		this.origin = this.__xs__line;
		this.line("/* KPR2JS GENERATED FILE; DO NOT EDIT! */");
		this.printConstants(this);
		this.items.forEach(this.printItem, this); 	
		return this.code;
	},
	nest(delta) {
		this.contentDepth += delta;
	},
	parseConstant(name, value) {
		this.constants[name] = value;
	},
	parseConstants(names) {
		if (names) {	
			names = names.split(",");
			var c = names.length;
			for (var i = 0; i < c; i++)
				this.parseConstant(names[i], i);
		}
	},
	prepare(tool) {
		this.tool = tool;
		this.constants = {};
		this.items.forEach(this.prepareItem, this); 	
	},
	prepareBehavior(item) {
		if (item.behavior != _behavior) {
			if (item.behaviorID)
				this.reportError(item, "an element cannot have a behavior attribute and a behavior element");
			else if (item.behavior.id)
				this.reportError(item.behavior, "nested behavior element cannot have an id attribute");
			else {
				// do not nest behavior objects to avoid closures
				var template = this.current;
				var behaviors = template.behaviors;
				item.behaviorID = item.behavior.id = template.id + ".behaviors[" + behaviors.length + "]";
				item.behavior.embed = true;
				item.behavior.prepare(this);
				behaviors.push(item.behavior);
			}
		}
	},
	prepareContent(content) {
		content.prepareContent(this);
	},
	prepareItem(item, i) {
		item.prepare(this);
	},
	print(tool) {
		this.code = "";
		this.printKind();
		this.origin = this.__xs__line;
		this.line("/* KPR2JS GENERATED FILE; DO NOT EDIT! */");
		this.printConstants(this);
		this.items.forEach(this.printItem, this); 	
		return this.code;
	},
	printColors(colors) {
		var states = colors.split(",");
		var c = states.length;
		this.cat("[");
		for (var i = 0; i < c; i++) {
			if (i) this.cat(", ");
			this.cat("'", states[i], "'");
		}
		this.cat("]");
	},
	printConstants(instance) {
		var instance = this.constants;
		for (var i in instance) {
			this.printVar(i);
			this.cat(instance[i], ";");
		}
	},
	printContent(content) {
		content.printContent(this);
	},
	printItem(item) {
		item.print(this);
	},
	printTexture(id, scale) {
		this.cat("new Texture('", id, "', ", scale, ")");
	},
	printKind() {
		this.cat("//@program\n");
	},
	printURL(url) {
		this.cat("url: ", url, ", ");
	},
	printVar(id) {
		this.line("var ", id, " = ");
	},
	serializeCoordinate(instance, property, text, quote) {
		if (instance.hasOwnProperty(property)) {
			if (text) 
				text += ", ";
			text += property + ":" 
			if (quote) text += "'";
			text += instance[property];
			if (quote) text += "'";
		}
		return text;
	},
	serializeCoordinates(instance) {
		var text = "";
		text = this.serializeCoordinate(instance, "horizontal", text, true);
		text = this.serializeCoordinate(instance, "left", text);
		text = this.serializeCoordinate(instance, "width", text);
		text = this.serializeCoordinate(instance, "right", text);
		text = this.serializeCoordinate(instance, "vertical", text, true);
		text = this.serializeCoordinate(instance, "top", text);
		text = this.serializeCoordinate(instance, "height", text);
		text = this.serializeCoordinate(instance, "bottom", text);
		return "{" + text + "}";
	},
	tab(delta) {
		this.tabDepth += delta;
	},
	get temporary() {
		var depth = this.contentDepth;
		return "_" + depth;
	},
	get content() {
		var depth = this.contentDepth;
		if (depth)
			return "$" + depth;
		return "this";
	},
	get container() {
		var depth = this.contentDepth;
		if (depth > 1)
			return "$" + (depth - 1);
		return "this";
	},
	get local() {
		var result = "$";
		var c = this.scopeDepth;
		while (c) {
			result += "$";
			c--;
		}
		return result;
	},
	reportError(instance, message) {
		if (instance)
			this.tool.reportError(instance.__xs__path, instance.__xs__line, message);
		else
			this.tool.reportError(null, 0, message);
	},
	scope(path) {
		this.scopeDepth++;
		var c = this.scopeDepth;
		this.line("var ");
		while (c) {
			this.cat("$");
			for (var i = 0; i < c; i++)
				this.cat("$");
			this.cat(" = $");
			c--;
			for (var i = 0; i < c; i++)
				this.cat("$");
			this.cat("; ");
		}
		if (path)
			this.cat("$ = ", path, ";");
	},
	unscope() {
		var c = 0;
		var d = this.scopeDepth;
		this.line();
		while (c < d) {
			this.cat("$");
			for (var i = 0; i < c; i++)
				this.cat("$");
			this.cat(" = $");
			c++;
			for (var i = 0; i < c; i++)
				this.cat("$");
			this.cat("; ");
		}
		this.scopeDepth--;
	},
};
var _private = {
	items: [],
	prepare(out) {
		this.items.forEach(this.prepareItem, out); 	
	},
	prepareItem(item, i) {
		item.prepare(this);
	},
	print(out) {
		out._private = true;
		this.items.forEach(this.printItem, out); 	
		out._private = false;
	},
	printItem(item, i) {
		item.print(this);
	},
};
var _module = {
	__proto__: _program,
	items: [],
		printKind() {
		this.cat("//@module\n");
	},
	printURL(url) {
		this.cat("url: mergeURI(module.uri, ", url, "), ");
	},
	printVar(id) {
		if (this._private)
			this.line("var ", id, " = ");
		else
			this.line("var ", id, " = exports.", id, " = ");
	},
};
var _shell = {
	cdata: "",
	modules: "modules/",
	width: 640,
	height: 480,
	generate(tool) {
		return "//@program\n" + this.cdata;
	},
	prepare(tool) {
	},
	print(tool) {
		return "//@program\n" + this.cdata;
	},
};

var g = new Grammar(infoset);
g.namespace("http://www.kinoma.com/kpr/1", "xs0");
g.object(_include, "xs0:include", {
	path: g.string("@path")
});
g.object(_require, "xs0:require", {
	id: g.string("@id"),
	path: g.string("@path")
});
g.object(_variable, "xs0:variable", {
	id: g.string("@id"),
	value: g.string("@value")
});
g.object(_null, "xs0:null", {
	id: g.string("@id")
});
g.object(_script, "xs0:script", {
	cdata: g.string(".")
});
g.object(_function, "xs0:function", {
	id: g.string("@id"),
	params: g.string("@params"),
	cdata: g.string(".")
});
g.object(_constructor, "xs0:constructor", {
	params: g.string("@params"),
	cdata: g.string(".")
});
g.object(_member, "", {
	id: g.string("@id")
});
g.object(_field, "xs0:field", {
	value: g.string("@value")
});
g.object(_method, "xs0:method", {
	params: g.string("@params"),
	cdata: g.string(".")
});
g.object(_getter, "xs0:getter", {
	cdata: g.string(".")
});
g.object(_setter, "xs0:setter", {
	params: g.string("@params"),
	cdata: g.string(".")
});
g.object(_class, "xs0:class", {
	_constructor: g.reference(".", _constructor),
	id: g.string("@id"),
	like: g.string("@like"),
	members: g.array(".", _member)
});
g.object(_effectPart, "", {});
g.object(_colorizePart, "xs0:colorize", {
	color: g.string("@color"),
	opacity: g.number("@opacity")
});
g.object(_innerGlowPart, "xs0:inner-glow", {
	blur: g.number("@blur"),
	color: g.string("@color"),
	opacity: g.number("@opacity"),
	radius: g.number("@radius")
});
g.object(_innerHilitePart, "xs0:inner-hilite", {
	blur: g.number("@blur"),
	color: g.string("@color"),
	opacity: g.number("@opacity"),
	x: g.number("@x"),
	y: g.number("@y")
});
g.object(_innerShadowPart, "xs0:inner-shadow", {
	blur: g.number("@blur"),
	color: g.string("@color"),
	opacity: g.number("@opacity"),
	x: g.number("@x"),
	y: g.number("@y")
});
g.object(_maskPart, "xs0:mask", {
	texture: g.string("@texture")
});
g.object(_outerGlowPart, "xs0:outer-glow", {
	blur: g.number("@blur"),
	color: g.string("@color"),
	opacity: g.number("@opacity"),
	radius: g.number("@radius")
});
g.object(_outerHilitePart, "xs0:outer-hilite", {
	blur: g.number("@blur"),
	color: g.string("@color"),
	opacity: g.number("@opacity"),
	x: g.number("@x"),
	y: g.number("@y")
});
g.object(_outerShadowPart, "xs0:outer-shadow", {
	blur: g.number("@blur"),
	color: g.string("@color"),
	opacity: g.number("@opacity"),
	x: g.number("@x"),
	y: g.number("@y")
});
g.object(_shadePart, "xs0:shade", {
	opacity: g.number("@opacity"),
	texture: g.string("@texture")
});
g.object(_effect, "xs0:effect", {
	id: g.string("@id"),
	parts: g.array(".", _effectPart)
});
g.object(_texture, "xs0:texture", {
	id: g.string("@id"),
	small: g.string("@small"),
	medium: g.string("@medium"),
	large: g.string("@large")
});
g.object(_variants, "xs0:variants", {
	offset: g.number("@offset"),
	names: g.string("@names")
});
g.object(_states, "xs0:states", {
	offset: g.number("@offset"),
	colors: g.string("@colors"),
	names: g.string("@names")
});
g.object(_tiles, "xs0:tiles", {
	left: g.number("@left"),
	right: g.number("@right"),
	top: g.number("@top"),
	bottom: g.number("@bottom")
});
g.object(_margins, "xs0:margins", {
	left: g.number("@left"),
	right: g.number("@right"),
	top: g.number("@top"),
	bottom: g.number("@bottom")
});
g.object(_borders, "xs0:borders", {
	left: g.number("@left"),
	right: g.number("@right"),
	top: g.number("@top"),
	bottom: g.number("@bottom"),
	color: g.string("@color"),
	states: g.reference(".", _states)
});
g.object(_skin, "xs0:skin", {
	id: g.string("@id"),
	texture: g.string("@texture"),
	x: g.number("@x"),
	y: g.number("@y"),
	width: g.number("@width"),
	height: g.number("@height"),
	variants: g.reference(".", _variants),
	states: g.reference(".", _states),
	tiles: g.reference(".", _tiles),
	margins: g.reference(".", _margins),
	aspect: g.string("@aspect"),
	color: g.string("@color"),
	borders: g.reference(".", _borders)
});
g.object(_style, "xs0:style", {
	id: g.string("@id"),
	font: g.string("@font"),
	color: g.string("@color"),
	states: g.reference(".", _states),
	align: g.string("@align"),
	indentation: g.number("@indentation"),
	leading: g.number("@leading"),
	lines: g.number("@lines"),
	margins: g.reference(".", _margins)
});
g.object(_behavior, "xs0:behavior", {
	id: g.string("@id"),
	like: g.string("@like"),
	members: g.array(".", _member),
	script: g.string(".")
});
g.object(_transition, "xs0:transition", {
	duration: g.number("@duration"),
	like: g.string("@like")
});
g.object(_handler, "xs0:handler", {
	path: g.string("@path"),
	behaviorID: g.string("@behavior"),
	behavior: g.reference(".", _behavior)
});
g.object(_any, "", {});
g.object(_insert, "xs0:insert", {});
g.object(_iterate, "xs0:iterate", {
	contents: g.array(".", _any),
	on: g.string("@on")
});
g.object(_scope, "xs0:scope", {
	contents: g.array(".", _any),
	_with: g.string("@with")
});
g.object(_select, "xs0:select", {
	contents: g.array(".", _any),
	on: g.string("@on")
});
g.object(_content, "xs0:content", {
	id: g.string("@id"),
	like: g.string("@like"),
	_with: g.string("@with"),
	keys: g.string("@keys"),
	anchor: g.string("@anchor"),
	name: g.string("@name"),
	behaviorID: g.string("@behavior"),
	behavior: g.reference(".", _behavior),
	horizontal: g.string("@horizontal"),
	left: g.string("@left"),
	width: g.string("@width"),
	right: g.string("@right"),
	vertical: g.string("@vertical"),
	top: g.string("@top"),
	height: g.string("@height"),
	bottom: g.string("@bottom"),
	skinID: g.string("@skin"),
	skin: g.reference(".", _skin),
	styleID: g.string("@style"),
	style: g.reference(".", _style),
	active: g.string("@active"),
	backgroundTouch: g.string("@backgroundTouch"),
	exclusiveTouch: g.string("@exclusiveTouch"),
	mutipleTouch: g.string("@mutipleTouch"),
	visible: g.string("@visible"),
	state: g.string("@state"),
	variant: g.string("@variant")
});
g.object(_container, "xs0:container", {
	contents: g.array(".", _any),
	clip: g.string("@clip")
});
g.object(_layout, "xs0:layout", {});
g.object(_port, "xs0:port", {});
g.object(_canvas, "xs0:canvas", {});
g.object(_scroller, "xs0:scroller", {
	loop: g.string("@loop")
});
g.object(_column, "xs0:column", {});
g.object(_line, "xs0:line", {});
g.object(_label, "xs0:label", {
	editable: g.string("@editable"),
	hidden: g.string("@hidden"),
	selectable: g.string("@selectable"),
	string: g.string("@string")
});
g.object(_span, "xs0:span", {
	behaviorID: g.string("@behavior"),
	behavior: g.reference(".", _behavior),
	string: g.string("@string"),
	style: g.string("@style"),
	_with: g.string("@with")
});
g.object(_wrap, "xs0:wrap", {
	alignment: g.string("@alignment"),
	content: g.reference(".", _content)
});
g.object(_block, "xs0:block", {
	behaviorID: g.string("@behavior"),
	behavior: g.reference(".", _behavior),
	spans: g.array(".", _span, _wrap),
	string: g.string("@string"),
	style: g.string("@style"),
	_with: g.string("@with")
});
g.object(_text, "xs0:text", {
	blocks: g.array(".", _block),
	editable: g.string("@editable"),
	selectable: g.string("@selectable"),
	string: g.string("@string")
});
g.object(_picture, "xs0:picture", {
	url: g.string("@url"),
	mime: g.string("@mime"),
	aspect: g.string("@aspect")
});
g.object(_thumbnail, "xs0:thumbnail", {
	url: g.string("@url"),
	mime: g.string("@mime"),
	aspect: g.string("@aspect")
});
g.object(_media, "xs0:media", {
	url: g.string("@url"),
	mime: g.string("@mime"),
	aspect: g.string("@aspect")
});
g.object(_browser, "xs0:browser", {
	url: g.string("@url")
});
g.object(_program, "/xs0:program", {
	id: g.string("@id"),
	items: g.array(".", _include, _require, _script, _function, _class, _null, _variable, _effect, _texture, _skin, _style, _behavior, _handler, _content)
});
g.object(_private, "xs0:private", {
	items: g.array(".", _function, _class, _null, _variable, _effect, _texture, _skin, _style, _behavior, _handler, _content)
});
g.object(_module, "/xs0:module", {
	items: g.array(".", _include, _require, _script, _private, _function, _class, _null, _variable, _effect, _texture, _skin, _style, _behavior, _handler, _content)
});
g.object(_shell, "/xs0:shell", {
	cdata: g.string("."),
	modules: g.string("@modules"),
	width: g.number("@width"),
	height: g.number("@height")
});
g.link();
export default g;
