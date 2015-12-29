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
	<object name="DOM">
		<string name="xmlPrefix" value="xml" script="false"/>
		<string name="xmlNamespace" value="http://www.w3.org/XML/1998/namespace" script="false"/>
		<string name="xmlnsPrefix" value="xmlns" script="false"/>
		<string name="xmlnsNamespace" value="http://www.w3.org/2000/xmlns/" script="false"/>
		
		<function name="checkAttributeNS" params="attribute" script="false">
			var namespace = attribute.namespace;
			var name = attribute.name;
			var prefix = attribute.prefix;
			if ((prefix != null) && (namespace == null))
				throw new DOM.Exception(DOM.exception.NAMESPACE_ERR);
			if ((prefix == DOM.xmlPrefix) && (namespace != DOM.xmlNamespace))
				throw new DOM.Exception(DOM.exception.NAMESPACE_ERR);
			if ((prefix == DOM.xmlnsPrefix) && (namespace != DOM.xmlnsNamespace))
				throw new DOM.Exception(DOM.exception.NAMESPACE_ERR);
			if ((name == DOM.xmlnsPrefix) && (namespace != DOM.xmlnsNamespace))
				throw new DOM.Exception(DOM.exception.NAMESPACE_ERR);
			if ((prefix != DOM.xmlnsPrefix) && (name != DOM.xmlnsPrefix) && (namespace == DOM.xmlnsNamespace))
				throw new DOM.Exception(DOM.exception.NAMESPACE_ERR);
		</function>
		<function name="checkElementNS" params="element" script="false">
			var namespace = element.namespace;
			var prefix = element.prefix;
			if ((prefix != null) && (namespace == null))
				throw new DOM.Exception(DOM.exception.NAMESPACE_ERR);
			if ((prefix == DOM.xmlPrefix) && (namespace != DOM.xmlNamespace))
				throw new DOM.Exception(DOM.exception.NAMESPACE_ERR);
		</function>
		<function name="compareAttributes" params="a, b" script="false">
			return xs.infoset.compareAttributes(a, b);
		</function>
		<function name="parse" params="s">
			var result = xs.infoset.scan.call(this, s);
			if (result && result.element)
				result.element.document = result;	
			return result;
		</function>
		<function name="serialize" params="document">
			document.normalizeNS();
			return xs.infoset.print.call(this, document);
		</function>
			
		<object name="node" script="false">
			<number name="ELEMENT_NODE" value="1"/>
			<number name="ATTRIBUTE_NODE" value="2"/>
			<number name="TEXT_NODE" value="3"/>
			<number name="CDATA_SECTION_NODE" value="4"/>
			<number name="ENTITY_REFERENCE_NODE" value="5"/>
			<number name="ENTITY_NODE" value="6"/>
			<number name="PROCESSING_INSTRUCTION_NODE" value="7"/>
			<number name="COMMENT_NODE" value="8"/>
			<number name="DOCUMENT_NODE" value="9"/>
			<number name="DOCUMENT_TYPE_NODE" value="10"/>
			<number name="DOCUMENT_FRAGMENT_NODE" value="11"/>
			<number name="NOTATION_NODE" value="12"/>
		
			<number name="changes" script="false"/>
			<array name="children" contents="DOM.node" script="false"/>
			<null name="document" script="false"/>
			<boolean name="hierarchical" script="false"/>
			<null name="name" script="false"/>
			<null name="namespace" script="false"/>
			<null name="parent" script="false"/>
			<null name="prefix"/>
			<null name="type" script="false"/>
			<null name="value" script="false"/>
			
			<function name="get attributes">
				return null;
			</function>
			<function name="get childNodes">
				return new DOM.NodeList(this);
			</function>
			<function name="get firstChild">
				var children = this.children;
				var c = children.length;
				if (c)
					return children[0];
				return null;
			</function>
			<function name="get lastChild">
				var children = this.children;
				var c = children.length;
				if (c)
					return children[c - 1]
				return null;
			</function>
			<function name="get localName">
				return this.name;
			</function>
			<function name="get namespaceURI">
				return this.namespace;
			</function>
			<function name="get nextSibling">
				var parent = this.parent;
				if (parent) {
					var children = parent.children;
					var c = children.length;
					for (var i = 0; i < c; i++) {
						if (this == children[i]) {
							i++;
							if (i < c)
								return children[i];
							break;
						}
					}
				}
				return null;
			</function>
			<function name="get nodeName">
				if (this.prefix)
					return this.prefix + ":" + this.name;
				return this.name;
			</function>
			<function name="get nodeType">
				return this.type;
			</function>
			<function name="get nodeValue">
				return this.value;
			</function>
			<function name="get ownerDocument">
				var node = this;
				while (node) {
					var document = node.document;
					if (document)
						return document;
					node = node.parent;
				}
				return null;
			</function>
			<function name="get parentNode">
				return this.parent;
			</function>
			<!--function name="get prefix">
				return this.prefix;
			</function-->
			<function name="get previousSibling">
				var parent = this.parent;
				if (parent) {
					var children = this.children;
					var c = children.length;
					for (var i = 0; i < c; i++) {
						if (child == children[i]) {
							i--;
							if (i >= 0)
								return children[i];
							break;
						}
					}
				}
				return null;
			</function>
			
			<function name="set attributes" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set childNodes" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set firstChild" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set lastChild" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set localName" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set namespaceURI" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set nodeName" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set nodeType" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set nodeValue" params="it">
				if (!this.hierarchical)
					this.value = it.toString();
			</function>
			<function name="set nextSibling" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set ownerDocument" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set parentNode" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<!--function name="set prefix" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function-->
			<function name="set previousSibling" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			
			<function name="appendChild" params="newChild">
				this.insertBefore(newChild, null);
			</function>
			<function name="checkChild" params="newChild" script="false">
				if (!this.hierarchical)
					throw new DOM.Exception(DOM.exception.HIERARCHY_REQUEST_ERR);
				if (xs.isInstanceOf(newChild, DOM.attribute))
					throw new DOM.Exception(DOM.exception.HIERARCHY_REQUEST_ERR);
				if (xs.isInstanceOf(newChild, DOM.document))
					throw new DOM.Exception(DOM.exception.HIERARCHY_REQUEST_ERR);
				var parent = this.parent;
				while (parent) {
					if (newChild == parent)
						throw new DOM.Exception(DOM.exception.HIERARCHY_REQUEST_ERR);
					parent = parent.parent;
				}
				if (newChild.ownerDocument != this.ownerDocument)
					throw new DOM.Exception(DOM.exception.WRONG_DOCUMENT_ERR);
			</function>
			<function name="cloneNode">
				throw new DOM.Exception(DOM.exception.NOT_SUPPORTED_ERR);
			</function>
			<function name="hasAttributes">
				return false;
			</function>
			<function name="hasChildNodes">
				return this.children.length > 0;
			</function>
			<function name="insertBefore" params="newChild, refChild">
				this.checkChild(newChild);
				var children = this.children;
				var c = children.length, i = 0, j = 0;
				if (refChild) {
					for (; i < c; i++) {
						if (refChild == children[i])
							break;
						if (newChild == children[i])
							j = 1;
					}
					if (i == c)
						throw new DOM.Exception(DOM.exception.NOT_FOUND_ERR);
				}
				else {
					if (c)
						i = c;
					else
						this.children = children = [];
				}
				var flag = false;
				if (newChild.nodeType == this.DOCUMENT_FRAGMENT_NODE) {
					var newChildren = newChild.children;
					this.children = children.slice(0, i).concat(newChildren, children.slice(i));
					newChild.children = [];
					var c = newChildren.length;
					for (var i = 0; i < c; i++) {
						newChild = newChildren[i];
						newChild.parent = this;
						flag |= (newChild.nodeType == this.ELEMENT_NODE);
					}
				}
				else {
					var parent = newChild.parent;
					if (parent)
						parent.removeChild(newChild);
					children.splice(i - j, 0, newChild);
					newChild.parent = this;	
					delete newChild.document;
					flag |= (newChild.nodeType == this.ELEMENT_NODE);
				}
				if (flag)
					this.invalidate();
			</function>
			<function name="invalidate" script="false">
				this.changes++;
				var parent = this.parent;
				while (parent) {
					parent.changes++;
					parent = parent.parent;
				}
			</function>
			<function name="isSupported" params="feature, version">
				return ((feature == "Core") && ((version == "1.0") || (version == "2.0")));
			</function>
			<function name="normalize">
				var children = this.children;
				var c = children.length;
				var document = this.ownerDocument;
				var oldChild = null;
				for (var i = 0; i < c; i++) {
					var child = children[i];
					if (xs.isInstanceOf(child, DOM.cdata)) {
						if (oldChild) {
							var newChild = document.createTextNode(oldChild.value + child.value);
							oldChild.document = document;	
							oldChild.parent = null;
							child.document = document;	
							child.parent = null;
							children.splice(i - 1, 2, newChild);
							newChild.parent = this;	
							delete newChild.document;
							oldChild = newChild;
							c--;
							i--;
						}
						else {
							if (child.value.length)
								oldChild = child;
							else {
								child.document = document;	
								child.parent = null;	
								children.splice(i, 1);
								c--;
								i--;
							}
						}
					}
					else {
						oldChild = null;
						if (xs.isInstanceOf(child, DOM.element))
							child.normalize();
					}
				}
			</function>
			<function name="normalizeNamespaces" params="link" script="false"/>
			<function name="removeChild" params="oldChild">
				var children = this.children;
				var c = children.length;
				for (var i = 0; i < c; i++) {
					if (oldChild == children[i]) {
						break;
					}
				}
				if (i == c)
					throw new DOM.Exception(DOM.exception.NOT_FOUND_ERR);
				oldChild.document = this.ownerDocument;	
				oldChild.parent = null;	
				children.splice(i, 1);
				if (oldChild.nodeType == this.ELEMENT_NODE)
					this.invalidate();
			</function>
			<function name="replaceChild" params="newChild, oldChild">
				this.checkChild(newChild);
				var children = this.children;
				var c = children.length;
				for (var i = 0; i < c; i++) {
					if (oldChild == children[i]) {
						break;
					}
				}
				if (i == c)
					throw new DOM.Exception(DOM.exception.NOT_FOUND_ERR);
				var flag = (oldChild.nodeType == this.ELEMENT_NODE);
				oldChild.document = this.ownerDocument;	
				oldChild.parent = null;	
				if (newChild.nodeType == this.DOCUMENT_FRAGMENT_NODE) {
					var newChildren = newChild.children;
					this.children = children.slice(0, i).concat(newChildren, children.slice(i + 1));
					newChild.children = [];
					var c = newChildren.length;
					for (var i = 0; i < c; i++) {
						newChild = newChildren[i];
						newChild.parent = this;
						flag |= (newChild.nodeType == this.ELEMENT_NODE);
					}
				}
				else {
					children[i] = newChild;
					newChild.parent = this;	
					delete newChild.document;
					flag |= (newChild.nodeType == this.ELEMENT_NODE);
				}
				if (flag)
					this.invalidate();
			</function>
		</object>

		<object name="attribute" prototype="DOM.node" script="false">
			<null name="name"/>
			<number name="type" value="2" script="false"/>
			<null name="value"/>

			<!--function name="get name">
				return this.nodeName;
			</function-->
			<function name="get ownerElement">
				return this.parent;
			</function>
			<function name="get parentNode">
				return null;
			</function>
			<function name="get specified">
				return true;
			</function>
			<!--function name="get value">
				return this.value;
			</function-->
			
			<!--function name="set name" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function-->
			<function name="set ownerElement" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set parentNode" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set specified" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<!--function name="set value" params="it">
				this.value = it;
			</function-->
		</object>
		
		<object name="characterData" prototype="DOM.node" script="false">
			<null name="value"/> <!-- should not exist -->

			<function name="get data">
				return this.value;
			</function>
			<function name="get length">
				return this.value.length;
			</function>
			
			<function name="set data" params="it">
				this.value = it.toString();
			</function>
			<function name="set length" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			
			<function name="appendData" params="arg">
				return this.value += arg;
			</function>
			<function name="deleteData" params="offset, count">
				var value = this.value;
				if ((offset < 0) || (value.length < offset) || (count < 0))
					throw new DOM.Exception(DOM.exception.INDEX_SIZE_ERR);
				return this.value = value.substring(0, offset) + value.substring(offset + count)
			</function>
			<function name="insertData" params="offset, arg">
				var value = this.value;
				if ((offset < 0) || (value.length < offset))
					throw new DOM.Exception(DOM.exception.INDEX_SIZE_ERR);
				return this.value = value.substring(0, offset) + arg + value.substring(offset)
			</function>
			<function name="replaceData" params="offset, count, arg">
				var value = this.value;
				if ((offset < 0) || (value.length < offset) || (count < 0))
					throw new DOM.Exception(DOM.exception.INDEX_SIZE_ERR);
				return this.value = value.substring(0, offset) + arg + value.substring(offset + count)
			</function>
			<function name="substringData" params="offset, count">
				var value = this.value;
				if ((offset < 0) || (value.length < offset) || (count < 0))
					throw new DOM.Exception(DOM.exception.INDEX_SIZE_ERR);
				return this.value = value.substring(offset, offset + count);
			</function>
		</object>
			
		<object name="cdata" prototype="DOM.characterData" script="false">
			<string name="name" value="#text" script="false"/>
			<number name="type" value="3" script="false"/>

			<function name="splitText" params="offset">
				var value = this.value;
				var parent = this.parent;
				if ((offset < 0) || (value.length < offset))
					throw new DOM.Exception(DOM.exception.INDEX_SIZE_ERR);
				if (!parent)
					throw new DOM.Exception(DOM.exception.HIERARCHY_REQUEST_ERR);
				var node = this.ownerDocument.createTextNode(value.substring(0, offset));
				this.parent.insertBefore(node, this);
				this.value = value.substring(offset)
			</function>
		</object>
		
		<object name="comment" prototype="DOM.characterData" script="false">
			<string name="name" value="#text" script="false"/>
			<number name="type" value="8" script="false"/>
		</object>
		
		<object name="document" prototype="DOM.node" script="false">
			<string name="name" value="#document" script="false"/>
			<number name="type" value="9" script="false"/>
			<null name="element" script="false"/>
			<null name="encoding" script="false"/>
			<boolean name="hierarchical" value="true" script="false"/>
			<null name="version" script="false"/>
			
			<function name="get doctype">
				return null;
			</function>
			<function name="get documentElement">
				return this.element;
			</function>
			<function name="get implementation">
				return DOM.implementation;
			</function>

			<function name="set doctype">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set documentElement">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set implementation">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>

			<function name="checkName" params="name" script="false">
				if (!name)
					throw new DOM.Exception(DOM.exception.NAMESPACE_ERR);
				// check [A-Za-z_][0-9A-Za-z_\.\-]+
				return name;
			</function>
			<function name="createAttribute" params="name">
				var attribute = xs.newInstanceOf(DOM.attribute);
				attribute.document = this;
				var a = name.split(":");
				if (a.length == 2) {
					attribute.name = this.checkName(a[1]);
					attribute.prefix = this.checkName(a[0]);
				}
				else
					attribute.name = this.checkName(name);
				return attribute;
			</function>
			<function name="createAttributeNS" params="namespace, name">
				var attribute = xs.newInstanceOf(DOM.attribute);
				attribute.document = this;
				var a = name.split(":");
				if (a.length == 2) {
					attribute.name = this.checkName(a[1]);
					attribute.namespace = namespace;
					attribute.prefix = this.checkName(a[0]);
				}
				else {
					attribute.name = this.checkName(name);
					attribute.namespace = namespace;
				}
				DOM.checkAttributeNS(attribute);
				return attribute;
			</function>
			<function name="createCDATASection" params="data">
				var cdata = xs.newInstanceOf(DOM.cdata);
				cdata.document = this;
				cdata.value = data.toString();
				return cdata;
			</function>
			<function name="createComment" params="data">
				var comment = xs.newInstanceOf(DOM.comment);
				comment.document = this;
				comment.value = data.toString();
				return comment;
			</function>
			<function name="createDocumentFragment">
				var documentFragment = xs.newInstanceOf(DOM.documentFragment);
				documentFragment.document = this;
				return documentFragment;
			</function>
			<function name="createElement" params="name">
				var element = xs.newInstanceOf(DOM.element);
				element.document = this;
				var a = name.split(":");
				if (a.length == 2) {
					element.name = this.checkName(a[1]);
					element.prefix = this.checkName(a[0]);
				}
				else
					element.name = this.checkName(name);
				return element;
			</function>
			<function name="createElementNS" params="namespace, name">
				var element = xs.newInstanceOf(DOM.element);
				element.document = this;
				var a = name.split(":");
				if (a.length == 2) {
					element.name = this.checkName(a[1]);
					element.namespace = namespace;
					element.prefix = this.checkName(a[0]);
				}
				else {
					element.name = this.checkName(name);
					element.namespace = namespace;
				}
				DOM.checkElementNS(element);
				return element;
			</function>
			<function name="createEntityReference" params="name">
				var data;
				switch(name) {
				case "amp": data = "&"; break;
				case "apos": data = "'"; break;
				case "gt": data = ">"; break;
				case "lt": data = "<"; break;
				case "quot": data = '"'; break;
				default:
					throw new DOM.Exception(DOM.exception.NOT_SUPPORTED_ERR);
				}
				var cdata = xs.newInstanceOf(DOM.cdata);
				cdata.document = this;
				cdata.value = data;
				return cdata;
			</function>
			<function name="createProcessingInstruction" params="target, data">
				var pi = xs.newInstanceOf(DOM.pi);
				pi.document = this;
				pi.name = target;
				pi.value = data.toString();
				return pi;
			</function>
			<function name="createTextNode" params="data">
				var cdata = xs.newInstanceOf(DOM.cdata);
				cdata.document = this;
				cdata.value = data.toString();
				return cdata;
			</function>
			<function name="getElementsByTagName" params="name">
				return new DOM.ElementList(this, name);
			</function>
			<function name="getElementsByTagNameNS" params="namespace, name">
				var a = name.split(":");
				var name = (a.length == 2) ? a[1] : name;
				return new DOM.ElementListNS(this, namespace, name);
			</function>
			<function name="getElementsByID" params="id">
				return null;
			</function>
			<function name="importNode" params="node, deep">
				throw new DOM.Exception(DOM.exception.NOT_SUPPORTED_ERR);
			</function>
			<function name="normalizeNS">
				this.normalize();
				this.element.normalizeNamespaces();
			</function>
		</object>
		
		<object name="documentFragment" prototype="DOM.node" script="false">
			<string name="name" value="#document-fragment" script="false"/>
			<number name="type" value="11" script="false"/>
			<boolean name="hierarchical" value="true" script="false"/>
		</object>
		
		<object name="element" prototype="DOM.node" script="false">
			<array name="_attributes" contents="DOM.attribute" script="false"/>
			<number name="type" value="1" script="false"/>
			<boolean name="hierarchical" value="true" script="false"/>
			<array name="xmlnsAttributes" contents="DOM.attribute" script="false"/>
			
			<function name="get attributes">
				return new DOM.NamedNodeMap(this);
			</function>
			<function name="get tagName">
				return this.nodeName;
			</function>

			<function name="set attributes" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set tagName" params="it">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			
			<function name="getAttribute" params="name">
				var attribute = this.getAttributeNode(name);
				if (attribute)
					return attribute.value;
			</function>
			<function name="getAttributeNS" params="namespace, name">
				var attribute = this.getAttributeNodeNS(namespace, name);
				if (attribute)
					return attribute.value;
			</function>
			<function name="getAttributeNode" params="name">
				var prefix = null;
				var a = name.split(":");
				if (a.length == 2) {
					name = a[1];
					prefix = a[0];
				}
				var attributes = this._attributes;
				var c = attributes.length;
				for (var i = 0; i < c; i++) {
					var attribute = attributes[i];
					if ((attribute.name == name) && (attribute.prefix == prefix))
						return attribute;
				}
			</function>
			<function name="getAttributeNodeNS" params="namespace, name">
				var a = name.split(":");
				var name = (a.length == 2) ? a[1] : name;
				var attributes = (DOM.xmlnsNamespace == namespace) ? this.xmlnsAttributes : this._attributes;
				var c = attributes.length;
				for (var i = 0; i < c; i++) {
					var attribute = attributes[i];
					if ((attribute.name == name) && (attribute.namespace == namespace))
						return attribute;
				}
			</function>
			<function name="getElementsByTagName" params="name">
				return new DOM.ElementList(this, name);
			</function>
			<function name="getElementsByTagNameNS" params="namespace, name">
				var a = name.split(":");
				var name = (a.length == 2) ? a[1] : name;
				return new DOM.ElementListNS(this, namespace, name);
			</function>
			<function name="hasAttribute" params="name">
				var attribute = this.getAttributeNode(name);
				return (attribute) ? true : false;
			</function>
			<function name="hasAttributeNS" params="namespace, name">
				var attribute = this.getAttributeNodeNS(namespace, name);
				return (attribute) ? true : false;
			</function>
			<function name="hasAttributes">
				return (this._attributes.length > 0) || (this.xmlnsAttributes.length > 0);
			</function>
			<function name="normalizeNamespaces" params="link" script="false">
				var attributes = this.xmlnsAttributes;
				var c = attributes.length;
				for (var i = 0; i < c; i++) {
					var attribute = attributes[i];
					DOM.checkAttributeNS(attribute);
					var prefix = attribute.name;
					if (prefix == DOM.xmlnsPrefix)
						prefix = null;
					link = DOM.setScope(link, attribute.value, prefix);
				}
				
				DOM.checkElementNS(this);
				var namespace = this.namespace;
				if (namespace) {
					var prefix = this.prefix;
					var scope = DOM.getScope(link, namespace, false);
					if (scope)
						this.prefix = scope.prefix;
					else {
						var name = prefix ? DOM.xmlnsPrefix + ":" + prefix : DOM.xmlnsPrefix;
						this.setAttributeNS(DOM.xmlnsNamespace, name, namespace);
						link = DOM.setScope(link, namespace, prefix);
					}
				}
				else {
					var scope = DOM.getDefaultScope(link);
					if (scope)
						throw new DOM.Exception(DOM.exception.NAMESPACE_ERR);
				}
				
				var attributes = this._attributes;
				var c = attributes.length;
				for (var i = 0; i < c; i++) {
					var attribute = attributes[i];
					DOM.checkAttributeNS(attribute);
					var namespace = attribute.namespace;
					if (namespace) {
						var prefix = attribute.prefix;
						var scope = DOM.getScope(link, namespace, true);
						if (scope)
							attribute.prefix = scope.prefix;
						else {
							if (!prefix)
								prefix = attribute.prefix = DOM.buildPrefix(link);
							var name = DOM.xmlnsPrefix + ":" + prefix;
							this.setAttributeNS(DOM.xmlnsNamespace, name, namespace);
							link = DOM.setScope(link, namespace, prefix);
						}
					}
				}
				
				var children = this.children;
				var c = children.length;
				for (var i = 0; i < c; i++) {
					children[i].normalizeNamespaces(link);
				}
			</function>
			<function name="removeAttribute" params="name">
				var prefix = null;
				var a = name.split(":");
				if (a.length == 2) {
					name = a[1];
					prefix = a[0];
				}
				var attributes = this._attributes;
				var c = attributes.length;
				for (var i = 0; i < c; i++) {
					var oldAttr = attributes[i];
					if ((oldAttr.name == name) && (oldAttr.prefix == prefix))
						break;
				}
				if (i < c) {
					oldAttr.document = this.ownerDocument;
					oldAttr.parent = null;
					attributes.slice(i, 1);
				}
			</function>
			<function name="removeAttributeNS" params="namespace, name">
				var a = name.split(":");
				var name = (a.length == 2) ? a[1] : name;
				var attributes = (DOM.xmlnsNamespace == namespace) ? this.xmlnsAttributes : this._attributes;
				var c = attributes.length;
				for (var i = 0; i < c; i++) {
					var oldAttr = attributes[i];
					if ((oldAttr.namespace == namespace) && (oldAttr.name == name))
						break;
				}
				if (i < c) {
					oldAttr.document = this.ownerDocument;
					oldAttr.parent = null;
					attributes.slice(i, 1);
				}
			</function>
			<function name="removeAttributeNode" params="oldAttr">
				var attributes = this._attributes;
				var c = attributes.length;
				for (var i = 0; i < c; i++)
					if (attributes[i] == oldAttr)
						break;
				if (i == c) 
					throw new DOM.Exception(DOM.exception.NOT_FOUND_ERR);
				oldAttr.document = this.ownerDocument;
				oldAttr.parent = null;
				attributes.slice(i, 1);
			</function>
			<function name="removeAttributeNodeNS" params="oldAttr">
				var attributes = (DOM.xmlnsNamespace == oldAttr.namespace) ? this.xmlnsAttributes : this._attributes;
				var c = attributes.length;
				for (var i = 0; i < c; i++)
					if (attributes[i] == oldAttr)
						break;
				if (i == c) 
					throw new DOM.Exception(DOM.exception.NOT_FOUND_ERR);
				oldAttr.document = this.ownerDocument;
				oldAttr.parent = null;
				attributes.slice(i, 1);
			</function>
			<function name="setAttribute" params="name, value">
				var attribute = this.getAttributeNode(name);
				if (attribute)
					attribute.value = value.toString();
				else {
					var document = this.ownerDocument;
					var attribute = document.createAttribute(name);
					attribute.value = value.toString();
					if (this._attributes.length)
						this._attributes.push(attribute);
					else
						this._attributes = [ attribute ];
					attribute.parent = this;
					delete attribute.document;
				}
			</function>
			<function name="setAttributeNS" params="namespace, name, value">
				var attribute = this.getAttributeNodeNS(namespace, name);
				if (attribute)
					attribute.value = value.toString();
				else {
					var document = this.ownerDocument;
					var newAttr = document.createAttributeNS(namespace, name);
					newAttr.value = value.toString();
					var flag = (DOM.xmlnsNamespace == newAttr.namespace);
					var attributes = (flag) ? this.xmlnsAttributes : this._attributes;
					var c = attributes.length;
					if (c)
						attributes.push(newAttr);
					else if (flag)
						this.xmlnsAttributes = [ newAttr ];
					else
						this._attributes = [ newAttr ];
					newAttr.parent = this;
					delete newAttr.document;
				}
			</function>
			<function name="setAttributeNode" params="newAttr">
				if (newAttr.nodeType != this.ATTRIBUTE_NODE)
					throw new DOM.Exception(DOM.exception.HIERARCHY_REQUEST_ERR);
				if (newAttr.ownerDocument != this.ownerDocument)
					throw new DOM.Exception(DOM.exception.WRONG_DOCUMENT_ERR);
				if (newAttr.parent)
					throw new DOM.Exception(DOM.exception.INUSE_ATTRIBUTE_ERR);
				var attributes = this._attributes;
				var c = attributes.length;
				for (var i = 0; i < c; i++) {
					var oldAttr = attributes[i];
					if (oldAttr.name == newAttr.name)
						break;
				}
				if (i == c) {
					if (c)
						attributes.push(newAttr);
					else
						this._attributes = [ newAttr ];
				}
				else {
					oldAttr.document = this.ownerDocument;
					oldAttr.parent = null;
					attributes[i] = newAttr;
				}
				newAttr.parent = this;
				delete newAttr.document;
			</function>
			<function name="setAttributeNodeNS" params="newAttr">
				if (newAttr.nodeType != this.ATTRIBUTE_NODE)
					throw new DOM.Exception(DOM.exception.HIERARCHY_REQUEST_ERR);
				if (newAttr.ownerDocument != this.ownerDocument)
					throw new DOM.Exception(DOM.exception.WRONG_DOCUMENT_ERR);
				if (newAttr.parent)
					throw new DOM.Exception(DOM.exception.INUSE_ATTRIBUTE_ERR);
				var flag = (DOM.xmlnsNamespace == newAttr.namespace);
				var attributes = (flag) ? this.xmlnsAttributes : this._attributes;
				var c = attributes.length;
				for (var i = 0; i < c; i++) {
					var oldAttr = attributes[i];
					if ((oldAttr.namespace == newAttr.namespace) && (oldAttr.name == newAttr.name))
						break;
				}
				if (i == c) {
					if (c)
						attributes.push(newAttr);
					else if (flag)
						this.xmlnsAttributes = [ newAttr ];
					else
						this._attributes = [ newAttr ];
				}
				else {
					oldAttr.document = this.ownerDocument;
					oldAttr.parent = null;
					attributes[i] = newAttr;
				}
				newAttr.parent = this;
				delete newAttr.document;
			</function>
			<!-- NOT DOM -->
			<function name="getElement" params="name">
				var child = this.getElementNode(name);
				if (child) {
					var child = child.firstChild;
					if (child)
						return child.value;
				}
			</function>
			<function name="getElementNS" params="namespace, name">
				var child = this.getElementNodeNS(namespace, name);
				if (child) {
					var child = child.firstChild;
					if (child)
						return child.value;
				}
			</function>
			<function name="getElementNode" params="name">
				var prefix = null;
				var a = name.split(":");
				if (a.length == 2) {
					name = a[1];
					prefix = a[0];
				}
				var children = this.children;
				var c = children.length;
				for (var i = 0; i < c; i++) {
					var child = children[i];
					if (xs.isInstanceOf(child, DOM.element)) {
						if ((child.name == name) && (child.prefix == prefix))
							return child;
					}
				}
			</function>
			<function name="getElementNodeNS" params="namespace, name">
				var a = name.split(":");
				var name = (a.length == 2) ? a[1] : name;
				var children = this.children;
				var c = children.length;
				for (var i = 0; i < c; i++) {
					var child = children[i];
					if (xs.isInstanceOf(child, DOM.element)) {
						if ((child.namespace == namespace) && (child.name == name))
							return child;
					}
				}
			</function>
		</object>
		
		<object name="pi" prototype="DOM.node">
			<number name="type" value="7" script="false"/>

			<function name="get target">
				return this.nodeName;
			</function>
			<function name="get data">
				return this.value;
			</function>
			
			<function name="set target">
				throw new DOM.Exception(DOM.exception.NO_MODIFICATION_ALLOWED_ERR);
			</function>
			<function name="set data">
				this.value = it;
			</function>
		</object>
		
		<object name="elementLink" script="false">
			<null name="element" script="false"/>
			<null name="nextLink" script="false"/>
		</object>
		
		<object name="elementList" script="false">
			<number name="cacheIndex" script="false"/>
			<null name="cacheLink" script="false"/>
			<null name="changes" script="false"/>
			<null name="element" script="false"/>
			<null name="firstLink" script="false"/>
			<null name="lastLink" script="false"/>
			<number name="_length" script="false"/>
			<null name="name" script="false"/>
			
			<function name="get length">
				this.update();
				return this._length;
			</function>
			<function name="append" params="child">
				var link = xs.newInstanceOf(DOM.elementLink);
				link.element = child;
				if (this.lastLink)
					this.lastLink.nextLink = link;
				else
					this.firstLink = link;
				this.lastLink = link;
				this._length++;
			</function>
			<function name="item" params="index">
				this.update();
				if ((index < 0) || (this._length <= index))
					return null;
				var cacheIndex = this.cacheIndex;
				var cacheLink = this.cacheLink;
				if (index < cacheIndex) {
					cacheIndex = 0;
					cacheLink = this.firstLink;
				}
				while (cacheIndex < index) {
					cacheIndex++;
					cacheLink = cacheLink.nextLink;
				}
				this.cacheIndex = cacheIndex;
				this.cacheLink = cacheLink;
				return cacheLink.element;
			</function>
			<function name="update" script="false">
				var element = this.element;
				if (this.changes != element.changes) {
					this.firstLink = null
					this.lastLink = null
					this._length = 0;
					this.updateAux(element, this.prefix, this.name);
					this.cacheIndex = 0;
					this.cacheLink = this.firstLink;
					this.changes = element.changes;
				}
			</function>
			<function name="updateAux" params="element, prefix, name" script="false">
				var children = element.children;
				var c = children.length;
				if (name == "*") {
					for (var i = 0; i < c; i++) {
						var child = children[i];
						if (xs.isInstanceOf(child, DOM.element)) {
							this.append(child);
							this.updateAux(child, prefix, name);
						}
					}
				}
				else {
					for (var i = 0; i < c; i++) {
						var child = children[i];
						if (xs.isInstanceOf(child, DOM.element)) {
							if ((child.name == name) && (child.prefix == prefix))
								this.append(child);
							this.updateAux(child, prefix, name);
						}
					}
				}
			</function>
		</object>
		<function name="ElementList" params="element, name" prototype="DOM.elementList" script="false">
			var prefix = null;
			var a = name.split(":");
			if (a.length == 2) {
				name = a[1];
				prefix = a[0];
			}
			this.changes = -1;
			this.element = element;
			this.name = name;
			this.prefix = prefix;
			this.update();
		</function>
		
		<object name="elementListNS" prototype="DOM.elementList" script="false">
			<null name="namespace" script="false"/>
			
			<function name="update" script="false">
				var element = this.element;
				if (this.changes != element.changes) {
					this.firstLink = null
					this.lastLink = null
					this._length = 0;
					this.updateAux(element, this.namespace, this.name);
					this.cacheIndex = 0;
					this.cacheLink = this.firstLink;
					this.changes = element.changes;
				}
			</function>
			<function name="updateAux" params="element, namespace, name" script="false">
				var children = element.children;
				var c = children.length;
				if (namespace == "*") {
					for (var i = 0; i < c; i++) {
						var child = children[i];
						if (xs.isInstanceOf(child, DOM.element)) {
							if (child.name == name)
								this.append(child);
							this.updateAux(child, namespace, name);
						}
					}
				}
				else if (name == "*") {
					for (var i = 0; i < c; i++) {
						var child = children[i];
						if (xs.isInstanceOf(child, DOM.element)) {
							if (child.namespace == namespace)
								this.append(child);
							this.updateAux(child, namespace, name);
						}
					}
				}
				else {
					for (var i = 0; i < c; i++) {
						var child = children[i];
						if (xs.isInstanceOf(child, DOM.element)) {
							if ((child.namespace == namespace) && (child.name == name))
								this.append(child);
							this.updateAux(child, namespace, name);
						}
					}
				}
			</function>
		</object>
		<function name="ElementListNS" params="element, namespace, name" prototype="DOM.elementListNS" script="false">
			this.changes = -1;
			this.element = element;
			this.namespace = namespace;
			this.name = name;
			this.update();
		</function>
		
		<object name="exception" prototype="Error.prototype" script="false">
			<number name="INDEX_SIZE_ERR" value="1"/>
			<number name="DOMSTRING_SIZE_ERR" value="2"/>
			<number name="HIERARCHY_REQUEST_ERR" value="3"/>
			<number name="WRONG_DOCUMENT_ERR" value="4"/>
			<number name="INVALID_CHARACTER_ERR" value="5"/>
			<number name="NO_DATA_ALLOWED_ERR" value="6"/>
			<number name="NO_MODIFICATION_ALLOWED_ERR" value="7"/>
			<number name="NOT_FOUND_ERR" value="8"/>
			<number name="NOT_SUPPORTED_ERR" value="9"/>
			<number name="INUSE_ATTRIBUTE_ERR" value="10"/>
			<number name="NVALID_STATE_ERR" value="11"/>
			<number name="SYNTAX_ERR" value="12"/>
			<number name="INVALID_MODIFICATION_ERR" value="13"/>
			<number name="NAMESPACE_ERR" value="14"/>
			<number name="INVALID_STATE_ERR" value="15"/>
			<number name="code"/>
		</object>
		<function name="Exception" params="code" prototype="DOM.exception" script="false">
			this.code = code;
		</function>
	
		<object name="namedNodeMap" script="false">
			<null name="node" script="false"/>
			
			<function name="get length">
				var node = this.node;
				return node.xmlnsAttributes.length + node._attributes.length;
			</function>
			<function name="getNamedItem" params="name">
				return this.node.getAttribute(name);
			</function>
			<function name="getNamedItemNS" params="namespace, name">
				return this.node.getAttributeNS(namespace, name);
			</function>
			<function name="item" params="index">
				var node = this.node;
				var nodes = node.xmlnsAttributes;
				var c = nodes.length;
				if ((0 <= index) && (index < c))
					return nodes[index];
				index -= c;
				nodes = node._attributes;
				c = nodes.length;
				if ((0 <= index) && (index < c))
					return nodes[index];
				return null;
			</function>
			<function name="setNamedItem" params="node">
				this.node.setAttributeNode(node);
			</function>
			<function name="setNamedItemNS" params="node">
				this.node.setAttributeNodeNS(node);
			</function>
			<function name="removeNamedItem" params="name">
				this.node.removeAttribute(name);
			</function>
			<function name="removeNamedItemNS" params="namespace, name">
				this.node.removeAttributeNS(namespace, name);
			</function>
		</object>
		<function name="NamedNodeMap" params="node" prototype="DOM.namedNodeMap" script="false">
			this.node = node;
		</function>
		
		<object name="nodeList" script="false">
			<null name="node" script="false"/>
			
			<function name="get length">
				return this.node.children.length;
			</function>
			<function name="item" params="index">
				var nodes = this.node.children;
				if ((0 <= index) && (index < nodes.length))
					return nodes[index];
				return null;
			</function>
		</object>
		<function name="NodeList" params="node" prototype="DOM.nodeList" script="false">
			this.node = node;
		</function>
		
		<object name="implementation" script="true">
			<function name="createDocument" params="namespace, name, documentType">
				if (documentType)
					throw new DOM.Exception(DOM.exception.NOT_SUPPORTED_ERR);
				var document = xs.newInstanceOf(DOM.document);
				var element = document.createElementNS(namespace, name);
				document.children = [ element ];
				element.parent = document;	
				document.element = element;
				return document;
			</function>
			<function name="createDocumentType" params="name, publicID, systemID">
				throw new DOM.Exception(DOM.exception.NOT_SUPPORTED_ERR);
			</function>
			<function name="hasFeature" params="feature, version">
				return ((feature == "Core") && ((version == "1.0") || (version == "2.0")));
			</function>
		</object>
		
		<object name="scope" script="false">
			<undefined name="link"/>
			<null name="namespace" script="false"/>
			<null name="prefix"/>
			
			<function name="find" params="namepace">
				var scope = this;
				while (scope) {
					if (scope.namespace == namespace)
						return scope;
					scope = scope.link;
				}
			</function>
		</object>
		<function name="getDefaultScope" params="link" script="false">
			while (link) {
				if (link.prefix == null)
					return link;
				link = link.link;
			}
		</function>
		<function name="getScope" params="link, namespace, skip" script="false">
			while (link) {
				if ((link.namespace == namespace) && (!skip || link.prefix))
					return link;
				link = link.link;
			}
		</function>
		<function name="setScope" params="link, namespace, prefix" script="false">
			var scope = xs.newInstanceOf(DOM.scope);
			scope.link = link;
			scope.namespace = namespace;
			scope.prefix = prefix;
			return scope;
		</function>
		<function name="buildPrefix" params="link" script="false">
			for (var i = 1; ; i++) {
				var prefix = "NS" + i;
				var scope = link;
				while (scope && (scope.prefix != prefix))
					scope = scope.link;
				if (!scope)
					return prefix;
			}
		</function>
	
		<function name="parseBinary" c="KPR_DOM_parseBinary" params="chunk, codeSpaces"/>
		<function name="reversePages" params="pages, space" script="false">
			if (pages) {
				var c = pages.length;
				if (c) {
					for (var i = c - 1; i >= 0; i--) {
						var page = pages[i];
						var namespace = page.sandbox.namespace;
						var result = {
							$namespace: namespace,
							$code: i,
						};
						space[namespace] = result;
						this.reverseTokens(page.sandbox.tokens, result);
					}
					space.$current = result;
				}
			}
		</function>
		<function name="reverseTokens" params="tokens, result" script="false">
			if (tokens) {
				var c = tokens.length;
				for (var i = c - 1; i >= 0; i--) {
					result[tokens[i]] = i;
				}
			}
			return result;
		</function>
		<function name="serializeBinary" params="document, codeSpaces">
			document.normalizeNS();
			var stream = [];
			var tagSpace = {
				$current: {
					$namespace: undefined,
					$code: -1,
				}
			};
			var attributeSpace = {
				$current: {
					$namespace: undefined,
					$code: -1,
				}
			};
			if (codeSpaces) {
				this.reversePages(codeSpaces.sandbox.tagTokenPages, tagSpace);
				this.reversePages(codeSpaces.sandbox.attributeStartTokenPages, attributeSpace);
			}
			stream.push(0x03);	// ver 1.3
			stream.push(0x01);	// unknown public id
			stream.push(0x6a); // charset=UTF-8
			stream.push(0);    // no string table
			this.serializeBinaryElement(stream, tagSpace, attributeSpace, document.element);
			return this.serializeBinaryAux(stream);
		</function>
		<function name="serializeBinaryAux" c="KPR_DOM_serializeBinaryAux" script="false"/>
		<function name="serializeBinaryElement" params="stream, tagSpace, attributeSpace, element" script="false">
			var namespace = element.namespace;
			if (namespace && (namespace != tagSpace.$current.$namespace)) {
				if (namespace in tagSpace) {
					tagSpace.$current = tagSpace[namespace];
					stream.push(0); // SWITCH
					stream.push(tagSpace.$current.$code);
				}
				else
					throw new Error("WBXML: no tag namespace: " + namespace);
			}
			var name = element.name;
			if (name in tagSpace.$current)
				var token = tagSpace.$current[name];
			else
				throw new Error("WBXML: no tag name: " + name);
			var attributes = element._attributes;
			var attributesCount = attributes.length;
			var children = element.children;
			var childrenCount = children.length;
			if (attributesCount) token |= 0x80;
			if (childrenCount) token |= 0x40;
			stream.push(token);
			if (attributesCount) {
				for (var i = 0; i < attributesCount; i++) {
					var attribute = attributes[i];
					var namespace = attribute.namespace;
					if (namespace && (namespace != attributeSpace.$current.namespace)) {
						if (namespace in attributeSpace) {
							attributeSpace.$current = attributeSpace[namespace];
							stream.push(0); // SWITCH
							stream.push(attributeSpace.$current.$code);
						}
						else
							throw new Error("WBXML: no attribute namespace: " + namespace);
					}
					var name = element.name;
					if (name in attributeSpace.$current)
						var token = attributeSpace.$current[name];
					else
						throw new Error("WBXML: no attribute name: " + name);
					stream.push(token);
					stream.push(0x03); // STR_I
					stream.push(attribute.value);
				}
				stream.push(0x01); // END
			}
			if (childrenCount) {
				for (var i = 0; i < childrenCount; i++) {
					var child = children[i];
					if (child.type == child.ELEMENT_NODE)
						this.serializeBinaryElement(stream, tagSpace, attributeSpace, child);
					else if (child.type == child.TEXT_NODE) {
						stream.push(0x03); // STR_I
						stream.push(child.value);
					}
				}
				stream.push(0x01); // END
			}
		</function>
	</object>

</package>
