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
	<namespace uri="http://www.kinoma.com/kpr/1"/>
	<object name="kprGrammar">
		<function name="countLines" params="text" c="kprGrammar_countLines"/>

		<object name="item">
			<function name="isExpression" params="value">
				return value.indexOf("$") >= 0;
			</function>
			<function name="prepare" params="out">
			</function>
			<function name="print" params="out">
			</function>
		</object>
	
		<object name="include" prototype="kprGrammar.item" pattern="include">
			<string name="path" pattern="@path"/>
			<function name="prepare" params="out">
				if (!this.path)
					out.reportError(this, "missing path attribute");
			</function>
			<function name="print" params="out">
				out.at(this.__xs__line);
				out.line("include('", this.path, "');");
			</function>
		</object>
		
		<object name="require" prototype="kprGrammar.item" pattern="require">
			<string name="id" pattern="@id"/>
			<string name="path" pattern="@path"/>
			<function name="prepare" params="out">
				if (!this.id)
					out.reportError(this, "missing id attribute");
			</function>
			<function name="print" params="out">
				out.at(this.__xs__line);
				if (!this.path)
					this.path = this.id;
				if (this.id == "THEME")
					out.line("THEME = require('", this.path, "');");
				else if (this.id == "KEYBOARD")
					out.line("KEYBOARD = require('", this.path, "');");
				else
					out.line("var ", this.id, " = require('", this.path, "');");
			</function>
		</object>
		
		<object name="variable" prototype="kprGrammar.item" pattern="variable">
			<string name="id" pattern="@id"/>
			<string name="value" value="undefined" pattern="@value"/>
			<function name="prepare" params="out">
				if (!this.id)
					out.reportError(this, "missing id attribute");
			</function>
			<function name="print" params="out">
				out.at(this.__xs__line);
				out.printVar(this.id);
				out.cat(this.value, ";");
			</function>
		</object>
		
		<object name="null" prototype="kprGrammar.item" pattern="null">
			<string name="id" pattern="@id"/>
			<function name="prepare" params="out">
				if (!this.id)
					out.reportError(this, "missing id attribute");
			</function>
			<function name="print" params="out">
				out.at(this.__xs__line);
				out.printVar(this.id);
				out.cat("null;");
			</function>
		</object>
		
		<object name="script" prototype="kprGrammar.item" pattern="script">
			<string name="cdata" pattern="."/>
			<function name="print" params="out">
				var c = kprGrammar.countLines(this.cdata);
				out.at(this.__xs__line + c);
				out.line(this.cdata.trim());
			</function>
		</object>
		
		<object name="_function" prototype="kprGrammar.item" pattern="function">
			<string name="id" pattern="@id"/>
			<string name="params" pattern="@params"/>
			<string name="cdata" pattern="."/>
			<function name="prepare" params="out">
				if (!this.id)
					out.reportError(this, "missing id attribute");
			</function>
			<function name="print" params="out">
				var c = kprGrammar.countLines(this.cdata);
				out.at(this.__xs__line + c - 1);
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
			</function>
		</object>
		
		<object name="constructor" prototype="kprGrammar.item" pattern="constructor">
			<string name="params" pattern="@params"/>
			<string name="cdata" pattern="."/>
		</object>
		<object name="member" prototype="kprGrammar.item">
			<string name="id" pattern="@id"/>
			<function name="bind" params="member, out">
				out.reportError(member, "invalid id");
			</function>
			<function name="prepare" params="out, i, members">
				if (!this.id)
					out.reportError(this, "missing id attribute");
				var c = members.length;
				for (i++; i < c; i++) {
					var member = members[i];
					if (member.id == this.id)
						member.bind(this, out);
				} 
			</function>
		</object>
		<object name="field" prototype="kprGrammar.member" pattern="field">
			<string name="value" value="undefined" pattern="@value"/>
			<function name="print" params="out">
				out.line(this.id, ": { value: ", this.value, ", writable: true },");
			</function>
			<function name="printProperty" params="out">
				out.line(this.id, ": ", this.value, ",");
			</function>
		</object>
		<object name="method" prototype="kprGrammar.member" pattern="method">
			<string name="params" pattern="@params"/>
			<string name="cdata" pattern="."/>
			<function name="print" params="out">
				out.line(this.id, ": { value: ");
				var c = kprGrammar.countLines(this.cdata);
				out.at(this.__xs__line + c - 1);
				out.tab(1);
				out.line("function(", this.params, ") {");
				out.tab(1);
				out.line(this.cdata.trim());
				out.tab(-1);
				out.line("},");
				out.tab(-1);
				out.line("},");
			</function>
			<function name="printProperty" params="out">
				var c = kprGrammar.countLines(this.cdata);
				out.at(this.__xs__line + c - 1);
				out.line(this.id, ": function(", this.params, ") {");
				out.tab(1);
				out.line(this.cdata.trim());
				out.tab(-1);
				out.line("},");
			</function>
		</object>
		<object name="getter" prototype="kprGrammar.member" pattern="getter">
			<string name="cdata" pattern="."/>
			<null name="setter"/>
			<function name="bind" params="member, out">
				if (kprGrammar.setter.isPrototypeOf(member) && (this.setter == null))
					this.setter = member;
				else
					out.reportError(this, "invalid id");
			</function>
			<function name="print" params="out">
				out.line(this.id, ": {");
				var c = kprGrammar.countLines(this.cdata);
				out.at(this.__xs__line + c - 1);
				out.tab(1);
				out.line("get: function() {");
				out.tab(1);
				out.line(this.cdata.trim());
				out.tab(-1);
				out.line("},");
				if (this.setter) {
					var c = kprGrammar.countLines(this.setter.cdata);
					out.at(this.setter.__xs__line + c - 1);
					out.line("set: function(", this.setter.params, ") {");
					out.tab(1);
					out.line(this.setter.cdata.trim());
					out.tab(-1);
					out.line("},");
				}
				out.tab(-1);
				out.line("},");
			</function>
			<function name="printProperty" params="out">
				var c = kprGrammar.countLines(this.cdata);
				out.at(this.__xs__line + c - 1);
				out.line("get ", this.id, "() {");
				out.tab(1);
				out.line(this.cdata.trim());
				out.tab(-1);
				out.line("},");
			</function>
		</object>
		<object name="setter" prototype="kprGrammar.member" pattern="setter">
			<function name="bind" params="member, out">
				if (kprGrammar.getter.isPrototypeOf(member) && (member.setter == null))
					member.setter = this;
				else
					out.reportError(this, "invalid id");
			</function>
			<string name="params" pattern="@params"/>
			<string name="cdata" pattern="."/>
			<function name="printProperty" params="out">
				var c = kprGrammar.countLines(this.cdata);
				out.at(this.__xs__line + c - 1);
				out.line("set ", this.id, "(", this.params, ") {");
				out.tab(1);
				out.line(this.cdata.trim());
				out.tab(-1);
				out.line("},");
			</function>
		</object>
		
		<object name="_class" prototype="kprGrammar.item" pattern="class">
			<reference name="constructor" contents="kprGrammar.constructor" pattern="."/>
			<string name="id" pattern="@id"/>
			<string name="like" value="Object" pattern="@like"/>
			<array name="members" contents="kprGrammar.member" pattern="."/>
			<function name="prepare" params="out">
				if (!this.id)
					out.reportError(this, "missing id attribute");
				this.members.forEach(this.prepareMember, out); 	
			</function>
			<function name="prepareMember" params="member, i, members">
				member.prepare(this, i, members); 	
			</function>
			<function name="print" params="out">
				var constructor = this.constructor
				if (constructor == kprGrammar.constructor) {
					out.printVar(this.id);
					out.cat("function() {");
					out.tab(1);
					out.line(this.like, ".call(this);");
					out.tab(-1);
					out.line("};");
				}
				else {
					var c = kprGrammar.countLines(constructor.cdata);
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
			</function>
			<function name="printMember" params="member">
				member.print(this); 	
			</function>
		</object>

		<object name="effectPart">
		</object>
		<object name="colorizePart" prototype="kprGrammar.effectPart" pattern="colorize">
			<string name="color" value="gray" pattern="@color"/>
			<number name="opacity" value="1" pattern="@opacity"/>
			<function name="print" params="out">
				out.cat("colorize('", this.color, "', ", this.opacity, ");");
			</function>
		</object>
		<object name="innerGlowPart" prototype="kprGrammar.effectPart" pattern="inner-glow">
			<number name="blur" value="1" pattern="@blur"/>
			<string name="color" value="white" pattern="@color"/>
			<number name="opacity" value="1" pattern="@opacity"/>
			<number name="radius" value="1" pattern="@radius"/>
			<function name="print" params="out">
				out.cat("innerGlow('", this.color, "', ", this.opacity, ", ", this.blur, ", ", this.radius, ");");
			</function>
		</object>
		<object name="innerHilitePart" prototype="kprGrammar.effectPart" pattern="inner-hilite">
			<number name="blur" value="1" pattern="@blur"/>
			<string name="color" value="white" pattern="@color"/>
			<number name="opacity" value="1" pattern="@opacity"/>
			<number name="x" value="0" pattern="@x"/>
			<number name="y" value="0" pattern="@y"/>
			<function name="print" params="out">
				out.cat("innerHilite('", this.color, "', ", this.opacity, ", ", this.blur, ", ", this.x, ", ", this.y, ");");
			</function>
		</object>
		<object name="innerShadowPart" prototype="kprGrammar.effectPart" pattern="inner-shadow">
			<number name="blur" value="1" pattern="@blur"/>
			<string name="color" value="black" pattern="@color"/>
			<number name="opacity" value="1" pattern="@opacity"/>
			<number name="x" value="0" pattern="@x"/>
			<number name="y" value="0" pattern="@y"/>
			<function name="print" params="out">
				out.cat("innerShadow('", this.color, "', ", this.opacity, ", ", this.blur, ", ", this.x, ", ", this.y, ");");
			</function>
		</object>
		<object name="maskPart" prototype="kprGrammar.effectPart" pattern="mask">
			<string name="texture" pattern="@texture"/>
			<function name="print" params="out">
				out.cat("mask(", this.texture, ");");
			</function>
		</object>
		<object name="outerGlowPart" prototype="kprGrammar.effectPart" pattern="outer-glow">
			<number name="blur" value="1" pattern="@blur"/>
			<string name="color" value="white" pattern="@color"/>
			<number name="opacity" value="1" pattern="@opacity"/>
			<number name="radius" value="1" pattern="@radius"/>
			<function name="print" params="out">
				out.cat("outerGlow('", this.color, "', ", this.opacity, ", ", this.blur, ", ", this.radius, ");");
			</function>
		</object>
		<object name="outerHilitePart" prototype="kprGrammar.effectPart" pattern="outer-hilite">
			<number name="blur" value="1" pattern="@blur"/>
			<string name="color" value="white" pattern="@color"/>
			<number name="opacity" value="1" pattern="@opacity"/>
			<number name="x" value="0" pattern="@x"/>
			<number name="y" value="0" pattern="@y"/>
			<function name="print" params="out">
				out.cat("outerHilite('", this.color, "', ", this.opacity, ", ", this.blur, ", ", this.x, ", ", this.y, ");");
			</function>
		</object>
		<object name="outerShadowPart" prototype="kprGrammar.effectPart" pattern="outer-shadow">
			<number name="blur" value="1" pattern="@blur"/>
			<string name="color" value="black" pattern="@color"/>
			<number name="opacity" value="1" pattern="@opacity"/>
			<number name="x" value="0" pattern="@x"/>
			<number name="y" value="0" pattern="@y"/>
			<function name="print" params="out">
				out.cat("outerShadow('", this.color, "', ", this.opacity, ", ", this.blur, ", ", this.x, ", ", this.y, ");");
			</function>
		</object>
		<object name="shadePart" prototype="kprGrammar.effectPart" pattern="shade">
			<number name="opacity" value="1" pattern="@opacity"/>
			<string name="texture" pattern="@texture"/>
			<function name="print" params="out">
				out.cat("shade('", this.texture, "', ", this.opacity, ");");
			</function>
		</object>
		<object name="effect" prototype="kprGrammar.item" pattern="effect">
			<string name="id" pattern="@id"/>
			<array name="parts" contents="kprGrammar.effectPart" pattern="."/>
			<function name="prepare" params="out">
				if (!this.id)
					out.reportError(this, "missing id attribute");
			</function>
			<function name="print" params="out">
				out.at(this.__xs__line);
				out.printVar(this.id);
				out.cat("new Effect();");
				var parts = this.parts;
				var c = parts.length;
				for (var i = 0; i < c; i++) {
					out.line(this.id, ".");
					parts[i].print(out);
				}
			</function>
		</object>
		
		<object name="texture" prototype="kprGrammar.item" pattern="texture">
			<string name="id" pattern="@id"/>
			<string name="small" pattern="@small"/>
			<string name="medium" pattern="@medium"/>
			<string name="large" pattern="@large"/>
			<function name="prepare" params="out">
				if (!this.id)
					out.reportError(this, "missing id attribute");
			</function>
			<function name="print" params="out">
				out.at(this.__xs__line);
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
			</function>
		</object>
		
		<object name="variants" pattern="variants">
			<number name="offset" pattern="@offset"/>
			<string name="names" pattern="@names"/>
		</object>
		
		<object name="states" pattern="states">
			<number name="offset" pattern="@offset"/>
			<string name="colors" pattern="@colors"/>
			<string name="names" pattern="@names"/>
		</object>
		
		<object name="tiles" pattern="tiles">
			<number name="left" pattern="@left"/>
			<number name="right" pattern="@right"/>
			<number name="top" pattern="@top"/>
			<number name="bottom" pattern="@bottom"/>
		</object>
		
		<object name="margins" pattern="margins">
			<number name="left" pattern="@left"/>
			<number name="right" pattern="@right"/>
			<number name="top" pattern="@top"/>
			<number name="bottom" pattern="@bottom"/>
		</object>
		
		<object name="borders" pattern="borders">
			<number name="left" pattern="@left"/>
			<number name="right" pattern="@right"/>
			<number name="top" pattern="@top"/>
			<number name="bottom" pattern="@bottom"/>
			<string name="color" pattern="@color"/>
			<reference name="states" contents="kprGrammar.states" pattern="."/>
		</object>
		
		<object name="skin" prototype="kprGrammar.item" pattern="skin">
			<string name="id" pattern="@id"/>
			
			<string name="texture" pattern="@texture"/>
			<number name="x" pattern="@x"/>
			<number name="y" pattern="@y"/>
			<number name="width" pattern="@width"/>
			<number name="height" pattern="@height"/>
			<reference name="variants" contents="kprGrammar.variants" pattern="."/>
			<reference name="states" contents="kprGrammar.states" pattern="."/>
			<reference name="tiles" contents="kprGrammar.tiles" pattern="."/>
			<reference name="margins" contents="kprGrammar.margins" pattern="."/>
			<string name="aspect" value="draw" pattern="@aspect"/>
			
			<string name="color" pattern="@color"/>
			<reference name="borders" contents="kprGrammar.borders" pattern="."/>
			
			<function name="prepare" params="out">
				if (!this.id)
					out.reportError(this, "missing id attribute");
				this.prepareConstructor(out);
			</function>
			<function name="prepareConstructor" params="out">
				if (this.texture) {
					out.parseConstants(this.variants.names);
					out.parseConstants(this.states.names);
				}
				else {
					out.parseConstants(this.states.names);
					out.parseConstants(this.borders.states.names);
				}
			</function>
			<function name="print" params="out">
				out.at(this.__xs__line);
				if (this.id == "applicationIconSkin")
					out.line("applicationIconSkin = ");
				else
					out.printVar(this.id);
				this.printConstructor(out);
				out.cat(";");
			</function>
			<function name="printConstructor" params="out">
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
					if (this.tiles != kprGrammar.tiles) {
						out.cat("tiles: ");
						this.printCoordinates(out, this.tiles);
						out.cat(", ");
					}
					if (this.margins != kprGrammar.margins) {
						out.cat("margins: ");
						this.printCoordinates(out, this.margins);
						out.cat(", ");
					}
					if (this.aspect != "draw")
						out.cat("aspect: '", this.aspect, "', ");
				}
				else {
					if (this.borders != kprGrammar.borders) {
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
			</function>
			<function name="printCoordinate" params="out, instance, property, flag">
				if (instance.hasOwnProperty(property)) {
					if (flag) 
						out.cat(", ");
					out.cat(property, ":", instance[property]);
					return true;
				}
			</function>
			<function name="printCoordinates" params="out, instance">
				var flag = false;
				out.cat("{ ");
				flag |= this.printCoordinate(out, instance, "left", flag);
				flag |= this.printCoordinate(out, instance, "right", flag);
				flag |= this.printCoordinate(out, instance, "top", flag);
				flag |= this.printCoordinate(out, instance, "bottom", flag);
				out.cat(" }");
			</function>
		</object>

		<object name="style" prototype="kprGrammar.item" pattern="style">
			<string name="id" pattern="@id"/>
			<string name="font" pattern="@font"/>
			<string name="color" pattern="@color"/>
			<reference name="states" contents="kprGrammar.states" pattern="."/>
			<string name="align" pattern="@align"/>
			<string name="horizontal" value="null"/>
			<number name="indentation" pattern="@indentation"/>
			<number name="leading" pattern="@leading"/>
			<number name="lines" pattern="@lines"/>
			<reference name="margins" contents="kprGrammar.margins" pattern="."/>
			<string name="vertical" value="null"/>
			<function name="prepare" params="out">
				if (!this.id)
					out.reportError(this, "missing id attribute");
				this.prepareConstructor(out);
			</function>
			<function name="prepareConstructor" params="out">
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
			</function>
			<function name="print" params="out">
				out.at(this.__xs__line);
				out.printVar(this.id);
				this.printConstructor(out);
				out.cat(";");
			</function>
			<function name="printConstructor" params="out">
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
			</function>
		</object>

		<object name="behavior" prototype="kprGrammar.item" pattern="behavior">
			<string name="id" pattern="@id"/>
			<string name="like" value="Behavior" pattern="@like"/>
			<array name="members" contents="kprGrammar.member" pattern="."/>
			<string name="script" pattern="."/>
			<regexp name="top" value="/^(\s+)function\s+([^\(]+)/"/>
			<regexp name="middle" value="/}(\s+)function\s+([^\(]+)/g"/>
			<regexp name="bottom" value="/}\s+$/"/>
			<boolean name="embed"/>
			<function name="prepare" params="out">
				if (!this.embed && !this.id)
					out.reportError(this, "missing id attribute");
				if (this.id == this.like)
					out.reportError(this, "invalid id attribute");
				this.members.forEach(this.prepareMember, out); 	
			</function>
			<function name="prepareMember" params="member, i, members">
				member.prepare(this, i, members); 	
			</function>
			<function name="print" params="out">
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
			</function>
			<function name="printMember" params="member">
				member.print(this); 	
			</function>
			<function name="printMemberProperty" params="member">
				member.printProperty(this); 	
			</function>
			<function name="printObject" params="out">
				out.cat("Object.create(", this.like, ".prototype, {");
				out.tab(1);
				if (this.members.length)
					this.members.forEach(this.printMember, out); 	
				else
					this.printScript(out);
				out.tab(-1);
				out.line("})");
			</function>
			<function name="printScript" params="out">
				var c = kprGrammar.countLines(this.script);
				out.at(this.__xs__line + c);
				var text = this.script;
				text = text.replace(this.top, "$2: { value: function");
				text = text.replace(this.middle, "}},$1$2: { value: function");
				text = text.replace(this.bottom, "}}");
				out.line(text);
			</function>
			<function name="printScriptProperty" params="out">
				var c = kprGrammar.countLines(this.script);
				out.at(this.__xs__line + c);
				var text = this.script;
				text = text.replace(this.top, "$2: function");
				text = text.replace(this.middle, "},$1$2: function");
				text = text.replace(this.bottom, "}");
				out.line(text);
			</function>
		</object>
		
		<object name="transition" prototype="kprGrammar.behavior" pattern="transition">
			<number name="duration" value="0" pattern="@duration"/>
			<string name="like" value="Transition" pattern="@like"/>
			<function name="print" params="out">
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
			</function>
		</object>
		
		<object name="handler" prototype="kprGrammar.item" pattern="handler">
			<string name="path" pattern="@path"/>
			<string name="behaviorID" value="" pattern="@behavior"/>
			<reference name="behavior" contents="kprGrammar.behavior" pattern="."/>
			<function name="prepare" params="out">
				if (this.behavior != kprGrammar.behavior) {
					if (this.behaviorID)
						out.reportError(this, "a handler element cannot have a behavior attribute and a behavior element");
					else if (this.behavior.id)
						out.reportError(this.behavior, "nested behavior element cannot have an id attribute");
					else {
						this.behavior.embed = true;
						this.behavior.prepare();
					}
				}
			</function>
			<function name="print" params="out">
				out.line("Handler.bind(\"", this.path, "\", ");
				if (this.behaviorID) {
					out.cat("Object.create(", this.behaviorID, ".prototype)");
				}
				else {
					this.behavior.printObject(out);
				}
				out.cat(");");
			</function>
		</object>
		
		<object name="any" prototype="kprGrammar.item">
			<function name="prepareContent" params="out">
			</function>
			<function name="printContent" params="out">
			</function>
		</object>

		<object name="insert" prototype="kprGrammar.any" pattern="insert">
			<function name="printContent" params="out">
				out.line("builder.call(", out.container, ";");
			</function>
		</object>
		
		<object name="iterate" prototype="kprGrammar.any" pattern="iterate">
			<array name="contents" contents="kprGrammar.any" pattern="."/>
			<string name="on" pattern="@on"/>
			<function name="prepareContent" params="out">
				this.contents.forEach(out.prepareContent, out); 	
			</function>
			<function name="printContent" params="out">
				out.at(this.__xs__line);
				out.line("(", this.on, ") ? ", this.on, ".map(function($) { var $$ = this; return [");
				out.tab(1);
				this.contents.forEach(out.printContent, out); 	
				out.tab(-1);
				out.line("]}, $) : null, ");
			</function>
		</object>
		
		<object name="scope" prototype="kprGrammar.any" pattern="scope">
			<array name="contents" contents="kprGrammar.any" pattern="."/>
			<string name="_with" pattern="@with"/>
			<function name="prepareContent" params="out">
				this.contents.forEach(out.prepareContent, out); 	
			</function>
			<function name="printContent" params="out">
				out.at(this.__xs__line);
				out.line("(function($, $$) { return [");
				out.tab(1);
				this.contents.forEach(out.printContent, out); 	
				out.tab(-1);
				out.line("]})(", this._with, ", $), ");
			</function>
		</object>

		<object name="select" prototype="kprGrammar.any" pattern="select">
			<array name="contents" contents="kprGrammar.any" pattern="."/>
			<string name="on" pattern="@on"/>
			<function name="prepareContent" params="out">
				this.contents.forEach(out.prepareContent, out); 	
			</function>
			<function name="printContent" params="out">
				out.at(this.__xs__line);
				out.line("(", this.on, ") ? [");
				out.tab(1);
				this.contents.forEach(out.printContent, out); 	
				out.tab(-1);
				out.line("] : null, ");
			</function>
		</object>
		
		<object name="content" prototype="kprGrammar.any" pattern="content">
			<string name="constructor" value="Content"/>
			<string name="id" pattern="@id"/>
			<string name="like" pattern="@like"/>
			<string name="_with" value="$" pattern="@with"/>
			<string name="keys" pattern="@keys"/>
			<string name="anchor" pattern="@anchor"/>
			<string name="name" pattern="@name"/>
			
			<string name="behaviorID" value="" pattern="@behavior"/>
			<reference name="behavior" contents="kprGrammar.behavior" pattern="."/>
			<null name="behaviors"/>
			
			<string name="horizontal" pattern="@horizontal"/>
			<string name="left" pattern="@left"/>
			<string name="width" pattern="@width"/>
			<string name="right" pattern="@right"/>
			<string name="vertical" pattern="@vertical"/>
			<string name="top" pattern="@top"/>
			<string name="height" pattern="@height"/>
			<string name="bottom" pattern="@bottom"/>
			
			<string name="skinID" value="" pattern="@skin"/>
			<reference name="skin" contents="kprGrammar.skin" pattern="."/>
			
			<string name="styleID" value="" pattern="@style"/>
			<reference name="style" contents="kprGrammar.style" pattern="."/>

			<string name="active" pattern="@active"/>
			<string name="backgroundTouch" pattern="@backgroundTouch"/>
			<string name="exclusiveTouch" pattern="@exclusiveTouch"/>
			<string name="mutipleTouch" pattern="@mutipleTouch"/>
			<string name="visible" pattern="@visible"/>
			<string name="state" value="" pattern="@state"/>
			<string name="variant" value="" pattern="@variant"/>
		
			<function name="prepare" params="out">
				if (!this.id)
					out.reportError(this, "missing id attribute");
				out.current = this;
				this.behaviors = [];
				this.prepareContent(out); 	
			</function>
			<function name="prepareContent" params="out">
				if (this.id && !this.behaviors)
					out.reportError(this, "nested content element cannot have an id attribute");
				out.prepareBehavior(this);
				if (this.skin != kprGrammar.skin) {
					if (this.skinID)
						out.reportError(this, "a content element cannot have a skin attribute and a skin element");
					else if (this.skin.id)
						out.reportError(this.skin, "nested skin element cannot have an id attribute");
					else
						this.skin.prepareConstructor(out);
				}
				if (this.style != kprGrammar.style) {
					if (this.styleID) 
						out.reportError(this, "a content element cannot have a style attribute and a style element");
					else if (this.style.id)
						out.reportError(this.style, "nested style element cannot have an id attribute");
					else
						this.style.prepareConstructor(out);
				}
			</function>
			<function name="print" params="out">
				out.current = this;
				out.at(this.__xs__line);
				out.printVar(this.id);
				if (this.like)
					out.cat(this.like);
				else
					out.cat(this.constructor);
				out.cat(".template(function($) { return { ");
				this.printProperties(out);
				out.cat("}});");
				if (this.behaviors.length) {
					out.line(this.id, ".behaviors = new Array(", this.behaviors.length, ");");
					this.behaviors.forEach(out.printItem, out);
				}
			</function>
			<function name="printConstructor" params="out">
				if (this.like)
					out.line(this.like);
				else
					out.line(this.constructor);
				out.cat("(", this._with, ", { ");
				this.printProperties(out);
				out.cat("})");
			</function>
			<function name="printContent" params="out">
				out.at(this.__xs__line);
				this.printConstructor(out);
				out.cat(",");
			</function>
			<function name="printProperties" params="out">
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
				else if (this.skin != kprGrammar.skin) {
					out.cat("skin: ");
					this.skin.printConstructor(out);
					out.cat(", ");
				}
				if (this.styleID)
					out.cat("style: ", this.styleID, ", ");
				else if (this.style != kprGrammar.style) {
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
			</function>
			<function name="printProperty" params="out, property">
				if (this.hasOwnProperty(property)) {
					out.cat(property, ": ", this[property], ", ");
				}
			</function>
		</object>
		
		<object name="container" prototype="kprGrammar.content" pattern="container">
			<string name="constructor" value="Container"/>
			<array name="contents" contents="kprGrammar.any" pattern="."/>
			<string name="clip" pattern="@clip"/>
			<function name="prepareContent" params="out">
				kprGrammar.content.prepareContent.call(this, out);
				this.contents.forEach(out.prepareContent, out); 	
			</function>
			<function name="printProperties" params="out">
				kprGrammar.content.printProperties.call(this, out);
				this.printProperty(out, "clip");
				if (this.contents.length) {
					out.cat("contents: [");
					out.tab(1);
					this.contents.forEach(out.printContent, out); 	
					out.tab(-1);
					out.line("], ");
				}
			</function>
		</object>
		
		<object name="layout" prototype="kprGrammar.container" pattern="layout">
			<string name="constructor" value="Layout"/>
		</object>
		
		<object name="port" prototype="kprGrammar.content" pattern="port">
			<string name="constructor" value="Port"/>
		</object>
		
		<object name="canvas" prototype="kprGrammar.content" pattern="canvas">
			<string name="constructor" value="Canvas"/>
		</object>
		
		<object name="scroller" prototype="kprGrammar.container" pattern="scroller">
			<string name="constructor" value="Scroller"/>
			<string name="loop" pattern="@loop"/>
			<function name="printProperties" params="out">
				kprGrammar.container.printProperties.call(this, out);
				if (this.loop)
					out.cat("loop: ", this.loop, ", ");
			</function>
		</object>
			
		<object name="column" prototype="kprGrammar.container" pattern="column">
			<string name="constructor" value="Column"/>
		</object>
			
		<object name="line" prototype="kprGrammar.container" pattern="line">
			<string name="constructor" value="Line"/>
		</object>
	
		<object name="label" prototype="kprGrammar.content" pattern="label">
			<string name="constructor" value="Label"/>
			<string name="editable" pattern="@editable"/>
			<string name="hidden" pattern="@hidden"/>
			<string name="selectable" pattern="@selectable"/>
			<string name="string" pattern="@string"/>
			<function name="printProperties" params="out">
				kprGrammar.content.printProperties.call(this, out);
				this.printProperty(out, "editable");
				this.printProperty(out, "hidden");
				this.printProperty(out, "selectable");
				if (this.string)
					out.cat("string: ", this.string, ", ");
			</function>
		</object>
		
		<object name="span" pattern="span">
			<string name="behaviorID" value="" pattern="@behavior"/>
			<reference name="behavior" contents="kprGrammar.behavior" pattern="."/>
			<string name="string" pattern="@string"/>
			<string name="style" value="" pattern="@style"/>
			<string name="_with" value="$" pattern="@with"/>
			<function name="prepareContent" params="out">
				out.prepareBehavior(this);
			</function>
			<function name="print" params="out">
				out.at(this.__xs__line);
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
			</function>
		</object>
		
		<object name="wrap" pattern="wrap">
			<string name="alignment" value="middle" pattern="@alignment"/>
			<reference name="content" contents="kprGrammar.content" pattern="."/>
			<function name="prepareContent" params="out">
				this.content.prepareContent(out);
			</function>
			<function name="print" params="out">
				out.at(this.__xs__line);
				out.line("{ align: '", this.alignment, "', content: ");
				this.content.printConstructor(out);
				out.cat("},");
			</function>
		</object>
		
		<object name="block" pattern="block">
			<string name="behaviorID" value="" pattern="@behavior"/>
			<reference name="behavior" contents="kprGrammar.behavior" pattern="."/>
			<array name="spans" contents="kprGrammar.span,kprGrammar.wrap" pattern="."/>
			<string name="string" pattern="@string"/>
			<string name="style" value="" pattern="@style"/>
			<string name="_with" value="$" pattern="@with"/>
			<function name="prepareSpan" params="span">
				span.prepareContent(this);
			</function>
			<function name="prepareContent" params="out">
				out.prepareBehavior(this);
				this.spans.forEach(this.prepareSpan, out); 	
			</function>
			<function name="print" params="out">
				out.at(this.__xs__line);
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
			</function>
			<function name="printSpan" params="span">
				span.print(this);
			</function>
		</object>
		
		<object name="text" prototype="kprGrammar.content" pattern="text">
			<string name="constructor" value="Text"/>
			<array name="blocks" contents="kprGrammar.block" pattern="."/>
			<string name="editable" pattern="@editable"/>
			<string name="selectable" pattern="@selectable"/>
			<string name="string" pattern="@string"/>
			<function name="prepareBlock" params="block">
				block.prepareContent(this);
			</function>
			<function name="prepareContent" params="out">
				kprGrammar.content.prepareContent.call(this, out);
				this.blocks.forEach(this.prepareBlock, out); 	
			</function>
			<function name="printBlock" params="block">
				block.print(this);
			</function>
			<function name="printProperties" params="out">
				kprGrammar.content.printProperties.call(this, out);
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
			</function>
		</object>
	
		<object name="resource" prototype="kprGrammar.content">
			<string name="url" value="null" pattern="@url"/>
			<string name="aspect" value="" pattern="@aspect"/>
			<string name="mime" value="null" pattern="@mime"/>
			<function name="printProperties" params="out">
				kprGrammar.content.printProperties.call(this, out);
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
			</function>
		</object>

		<object name="picture" prototype="kprGrammar.resource" pattern="picture">
			<string name="constructor" value="Picture"/>
		</object>
	
		<object name="thumbnail" prototype="kprGrammar.resource" pattern="thumbnail">
			<string name="constructor" value="Thumbnail"/>
			<string name="url" value="null" pattern="@url"/>
			<string name="mime" value="null" pattern="@mime"/>
			<string name="aspect" value="" pattern="@aspect"/>
		</object>
	
		<object name="media" prototype="kprGrammar.resource" pattern="media">
			<string name="constructor" value="Media"/>
			<string name="url" value="null" pattern="@url"/>
			<string name="mime" value="null" pattern="@mime"/>
			<string name="aspect" value="" pattern="@aspect"/>
		</object>
	
		<object name="browser" prototype="kprGrammar.content" pattern="browser">
			<string name="constructor" value="Browser"/>
			<string name="url" value="null" pattern="@url"/>
			<function name="prepare" params="out">
				out.reportError(this, "the browser element must be nested");
				kprGrammar.content.prototype.prepare.call(this, out);
			</function>
			<function name="printConstructor" params="out">
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
			</function>
		</object>

		<object name="program" pattern="/program">
			<string name="id" pattern="@id"/>
			<array name="items" contents="kprGrammar.include, kprGrammar.require, kprGrammar.script, kprGrammar._function, kprGrammar._class, kprGrammar.null, kprGrammar.variable, kprGrammar.effect, kprGrammar.texture, kprGrammar.skin, kprGrammar.style, kprGrammar.behavior, kprGrammar.handler, kprGrammar.content" pattern="."/>
			
			<null name="code"/>
			<null name="constants"/>
			<number name="contentDepth"/>
			<boolean name="private"/>
			<number name="scopeDepth"/>
			<number name="tabDepth"/>
			<number name="origin"/>
			
			<function name="at" params="line">
				//if (this.origin != line) {
				//	this.origin = line;
					this.cat("\n//@line ", line);
				//}
			</function>
			<function name="cat">
				var c = arguments.length;
				for (var i = 0; i < c; i++)
					this.code += arguments[i];
			</function>
			<function name="line">
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
			</function>
			<function name="generate">
				this.constants = {};
				this.items.forEach(this.prepareItem, this); 	
				this.code = "";
				this.printKind();
				this.origin = this.__xs__line;
				this.cat("//@line ", this.__xs__line, " \"", this.__xs__path, "\"");
				this.line("/* KPS2JS GENERATED FILE; DO NOT EDIT! */");
				this.printConstants(this);
				this.items.forEach(this.printItem, this); 	
				return this.code;
			</function>
			<function name="nest" params="delta">
				this.contentDepth += delta;
			</function>
			<function name="parseConstant" params="name, value">
				this.constants[name] = value;
			</function>
			<function name="parseConstants" params="names">
				if (names) {	
					names = names.split(",");
					var c = names.length;
					for (var i = 0; i < c; i++)
						this.parseConstant(names[i], i);
				}
			</function>
			<function name="prepareBehavior" params="item">
				if (item.behavior != kprGrammar.behavior) {
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
			</function>
			<function name="prepareContent" params="content">
				content.prepareContent(this);
			</function>
			<function name="prepareItem" params="item, i">
				item.prepare(this);
			</function>
			<function name="printColors" params="colors">
				var states = colors.split(",");
				var c = states.length;
				this.cat("[");
				for (var i = 0; i < c; i++) {
					if (i) this.cat(", ");
					this.cat("'", states[i], "'");
				}
				this.cat("]");
			</function>
			<function name="printConstants" params="instance">
				var instance = this.constants;
				for (var i in instance) {
					this.printVar(i);
					this.cat(instance[i], ";");
				}
			</function>
			<function name="printContent" params="content">
				content.printContent(this);
			</function>
			<function name="printItem" params="item">
				item.print(this);
			</function>
			<function name="printTexture" params="id, scale">
				this.cat("new Texture('", id, "', ", scale, ")");
			</function>
			<function name="printKind">
				this.cat("//@program\n");
			</function>
			<function name="printURL" params="url">
				this.cat("url: ", url, ", ");
			</function>
			<function name="printVar" params="id">
				this.line("var ", id, " = ");
			</function>
			<function name="serializeCoordinate" params="instance, property, text, quote">
				if (instance.hasOwnProperty(property)) {
					if (text) 
						text += ", ";
					text += property + ":" 
					if (quote) text += "'";
					text += instance[property];
					if (quote) text += "'";
				}
				return text;
			</function>
			<function name="serializeCoordinates" params="instance">
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
			</function>
			<function name="tab" params="delta">
				this.tabDepth += delta;
			</function>
			<function name="get temporary">
				var depth = this.contentDepth;
				return "_" + depth;
			</function>
			<function name="get content">
				var depth = this.contentDepth;
				if (depth)
					return "$" + depth;
				return "this";
			</function>
			<function name="get container">
				var depth = this.contentDepth;
				if (depth > 1)
					return "$" + (depth - 1);
				return "this";
			</function>
			<function name="get local">
				var result = "$";
				var c = this.scopeDepth;
				while (c) {
					result += "$";
					c--;
				}
				return result;
			</function>
			<function name="reportError" params="instance, message">
                if (instance)
                    throw new Error(instance.__xs__path + ":" + instance.__xs__line + ": " + message);
				throw new Error(message);
			</function>
			<function name="scope" params="path">
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
			</function>
			<function name="unscope">
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
			</function>
		</object>
	
		<object name="private" pattern="private">
			<array name="items" contents="kprGrammar._function, kprGrammar._class, kprGrammar.null, kprGrammar.variable, kprGrammar.effect, kprGrammar.texture, kprGrammar.skin, kprGrammar.style, kprGrammar.behavior, kprGrammar.handler, kprGrammar.content" pattern="."/>
			<function name="prepare" params="out">
				this.items.forEach(this.prepareItem, out); 	
			</function>
			<function name="prepareItem" params="item, i">
				item.prepare(this);
			</function>
			<function name="print" params="out">
				out.private = true;
				this.items.forEach(this.printItem, out); 	
				out.private = false;
			</function>
			<function name="printItem" params="item, i">
				item.print(this);
			</function>
		</object>
		<object name="module" prototype="kprGrammar.program" pattern="/module">
			<array name="items" contents="kprGrammar.include, kprGrammar.require, kprGrammar.script, kprGrammar.private, kprGrammar._function, kprGrammar._class, kprGrammar.null, kprGrammar.variable, kprGrammar.effect, kprGrammar.texture, kprGrammar.skin, kprGrammar.style, kprGrammar.behavior, kprGrammar.handler, kprGrammar.content" pattern="."/>
			<function name="printKind">
				this.cat("//@module\n");
			</function>
			<function name="printURL" params="url">
				this.cat("url: mergeURI(module.uri, ", url, "), ");
			</function>
			<function name="printVar" params="id">
				if (this.private)
					this.line("var ", id, " = ");
				else
					this.line("var ", id, " = exports.", id, " = ");
			</function>
		</object>
		<object name="shell" pattern="/shell">
			<string name="cdata" pattern="."/>
			<string name="modules" value="modules/" pattern="@modules"/>
			<number name="width" value="640" pattern="@width"/>
			<number name="height" value="480" pattern="@height"/>
			<function name="generate">
				return "//@program\n" + this.cdata;
			</function>
		</object>
	</object>
</package>
